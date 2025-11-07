#include "audio.h"
#include "settings.h"

namespace
{
size_t writeBinaryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    const size_t totalSize = size * nmemb;
    if (totalSize == 0 || userp == nullptr)
    {
        return 0;
    }

    auto *buffer = static_cast<std::vector<unsigned char> *>(userp);
    const auto *data = static_cast<const unsigned char *>(contents);
    buffer->insert(buffer->end(), data, data + totalSize);
    return totalSize;
}

void ensureCurlInitialized()
{
    static std::once_flag curlInitFlag;
    std::call_once(curlInitFlag, []() { curl_global_init(CURL_GLOBAL_DEFAULT); });
}

bool writeBufferToFile(const std::string &filePath, const std::vector<unsigned char> &buffer)
{
    if (filePath.empty() || buffer.empty())
    {
        return false;
    }

    const QString qFilePath = QString::fromStdString(filePath);
    const QFileInfo fileInfo(qFilePath);
    QDir dir = fileInfo.dir();
    if (!dir.exists())
    {
        if (!dir.mkpath(QStringLiteral(".")))
        {
            return false;
        }
    }

    QFile file(qFilePath);
    if (!file.open(QIODevice::WriteOnly))
    {
        return false;
    }

    const qint64 written = file.write(reinterpret_cast<const char *>(buffer.data()),
                                      static_cast<qint64>(buffer.size()));
    file.close();

    return written == static_cast<qint64>(buffer.size());
}

QString describeCurlFailure(CURLcode code, long httpStatus)
{
    QString message = QObject::tr("Network request failed.");
    if (code != CURLE_OK)
    {
        message += QObject::tr(" Error: %1.").arg(QString::fromUtf8(curl_easy_strerror(code)));
    }
    if (httpStatus >= 400)
    {
        message += QObject::tr(" HTTP status: %1.").arg(httpStatus);
    }
    return message;
}

QByteArray performElevenLabsJsonGet(const QByteArray &url,
                                    const QString &token,
                                    bool silent,
                                    const QString &errorTitle,
                                    long *outStatus,
                                    CURLcode *outCurlCode)
{
    ensureCurlInitialized();

    CURL *curl = curl_easy_init();
    if (!curl)
    {
        return {};
    }

    std::vector<unsigned char> response;
    struct curl_slist *headers = nullptr;

    const std::string authHeader = "xi-api-key: " + token.toStdString();
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, authHeader.c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.constData());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeBinaryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "SRT-Editor/1.0");

    const CURLcode res = curl_easy_perform(curl);
    long httpStatus = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpStatus);
    if (outStatus)
    {
        *outStatus = httpStatus;
    }
    if (outCurlCode)
    {
        *outCurlCode = res;
    }

    if (headers)
    {
        curl_slist_free_all(headers);
    }
    curl_easy_cleanup(curl);

    if (!silent && (res != CURLE_OK || httpStatus >= 400))
    {
        QMessageBox::warning(nullptr,
                             errorTitle,
                             describeCurlFailure(res, httpStatus));
        return {};
    }

    if (response.empty())
    {
        return {};
    }

    return QByteArray(reinterpret_cast<const char *>(response.data()), static_cast<int>(response.size()));
}

QList<QString> extractIdsFromArray(const QJsonArray &array, const QString &idKey)
{
    QList<QString> ids;
    ids.reserve(array.size());

    for (const QJsonValue &value : array)
    {
        if (!value.isObject())
        {
            continue;
        }

        const QString id = value.toObject().value(idKey).toString().trimmed();
        if (!id.isEmpty())
        {
            ids.append(id);
        }
    }

    return ids;
}
} // namespace

Audio::Audio() = default;

Audio::~Audio() = default;

void Audio::elevenlabs_text_to_speech(QString text, std::string filePath, std::string token)
{
    this->elevenlabs_text_to_speech(text, filePath, QString(), QString(), token);
}

