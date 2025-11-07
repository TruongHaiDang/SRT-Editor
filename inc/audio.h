#pragma once

#ifndef __AUDIO_H__
#define __AUDIO_H__

#include <QString>
#include <string>
#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QObject>
#include <QUrl>
#include <QList>

#include <curl/curl.h>

#include <mutex>
#include <vector>

#include <taglib/fileref.h>
#include <taglib/audioproperties.h>

class Audio
{
private:

public:
    Audio();
    ~Audio();

    void elevenlabs_text_to_speech(QString text, std::string filePath, std::string token);
    void elevenlabs_text_to_speech(QString text, std::string filePath, QString voice, QString model, std::string token);
    QList<QString> elevenlabs_get_voices(const QString &token);
    QList<QString> elevenlabs_get_models(const QString &token);
    
    void openai_text_to_speech(QString text, std::string filePath, std::string token);
    void openai_text_to_speech(QString text, std::string filePath, QString voice, QString model, std::string token);
    
    double get_audio_duration_seconds(const std::string &filePath);
};

#endif
