#pragma once

#include <QDialog>
#include <memory>
#include <QTreeWidgetItem>
#include <QString>
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

    void on_item_change(QTreeWidgetItem *current, QTreeWidgetItem *previous);
};
