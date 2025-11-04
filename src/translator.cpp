#include "translator.h"

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

QString extractMessageContent(const QJsonObject &messageObj)
{
    const QJsonValue contentValue = messageObj.value(QStringLiteral("content"));
    if (contentValue.isString())
    {
        return contentValue.toString().trimmed();
    }

    if (contentValue.isArray())
    {
        const QJsonArray contentArray = contentValue.toArray();
        QStringList textFragments;
        for (const QJsonValue &value : contentArray)
        {
            if (value.isObject())
            {
                const QJsonObject fragmentObj = value.toObject();
                if (fragmentObj.value(QStringLiteral("type")).toString() == QStringLiteral("text"))
                {
                    textFragments.append(fragmentObj.value(QStringLiteral("text")).toString());
                }
            }
            else if (value.isString())
            {
                textFragments.append(value.toString());
            }
        }

        return textFragments.join(QStringLiteral("\n")).trimmed();
    }

    return {};
}
} // namespace

Translator::Translator(/* args */)
{
}

Translator::~Translator()
{
}

QString Translator::translate_by_github_model(QString input, std::string src_lang, std::string target_lang, const QString &token)
{
    if (input.isEmpty())
    {
        return input;
    }

    const QString trimmedToken = token.trimmed();
    if (trimmedToken.isEmpty())
    {
        QMessageBox::warning(nullptr,
                             QObject::tr("Missing API key"),
                             QObject::tr("Please configure an API key for Github Model before translating."));
        return input;
    }

    ensureCurlInitialized();

    CURL *curl = curl_easy_init();
    if (!curl)
    {
        return input;
    }

    std::string responseBuffer;
    CURLcode res;
    struct curl_slist *headers = nullptr;

    headers = curl_slist_append(headers, "Content-Type: application/json");
    const QByteArray tokenBytes = trimmedToken.toUtf8();
    const std::string authHeader = "Authorization: Bearer " + tokenBytes.toStdString();
    headers = curl_slist_append(headers, authHeader.c_str());

    QJsonArray messages;
    QJsonObject systemMessage{
        {"role", "system"},
        {"content", QStringLiteral("You are a translation assistant. Translate text as faithfully as possible. Output only the final translated text. No explanations or extra content.")}};

    const QString prompt = QStringLiteral("Translate the following text from %1 to %2:\n%3")
                               .arg(QString::fromStdString(src_lang),
                                    QString::fromStdString(target_lang),
                                    input);

    QJsonObject userMessage{
        {"role", "user"},
        {"content", prompt}};

    messages.append(systemMessage);
    messages.append(userMessage);

    QJsonObject payload{
        {"model", QStringLiteral("openai/gpt-4o-mini")},
        {"messages", messages}};

    const QByteArray jsonBytes = QJsonDocument(payload).toJson(QJsonDocument::Compact);

    curl_easy_setopt(curl, CURLOPT_URL, "https://models.github.ai/inference/chat/completions");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonBytes.constData());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, jsonBytes.size());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBuffer);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "SRT-Editor/1.0");

    res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
    {
        return input;
    }

    const QJsonDocument responseDoc = QJsonDocument::fromJson(QByteArray::fromStdString(responseBuffer));
    if (!responseDoc.isObject())
    {
        return input;
    }

    const QJsonObject responseObj = responseDoc.object();
    const QJsonArray choices = responseObj.value(QStringLiteral("choices")).toArray();
    if (choices.isEmpty())
    {
        return input;
    }

    const QJsonObject choiceObj = choices.first().toObject();
    const QJsonObject messageObj = choiceObj.value(QStringLiteral("message")).toObject();
    const QString content = extractMessageContent(messageObj);

    if (content.trimmed().isEmpty())
    {
        return input;
    }

    return content.trimmed();
}

QString Translator::translate_by_openai(QString input, std::string src_lang, std::string target_lang, const QString &token)
{
    if (input.isEmpty())
    {
        return input;
    }

    const QString trimmedToken = token.trimmed();
    if (trimmedToken.isEmpty())
    {
        QMessageBox::warning(nullptr,
                             QObject::tr("Missing API key"),
                             QObject::tr("Please configure an API key for OpenAI before translating."));
        return input;
    }

    ensureCurlInitialized();

    CURL *curl = curl_easy_init();
    if (!curl)
    {
        return input;
    }

    std::string responseBuffer;
    struct curl_slist *headers = nullptr;

    headers = curl_slist_append(headers, "Content-Type: application/json");
    const QByteArray tokenBytes = trimmedToken.toUtf8();
    const std::string authHeader = "Authorization: Bearer " + tokenBytes.toStdString();
    headers = curl_slist_append(headers, authHeader.c_str());

    QJsonArray messages;
    QJsonObject developerMessage{
        {"role", "assistant"},
        {"content", QStringLiteral("You are a translation assistant. Translate text as faithfully as possible. Output only the final translated text. No explanations or extra content.")}};

    const QString prompt = QStringLiteral("Translate the following text from %1 to %2:\n%3")
                               .arg(QString::fromStdString(src_lang),
                                    QString::fromStdString(target_lang),
                                    input);

    QJsonObject userMessage{
        {"role", "user"},
        {"content", prompt}};

    messages.append(developerMessage);
    messages.append(userMessage);

    QJsonObject payload{
        {"model", QStringLiteral("gpt-5")},
        {"messages", messages}};

    const QByteArray jsonBytes = QJsonDocument(payload).toJson(QJsonDocument::Compact);

    curl_easy_setopt(curl, CURLOPT_URL, "https://api.openai.com/v1/chat/completions");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonBytes.constData());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, jsonBytes.size());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBuffer);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "SRT-Editor/1.0");

    const CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
    {
        return input;
    }

    const QJsonDocument responseDoc = QJsonDocument::fromJson(QByteArray::fromStdString(responseBuffer));
    if (!responseDoc.isObject())
    {
        return input;
    }

    const QJsonArray choices = responseDoc.object().value(QStringLiteral("choices")).toArray();
    if (choices.isEmpty())
    {
        return input;
    }

    const QJsonObject messageObj = choices.first().toObject().value(QStringLiteral("message")).toObject();
    const QString content = extractMessageContent(messageObj);

    if (content.trimmed().isEmpty())
    {
        return input;
    }

    return content.trimmed();
}

QString Translator::translate_by_gemini(QString input, std::string src_lang, std::string target_lang, const QString &token)
{
    if (token.trimmed().isEmpty())
    {
        QMessageBox::warning(nullptr,
                             QObject::tr("Missing API key"),
                             QObject::tr("Please configure an API key for Gemini before translating."));
    }
    return input;
}

QString Translator::translate_by_google_translate(QString input, std::string src_lang, std::string target_lang, const QString &token)
{
    if (token.trimmed().isEmpty())
    {
        QMessageBox::warning(nullptr,
                             QObject::tr("Missing API key"),
                             QObject::tr("Please configure an API key for Google Translate before translating."));
    }
    return input;
}
