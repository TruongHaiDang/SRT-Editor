#pragma once

#ifndef __TEXT_TO_SPEECH_H__
#define __TEXT_TO_SPEECH_H__

#include "ui_text_to_speech_window.h"
#include <QDialog>
#include <QWidget>

namespace Ui
{
    class TextToSpeechWindow;
}

class TextToSpeechWindow: public QDialog
{
    Q_OBJECT

private:
    std::unique_ptr<Ui::TextToSpeechWindow> ui;

public:
    explicit TextToSpeechWindow(QWidget *parent = nullptr);
    ~TextToSpeechWindow() override;

};

#endif
