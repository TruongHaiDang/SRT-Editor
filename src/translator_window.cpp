#include "translator_window.h"

#include "ui_translator_window.h"

#include <QComboBox>
#include <QDebug>
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

QStringList parseModelNames(const QByteArray &payload)
{
    QStringList models;
    if (payload.isEmpty())
    {
        return models;
    }

    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(payload, &error);
    if (error.error != QJsonParseError::NoError)
    {
        qDebug() << "JSON parse error:" << error.errorString();
        return models;
    }

    const QJsonObject root = doc.object();
    qDebug() << "Root object keys:" << root.keys();
    
    const QJsonValue dataValue = root.value(QStringLiteral("data"));
    qDebug() << "dataValue type:" << static_cast<int>(dataValue.type());
    qDebug() << "dataValue isArray:" << dataValue.isArray();
    
    if (!dataValue.isArray())
    {
        return models;
    }

    const QJsonArray dataArray = dataValue.toArray();
    qDebug() << "Data array size:" << dataArray.size();
    
    for (const QJsonValue &item : dataArray)
    {
        if (!item.isObject())
        {
            continue;
        }

        const QJsonObject modelObj = item.toObject();
        const QString id = modelObj.value(QStringLiteral("id")).toString();
        if (id.isEmpty())
        {
            continue;
        }

        if (id.contains("gpt-"))
        {
            models.append(id);
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
    if (!provider.isEmpty())
    {
        refreshModelList(provider);
    }
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

    const QString token = settings.value("ai/lang/apiKey").toString().trimmed();
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
