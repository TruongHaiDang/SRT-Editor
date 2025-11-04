#ifndef __TRANSLATOR_H__
#define __TRANSLATOR_H__

#include <QString>
#include <string>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QMessageBox>
#include <QObject>
#include <QByteArray>
#include <QTextStream>
#include <QStringList>

#include <curl/curl.h>

#include <cstdlib>
#include <string>
#include <mutex>

class Translator
{
private:
    /* data */
public:
    Translator(/* args */);
    ~Translator();

    QString translate_by_github_model(QString input, std::string src_lang, std::string target_lang, const QString &token);
    QString translate_by_openai(QString input, std::string src_lang, std::string target_lang, const QString &token);
    QString translate_by_gemini(QString input, std::string src_lang, std::string target_lang, const QString &token);
    QString translate_by_google_translate(QString input, std::string src_lang, std::string target_lang, const QString &token);
};

#endif
