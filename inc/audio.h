#pragma once

#ifndef __AUDIO_H__
#define __AUDIO_H__

#include <QString>
#include <string>

class Audio
{
private:

public:
    Audio();
    ~Audio();

    void elevenlabs_text_to_speech(QString text, std::string filePath, std::string token);
    void elevenlabs_text_to_speech(QString text, std::string filePath, QString voice, QString model, std::string token);
    void openai_text_to_speech(QString text, std::string filePath, std::string token);
    void openai_text_to_speech(QString text, std::string filePath, QString voice, QString model, std::string token);
    double get_audio_duration_seconds(const std::string &filePath);
};

#endif
