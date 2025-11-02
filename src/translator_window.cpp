#include "translator_window.h"

#include "ui_translator_window.h"

#include <QComboBox>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QStringList>
#include <QUrl>

#include <curl/curl.h>

#include <mutex>
#include <string>

namespace
{
size_t writeCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    const size_t totalSize = size * nmemb;
    auto *buffer = static_cast<std::string *>(userp);
    buffer->append(static_cast<const char *>(contents), totalSize);
    return totalSize;
}

void ensureCurlInitialized()
{
    static std::once_flag curlInitFlag;
    std::call_once(curlInitFlag, []() { curl_global_init(CURL_GLOBAL_DEFAULT); });
}

QByteArray performGetRequest(const QByteArray &url, const QList<QByteArray> &headers = {})
{
    ensureCurlInitialized();

    struct curl_slist *curlHeaders = nullptr;
    for (const QByteArray &header : headers)
    {
        curlHeaders = curl_slist_append(curlHeaders, header.constData());
    }

    CURL *curl = curl_easy_init();
    if (!curl)
    {
        if (curlHeaders)
        {
            curl_slist_free_all(curlHeaders);
        }
        return {};
    }

    std::string response;

    curl_easy_setopt(curl, CURLOPT_URL, url.constData());
    if (curlHeaders)
    {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curlHeaders);
    }
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "SRT-Editor/1.0");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    const CURLcode res = curl_easy_perform(curl);
    long statusCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &statusCode);

    if (curlHeaders)
    {
        curl_slist_free_all(curlHeaders);
    }
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || statusCode >= 400)
    {
        return {};
    }

    return QByteArray::fromStdString(response);
}

bool containsTextModality(const QJsonObject &obj)
{
    const auto modalityArrays = {
        QStringLiteral("modalities"),
        QStringLiteral("supportedModalities"),
        QStringLiteral("capabilities"),
        QStringLiteral("input_modalities"),
        QStringLiteral("output_modalities")
    };

    for (const QString &key : modalityArrays)
    {
        const QJsonValue value = obj.value(key);
        if (value.isArray())
        {
            const QJsonArray array = value.toArray();
            for (const QJsonValue &item : array)
            {
                const QString modality = item.toString().toLower();
                if (modality.contains(QStringLiteral("text")) || modality.contains(QStringLiteral("chat")))
                {
                    return true;
                }
            }
        }
        else if (value.isString())
        {
            const QString modality = value.toString().toLower();
            if (modality.contains(QStringLiteral("text")) || modality.contains(QStringLiteral("chat")))
            {
                return true;
            }
        }
        else if (value.isObject())
        {
            const QJsonObject capObj = value.toObject();
            for (auto it = capObj.constBegin(); it != capObj.constEnd(); ++it)
            {
                const QString keyLower = it.key().toLower();
                if ((keyLower.contains(QStringLiteral("text")) || keyLower.contains(QStringLiteral("chat"))) &&
                    it.value().toBool(false))
                {
                    return true;
                }
            }
        }
    }

    return false;
}

bool looksLikeTextModel(const QString &identifier, const QJsonObject &source)
{
    const QString idLower = identifier.toLower();
    static const QStringList forbiddenKeywords = {
        QStringLiteral("audio"),
        QStringLiteral("speech"),
        QStringLiteral("voice"),
        QStringLiteral("whisper"),
        QStringLiteral("image"),
        QStringLiteral("vision"),
        QStringLiteral("video"),
        QStringLiteral("diffusion"),
        QStringLiteral("render"),
        QStringLiteral("music")
    };

    for (const QString &keyword : forbiddenKeywords)
    {
        if (idLower.contains(keyword))
        {
            return false;
        }
    }

    if (!source.isEmpty() && containsTextModality(source))
    {
        return true;
    }

    return source.isEmpty();
}

QStringList parseModelNames(const QByteArray &payload)
{
    QStringList models;
    if (payload.isEmpty())
    {
        return models;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError)
    {
        return models;
    }

    auto appendFromArray = [&models](const QJsonArray &array) {
        for (const QJsonValue &value : array)
        {
            QString candidate;
            QJsonObject sourceObject;
            if (value.isObject())
            {
                const QJsonObject obj = value.toObject();
                sourceObject = obj;
                candidate = obj.value(QStringLiteral("id")).toString();
                if (candidate.isEmpty())
                {
                    candidate = obj.value(QStringLiteral("name")).toString();
                }
                if (candidate.isEmpty())
                {
                    candidate = obj.value(QStringLiteral("model")).toString();
                }
            }
            else if (value.isString())
            {
                candidate = value.toString();
            }

            if (!candidate.isEmpty() && looksLikeTextModel(candidate, sourceObject))
            {
                if (!models.contains(candidate))
                {
                    models.append(candidate);
                }
            }
        }
    };

    if (doc.isArray())
    {
        appendFromArray(doc.array());
    }
    else if (doc.isObject())
    {
        const QJsonObject obj = doc.object();
        static const char *keys[] = {"models", "data", "items", "summaries"};
        for (const char *key : keys)
        {
            const QJsonValue value = obj.value(QString::fromUtf8(key));
            if (value.isArray())
            {
                appendFromArray(value.toArray());
            }
        }

        const QString singleId = obj.value(QStringLiteral("id")).toString();
        if (!singleId.isEmpty() && looksLikeTextModel(singleId, obj) && !models.contains(singleId))
        {
            models.append(singleId);
        }

        const QString singleName = obj.value(QStringLiteral("name")).toString();
        if (!singleName.isEmpty() && looksLikeTextModel(singleName, obj) && !models.contains(singleName))
        {
            models.append(singleName);
        }
    }

    return models;
}

