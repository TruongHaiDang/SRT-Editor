#pragma once

#ifndef __TEXT_TO_SPEECH_H__
#define __TEXT_TO_SPEECH_H__

#include "ui_text_to_speech_window.h"
#include "settings.h"
#include <QDialog>
#include <QWidget>
#include <QList>
#include <QString>
#include <QVector>
#include <memory>

namespace Ui
{
    class TextToSpeechWindow;
}

class TextToSpeechWindow: public QDialog
{
    Q_OBJECT

private:
    std::unique_ptr<Ui::TextToSpeechWindow> ui;
    QList<QString> openaiModels = {"tts-1", "tts-1-hd", "gpt-4o-mini-tts"};
    QList<QString> openaiVoices = { "alloy", "ash", "ballad", "coral", "echo", "fable", "onyx", "nova", "sage", "shimmer", "verse" };
    Settings settings;
    QString outputDirectory_;
    QString defaultOutputDirButtonText_;

public:
    struct Entry
    {
        QString text;
        QString duration;
        QString filePath;
    };

private:
    void init_general_settings();
    void init_openai_settings();
    void init_elevenlabs_settings();
    void update_speed_label(int value);
    void refresh_output_directory_button();
    void select_output_directory();

public:
    explicit TextToSpeechWindow(QWidget *parent = nullptr);
    ~TextToSpeechWindow() override;

    void set_entries(const QVector<Entry> &entries);

};

#endif
