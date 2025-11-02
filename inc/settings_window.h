#pragma once

#include <QDialog>
#include <QTreeWidgetItem>
#include <QString>
#include <memory>

#include "settings.h"

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
};