QStringList fetchGithubModels(const QString &token)
{
    if (token.isEmpty())
    {
        return {};
    }

    QList<QByteArray> headers;
    headers << QByteArray("Authorization: Bearer ") + token.toUtf8();
    headers << QByteArray("Accept: application/json");

    const QByteArray response = performGetRequest(QByteArrayLiteral("https://api.github.com/models/catalog"), headers);
    return parseModelNames(response);
}

QStringList fetchOpenAIModels(const QString &token)
{
    if (token.isEmpty())
    {
        return {};
    }

    QList<QByteArray> headers;
    headers << QByteArray("Authorization: Bearer ") + token.toUtf8();
    headers << QByteArray("Accept: application/json");

    const QByteArray response = performGetRequest(QByteArrayLiteral("https://api.openai.com/v1/models"), headers);
    return parseModelNames(response);
}

QStringList fetchGeminiModels(const QString &apiKey)
{
    if (apiKey.isEmpty())
    {
        return {};
    }

    const QByteArray encodedKey = QUrl::toPercentEncoding(apiKey);
    const QByteArray url = QByteArray("https://generativelanguage.googleapis.com/v1beta/models?key=") + encodedKey;

    QList<QByteArray> headers;
    headers << QByteArray("Accept: application/json");

    const QByteArray response = performGetRequest(url, headers);
    return parseModelNames(response);
}
} // namespace

TranslatorWindow::TranslatorWindow(QWidget *parent)
    : QDialog(parent), ui(std::make_unique<Ui::TranslatorWindow>())
{
    ui->setupUi(this);
    ui->subtitleTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    const QString provider = settings.value("ai/lang/provider").toString().trimmed();
    qDebug() << "Provider" << provider;
}

TranslatorWindow::~TranslatorWindow() = default;

void TranslatorWindow::refreshModelList(const QString &service)
{
    ui->modelList->clear();

    const bool isGoogleTranslate = service.compare(QStringLiteral("Google Translate"), Qt::CaseInsensitive) == 0;
    ui->modelList->setEnabled(!isGoogleTranslate);

    if (isGoogleTranslate)
    {
        return;
    }

    const QString token = apiKeyForService(service);
    if (token.isEmpty())
    {
        return;
    }

    QStringList models;
    if (service.compare(QStringLiteral("Github Model"), Qt::CaseInsensitive) == 0)
    {
        models = fetchGithubModels(token);
    }
    else if (service.compare(QStringLiteral("OpenAI"), Qt::CaseInsensitive) == 0)
    {
        models = fetchOpenAIModels(token);
    }
    else if (service.compare(QStringLiteral("Gemini"), Qt::CaseInsensitive) == 0)
    {
        models = fetchGeminiModels(token);
    }

    if (!models.isEmpty())
    {
        ui->modelList->addItems(models);
    }
}

QString TranslatorWindow::apiKeyForService(const QString &service) const
{
    const auto tryKeys = [this](const QStringList &keys) -> QString {
        for (const QString &key : keys)
        {
            const QString value = settings.value(key).toString().trimmed();
            if (!value.isEmpty())
            {
                return value;
            }
        }
        return {};
    };

    const QString lower = service.toLower();
    QStringList candidates;

    if (lower.contains(QStringLiteral("github")))
    {
        candidates << QStringLiteral("ai/github/apiKey")
                   << QStringLiteral("github/apiKey")
                   << QStringLiteral("translator/github/apiKey");
    }
    else if (lower.contains(QStringLiteral("openai")))
    {
        candidates << QStringLiteral("ai/openai/apiKey")
                   << QStringLiteral("openai/apiKey")
                   << QStringLiteral("translator/openai/apiKey");
    }
    else if (lower.contains(QStringLiteral("gemini")))
    {
        candidates << QStringLiteral("ai/gemini/apiKey")
                   << QStringLiteral("gemini/apiKey")
                   << QStringLiteral("translator/gemini/apiKey");
    }
    else if (lower.contains(QStringLiteral("google")))
    {
        candidates << QStringLiteral("ai/google/apiKey")
                   << QStringLiteral("google/apiKey")
                   << QStringLiteral("translator/google/apiKey");
    }

    candidates << QStringLiteral("ai/apiKey")
               << QStringLiteral("translator/apiKey");

    return tryKeys(candidates);
}
