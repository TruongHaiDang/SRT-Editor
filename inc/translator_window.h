#pragma once

#include <QDialog>
#include <memory>

#include "settings.h"
#include "ui_translator_window.h"

#include <QComboBox>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QProcessEnvironment>
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

private slots:
    void refreshModelList(const QString &service);

private:
    QString apiKeyForService(const QString &service) const;

    std::unique_ptr<Ui::TranslatorWindow> ui;
    Settings settings;
};