void Audio::elevenlabs_text_to_speech(QString text,
                                      std::string filePath,
                                      QString voice,
                                      QString model,
                                      std::string token)
{
    const QString trimmedText = text.trimmed();
    if (trimmedText.isEmpty())
    {
        QMessageBox::information(nullptr,
                                 QObject::tr("Nothing to convert"),
                                 QObject::tr("Please provide text before requesting speech synthesis."));
        return;
    }

    const QString trimmedToken = QString::fromStdString(token).trimmed();
    if (trimmedToken.isEmpty())
    {
        QMessageBox::warning(nullptr,
                             QObject::tr("Missing API key"),
                             QObject::tr("An ElevenLabs API key is required to generate speech."));
        return;
    }

    QString voiceId = voice.trimmed();
    if (voiceId.isEmpty())
    {
        QMessageBox::warning(nullptr,
                             QObject::tr("Missing voice"),
                             QObject::tr("Please provide a valid ElevenLabs voice identifier."));
        return;
    }

    QString modelId = model.trimmed();
    if (modelId.isEmpty())
    {
        modelId = QStringLiteral("eleven_turbo_v2");
    }

    ensureCurlInitialized();

    CURL *curl = curl_easy_init();
    if (!curl)
    {
        QMessageBox::warning(nullptr,
                             QObject::tr("Failed to initialize"),
                             QObject::tr("Unable to initialize network stack for ElevenLabs request."));
        return;
    }

    std::vector<unsigned char> audioBuffer;
    struct curl_slist *headers = nullptr;

    const std::string authHeader = "xi-api-key: " + trimmedToken.toStdString();
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: audio/mpeg");
    headers = curl_slist_append(headers, authHeader.c_str());

    QJsonObject payload{{QStringLiteral("text"), trimmedText},
                        {QStringLiteral("model_id"), modelId},
                        {QStringLiteral("voice_id"), voiceId}};

    const QByteArray jsonBytes = QJsonDocument(payload).toJson(QJsonDocument::Compact);

    QString endpoint = QStringLiteral("https://api.elevenlabs.io/v1/text-to-speech/%1").arg(voiceId);

    curl_easy_setopt(curl, CURLOPT_URL, endpoint.toUtf8().constData());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonBytes.constData());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(jsonBytes.size()));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeBinaryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &audioBuffer);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "SRT-Editor/1.0");

    const CURLcode res = curl_easy_perform(curl);
    long httpStatus = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpStatus);

    if (headers)
    {
        curl_slist_free_all(headers);
    }
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || httpStatus >= 400 || audioBuffer.empty())
    {
        QMessageBox::warning(nullptr,
                             QObject::tr("Conversion failed"),
                             describeCurlFailure(res, httpStatus));
        return;
    }

    if (!writeBufferToFile(filePath, audioBuffer))
    {
        QMessageBox::warning(nullptr,
                             QObject::tr("Save failed"),
                             QObject::tr("Unable to write synthesized speech to %1.")
                                 .arg(QString::fromStdString(filePath)));
    }
}

QList<QString> Audio::elevenlabs_get_voices(const QString &token)
{
    const QString trimmedToken = token.trimmed();
    if (trimmedToken.isEmpty())
    {
        QMessageBox::warning(nullptr,
                             QObject::tr("Missing API key"),
                             QObject::tr("Please configure an API key for ElevenLabs in Settings ▸ Audio."));
        return {};
    }

    long httpStatus = 0;
    CURLcode curlCode = CURLE_OK;
    const QByteArray payload = performElevenLabsJsonGet(QByteArrayLiteral("https://api.elevenlabs.io/v1/voices"),
                                                        trimmedToken,
                                                        true,
                                                        QObject::tr("Unable to fetch voices"),
                                                        &httpStatus,
                                                        &curlCode);
    if (payload.isEmpty())
    {
        if (httpStatus == 401)
        {
            QMessageBox::warning(nullptr,
                                 QObject::tr("Invalid ElevenLabs API key"),
                                 QObject::tr("Your ElevenLabs API key was rejected (HTTP 401). Please update it in Settings ▸ Audio."));
        }
        else if (httpStatus >= 400 || curlCode != CURLE_OK)
        {
            QMessageBox::warning(nullptr,
                                 QObject::tr("Unable to fetch voices"),
                                 describeCurlFailure(curlCode, httpStatus));
        }
        return {};
    }

    const QJsonDocument doc = QJsonDocument::fromJson(payload);
    if (doc.isArray())
    {
        return extractIdsFromArray(doc.array(), QStringLiteral("voice_id"));
    }

    if (doc.isObject())
    {
        return extractIdsFromArray(doc.object().value(QStringLiteral("voices")).toArray(), QStringLiteral("voice_id"));
    }

    return {};
}

QList<QString> Audio::elevenlabs_get_models(const QString &token)
{
    const QString trimmedToken = token.trimmed();
    if (trimmedToken.isEmpty())
    {
        QMessageBox::warning(nullptr,
                             QObject::tr("Missing API key"),
                             QObject::tr("Please configure an API key for ElevenLabs in Settings ▸ Audio."));
        return {};
    }

    long httpStatus = 0;
    CURLcode curlCode = CURLE_OK;
    const QByteArray payload = performElevenLabsJsonGet(QByteArrayLiteral("https://api.elevenlabs.io/v1/models"),
                                                        trimmedToken,
                                                        true,
                                                        QObject::tr("Unable to fetch models"),
                                                        &httpStatus,
                                                        &curlCode);
    if (payload.isEmpty())
    {
        if (httpStatus == 401)
        {
            QMessageBox::warning(nullptr,
                                 QObject::tr("Invalid ElevenLabs API key"),
                                 QObject::tr("Your ElevenLabs API key was rejected (HTTP 401). Please update it in Settings ▸ Audio."));
        }
        else if (httpStatus >= 400 || curlCode != CURLE_OK)
        {
            QMessageBox::warning(nullptr,
                                 QObject::tr("Unable to fetch models"),
                                 describeCurlFailure(curlCode, httpStatus));
        }
        return {};
    }

    QList<QString> models;
    const QJsonDocument doc = QJsonDocument::fromJson(payload);
    if (doc.isArray())
    {
        models = extractIdsFromArray(doc.array(), QStringLiteral("model_id"));
    }
    else if (doc.isObject())
    {
        models = extractIdsFromArray(doc.object().value(QStringLiteral("models")).toArray(), QStringLiteral("model_id"));
    }

    return models;
}

