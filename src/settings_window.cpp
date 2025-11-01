#include "settings_window.h"

#include "ui_settings_window.h"

SettingsWindow::SettingsWindow(QWidget *parent)
    : QDialog(parent), ui(std::make_unique<Ui::SettingsWindow>())
{
    ui->setupUi(this);
    connect(ui->settingTree, &QTreeWidget::currentItemChanged, this, &SettingsWindow::on_item_change);
}

SettingsWindow::~SettingsWindow() = default;

void SettingsWindow::on_item_change(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    QTreeWidgetItem *item = ui->settingTree->currentItem();
    if (item)
    {
        QString text = item->text(0); // cột 0
        if (text == "Language") {
            // code
        } else if (text == "Audio") {
            // code
        } else {
            // default
        }
    }
}
