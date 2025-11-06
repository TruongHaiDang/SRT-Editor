#pragma once

#ifndef __TEXT_TO_SPEECH_H__
#define __TEXT_TO_SPEECH_H__

#include "ui_text_to_speech_window.h"
#include <QDialog>
#include <QWidget>
#include <QList>
#include <QString>

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

public:
    explicit TextToSpeechWindow(QWidget *parent = nullptr);
    ~TextToSpeechWindow() override;

};

#endif
