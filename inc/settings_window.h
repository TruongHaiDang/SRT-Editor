#pragma once

#include <QDialog>
#include <QTreeWidgetItem>
#include <QString>
#include <memory>
#include "settings.h"
#include "ui_settings_window.h"
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QVBoxLayout>

namespace Ui
{
    class SettingsWindow;
}

class SettingsWindow : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsWindow(QWidget *parent = nullptr);
    ~SettingsWindow() override;

private:
    std::unique_ptr<Ui::SettingsWindow> ui;
    Settings settings;

    void on_item_change(QTreeWidgetItem *current, QTreeWidgetItem *previous);
    void draw_ai_provider_language();
    void draw_ai_provider_text_to_speech();
};