void Audio::openai_text_to_speech(QString text, std::string filePath, std::string token)
{
    this->openai_text_to_speech(text, filePath, QString(), QString(), token);
}

void Audio::openai_text_to_speech(QString text,
                                  std::string filePath,
                                  QString voice,
                                  QString model,
                                  std::string token)
{
    const QString trimmedText = text.trimmed();
    if (trimmedText.isEmpty())
    {
        QMessageBox::information(nullptr,
                                 QObject::tr("Nothing to convert"),
                                 QObject::tr("Please provide text before requesting speech synthesis."));
        return;
    }

    const QString trimmedToken = QString::fromStdString(token).trimmed();
    if (trimmedToken.isEmpty())
    {
        QMessageBox::warning(nullptr,
                             QObject::tr("Missing API key"),
                             QObject::tr("An OpenAI API key is required to generate speech."));
        return;
    }

    QString voiceName = voice.trimmed();
    if (voiceName.isEmpty())
    {
        voiceName = QStringLiteral("alloy");
    }

    QString modelName = model.trimmed();
    if (modelName.isEmpty())
    {
        modelName = QStringLiteral("gpt-4o-mini-tts");
    }

    ensureCurlInitialized();

    CURL *curl = curl_easy_init();
    if (!curl)
    {
        QMessageBox::warning(nullptr,
                             QObject::tr("Failed to initialize"),
                             QObject::tr("Unable to initialize network stack for OpenAI request."));
        return;
    }

    std::vector<unsigned char> audioBuffer;
    struct curl_slist *headers = nullptr;

    const std::string authHeader = "Authorization: Bearer " + trimmedToken.toStdString();
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: audio/mpeg");
    headers = curl_slist_append(headers, authHeader.c_str());

    QJsonObject payload{
        {QStringLiteral("model"), modelName},
        {QStringLiteral("voice"), voiceName},
        {QStringLiteral("input"), trimmedText},
        {QStringLiteral("response_format"), QStringLiteral("mp3")}};
    
    const QByteArray jsonBytes = QJsonDocument(payload).toJson(QJsonDocument::Compact);

    curl_easy_setopt(curl, CURLOPT_URL, "https://api.openai.com/v1/audio/speech");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonBytes.constData());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(jsonBytes.size()));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeBinaryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &audioBuffer);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "SRT-Editor/1.0");

    const CURLcode res = curl_easy_perform(curl);
    long httpStatus = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpStatus);

    if (headers)
    {
        curl_slist_free_all(headers);
    }
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || httpStatus >= 400 || audioBuffer.empty())
    {
        QMessageBox::warning(nullptr,
                             QObject::tr("Conversion failed"),
                             describeCurlFailure(res, httpStatus));
        return;
    }

    if (!writeBufferToFile(filePath, audioBuffer))
    {
        QMessageBox::warning(nullptr,
                             QObject::tr("Save failed"),
                             QObject::tr("Unable to write synthesized speech to %1.")
                                 .arg(QString::fromStdString(filePath)));
    }
}

double Audio::get_audio_duration_seconds(const std::string &filePath)
{
    if (filePath.empty())
    {
        return 0.0;
    }

    const QString qFilePath = QString::fromStdString(filePath);
    if (!QFile::exists(qFilePath))
    {
        return 0.0;
    }

#ifdef _WIN32
    const std::wstring widePath = qFilePath.toStdWString();
    TagLib::FileRef fileRef(widePath.c_str());
#else
    QByteArray pathBytes = QFile::encodeName(qFilePath);
    TagLib::FileRef fileRef(pathBytes.constData());
#endif

    if (fileRef.isNull())
    {
        return 0.0;
    }

    const TagLib::AudioProperties *properties = fileRef.audioProperties();
    if (!properties)
    {
        return 0.0;
    }

    const int lengthMs = properties->lengthInMilliseconds();
    if (lengthMs > 0)
    {
        return static_cast<double>(lengthMs) / 1000.0;
    }

    const int lengthSeconds = properties->length();
    if (lengthSeconds > 0)
    {
        return static_cast<double>(lengthSeconds);
    }

    return 0.0;
}
