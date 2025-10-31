#pragma once

#include <QMainWindow>
#include <QGuiApplication>
#include <QScreen>
#include <memory>

#include "ui_main_window.h"

namespace Ui
{
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    void centerOnPrimaryScreen() noexcept;
    std::unique_ptr<Ui::MainWindow> ui;

    void add_subtitle();
    void remove_subtitle();
};
