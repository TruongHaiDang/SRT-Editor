#pragma once

#include <QDialog>
#include <memory>

#include "settings.h"
#include "ui_translator_window.h"
#include "translator.h"

#include <QComboBox>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QStringList>
#include <QUrl>

#include <curl/curl.h>

#include <mutex>
#include <string>

namespace Ui
{
    class TranslatorWindow;
}

class TranslatorWindow : public QDialog
{
    Q_OBJECT

public:
    explicit TranslatorWindow(QWidget *parent = nullptr);
    ~TranslatorWindow() override;

    void setSourceTexts(const QStringList &sourceTexts);
    QStringList targetTexts() const;

private slots:
    void refreshModelList(const QString &service);
    void handleTranslateButton();
    void translateAll();

private:
    bool validateLanguageInputs();
    std::unique_ptr<Ui::TranslatorWindow> ui;
    Settings settings;
    Translator translator;
};
