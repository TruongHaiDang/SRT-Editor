#pragma once

#include <QMainWindow>
#include <QGuiApplication>
#include <QScreen>
#include <memory>
#include <QString>
#include <QDesktopServices>
#include "translator_window.h"
#include "settings_window.h"
#include "text_to_speech_window.h"
#include <QDir>
#include <algorithm>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QMessageBox>
#include <QRegularExpression>
#include <QTableWidgetItem>
#include <QTextStream>
#include <QTime>
#include <QtGlobal>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QStringConverter>
#endif
#include "settings.h"
#include "configure.h"
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

    QString baseWindowTitle_;
    QString currentProjectPath_;
    Settings settings;

    void init_settings();
    void new_project();
    void open_project();
    void save_project();
    void save_as_project();
    void open_settings_window();
    bool load_project_from_file(const QString &file_path);
    bool save_project_to_file(const QString &file_path);
    void add_subtitle();
    void remove_subtitle();
    void open_translator_window();
    void open_portfolio_website();
    void open_text_to_speech_window();
    void show_software_info();
};
