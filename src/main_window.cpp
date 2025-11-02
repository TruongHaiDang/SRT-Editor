#include "main_window.h"
#include "translator_window.h"
#include "settings_window.h"

#include <QDir>
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

namespace
{
    QString computeDurationString(const QString &start, const QString &end)
    {
        const QTime startTime = QTime::fromString(start, QStringLiteral("hh:mm:ss,zzz"));
        const QTime endTime = QTime::fromString(end, QStringLiteral("hh:mm:ss,zzz"));

        if (!startTime.isValid() || !endTime.isValid())
        {
            return {};
        }

        int msecs = startTime.msecsTo(endTime);
        if (msecs < 0)
        {
            msecs += 24 * 60 * 60 * 1000;
        }
        if (msecs < 0)
        {
            return {};
        }

        const int hours = msecs / (60 * 60 * 1000);
        const int minutes = (msecs / (60 * 1000)) % 60;
        const int seconds = (msecs / 1000) % 60;
        const int milliseconds = msecs % 1000;

        return QStringLiteral("%1:%2:%3,%4")
            .arg(hours, 2, 10, QLatin1Char('0'))
            .arg(minutes, 2, 10, QLatin1Char('0'))
            .arg(seconds, 2, 10, QLatin1Char('0'))
            .arg(milliseconds, 3, 10, QLatin1Char('0'));
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(std::make_unique<Ui::MainWindow>())
{
    ui->setupUi(this);
    baseWindowTitle_ = windowTitle();

    ui->subtitleTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->subtitleTable->horizontalHeader()->setStretchLastSection(true);
    resize(1280, 800);
    centerOnPrimaryScreen();

    connect(ui->actionNew, &QAction::triggered, this, &MainWindow::new_project);
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::open_project);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::save_project);
    connect(ui->actionSave_as, &QAction::triggered, this, &MainWindow::save_as_project);
    connect(ui->actionSettings, &QAction::triggered, this, &MainWindow::open_settings_window);
    connect(ui->actionClose, &QAction::triggered, this, &MainWindow::close);
    connect(ui->actionAdd_subtitle, &QAction::triggered, this, &MainWindow::add_subtitle);
    connect(ui->actionRemove_subtitle, &QAction::triggered, this, &MainWindow::remove_subtitle);
    connect(ui->actionAuto_translate, &QAction::triggered, this, &MainWindow::open_translator_window);
    connect(ui->actionAuthor, &QAction::triggered, this, &MainWindow::open_portfolio_website);

    init_settings();
    ui->statusbar->showMessage("Ready!");
}

MainWindow::~MainWindow() = default;

void MainWindow::init_settings()
{
    const QString provider = settings.value("ai/lang/provider").toString().trimmed();
    if (provider.isEmpty())
    {
        settings.setValue("ai/lang/provider", "OpenAI");
        settings.sync();
    }
}

void MainWindow::centerOnPrimaryScreen() noexcept
{
    const QScreen *const primaryScreen = QGuiApplication::primaryScreen();
    if (!primaryScreen)
    {
        return;
    }

    const QRect availableGeometry = primaryScreen->availableGeometry();
    const QSize windowSize = size();

    const int targetX = availableGeometry.center().x() - windowSize.width() / 2;
    const int targetY = availableGeometry.center().y() - windowSize.height() / 2;
    move(targetX, targetY);
}

void MainWindow::new_project()
{
    ui->subtitleTable->clearContents();
    ui->subtitleTable->setRowCount(0);
    currentProjectPath_.clear();
    setWindowTitle(baseWindowTitle_);
    ui->statusbar->showMessage(tr("New project created."));
}

void MainWindow::open_project()
{
    const QString initialDirectory = currentProjectPath_.isEmpty()
                                         ? QDir::homePath()
                                         : QFileInfo(currentProjectPath_).absolutePath();
    const QString filePath = QFileDialog::getOpenFileName(this,
                                                          tr("Open Subtitle File"),
                                                          initialDirectory,
                                                          tr("Subtitle Files (*.srt);;All Files (*.*)"));

    if (filePath.isEmpty())
    {
        return;
    }

    if (!load_project_from_file(filePath))
    {
        return;
    }

    const QFileInfo fileInfo(filePath);
    currentProjectPath_ = fileInfo.absoluteFilePath();
    setWindowTitle(QStringLiteral("%1 - %2").arg(baseWindowTitle_, fileInfo.fileName()));
    ui->statusbar->showMessage(tr("Opened %1 (%2 subtitles)")
                                   .arg(fileInfo.fileName())
                                   .arg(ui->subtitleTable->rowCount()));
}

void MainWindow::save_project()
{
    if (currentProjectPath_.isEmpty())
    {
        save_as_project();
        return;
    }

    if (!save_project_to_file(currentProjectPath_))
    {
        return;
    }

    const QFileInfo fileInfo(currentProjectPath_);
    ui->statusbar->showMessage(tr("Saved %1").arg(fileInfo.fileName()));
}

void MainWindow::save_as_project()
{
    QString suggestedPath = currentProjectPath_;
    if (suggestedPath.isEmpty())
    {
        suggestedPath = QDir::homePath() + QDir::separator() + tr("untitled.srt");
    }

    const QString filePath = QFileDialog::getSaveFileName(this,
                                                          tr("Save Subtitle File"),
                                                          suggestedPath,
                                                          tr("Subtitle Files (*.srt);;All Files (*.*)"));

    if (filePath.isEmpty())
    {
        return;
    }

    QString normalizedPath = filePath;
    if (!normalizedPath.endsWith(QStringLiteral(".srt"), Qt::CaseInsensitive))
    {
        normalizedPath += QStringLiteral(".srt");
    }

    if (!save_project_to_file(normalizedPath))
    {
        return;
    }

    const QFileInfo fileInfo(normalizedPath);
    currentProjectPath_ = fileInfo.absoluteFilePath();
    setWindowTitle(QStringLiteral("%1 - %2").arg(baseWindowTitle_, fileInfo.fileName()));
    ui->statusbar->showMessage(tr("Saved %1").arg(fileInfo.fileName()));
}

void MainWindow::open_settings_window()
{
    SettingsWindow settingsDialog(this);
    settingsDialog.exec();
}

bool MainWindow::load_project_from_file(const QString &file_path)
{
    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QMessageBox::warning(this,
                             tr("Open Failed"),
                             tr("Unable to open \"%1\"\n\n%2").arg(file_path, file.errorString()));
        return false;
    }

    QTextStream stream(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    stream.setEncoding(QStringConverter::Utf8);
#else
    stream.setCodec("UTF-8");
#endif

    ui->subtitleTable->setRowCount(0);

    QStringList block;
    auto processBlock = [this](const QStringList &lines)
    {
        if (lines.size() < 2)
        {
            return;
        }

        QStringList payload = lines;
        payload.removeFirst(); // subtitle index

        if (payload.isEmpty())
        {
            return;
        }

        const QString timingLine = payload.takeFirst();
        static const QRegularExpression timingPattern(
            QStringLiteral(R"((\d{2}:\d{2}:\d{2},\d{3})\s*-->\s*(\d{2}:\d{2}:\d{2},\d{3}))"));
        const QRegularExpressionMatch match = timingPattern.match(timingLine);
        if (!match.hasMatch())
        {
            return;
        }

        const QString start = match.captured(1);
        const QString end = match.captured(2);
        const QString text = payload.join(QStringLiteral("\n"));

        const QString duration = computeDurationString(start, end);

        const int row = ui->subtitleTable->rowCount();
        ui->subtitleTable->insertRow(row);
        ui->subtitleTable->setItem(row, 0, new QTableWidgetItem(start));
        ui->subtitleTable->setItem(row, 1, new QTableWidgetItem(end));
        ui->subtitleTable->setItem(row, 2, new QTableWidgetItem(duration));
        ui->subtitleTable->setItem(row, 3, new QTableWidgetItem(text));
    };

    while (!stream.atEnd())
    {
        QString line = stream.readLine();
        if (line.endsWith(QLatin1Char('\r')))
        {
            line.chop(1);
        }
        if (line.trimmed().isEmpty())
        {
            if (!block.isEmpty())
            {
                processBlock(block);
                block.clear();
            }
        }
        else
        {
            block << line;
        }
    }

    if (!block.isEmpty())
    {
        processBlock(block);
    }

    return true;
}

bool MainWindow::save_project_to_file(const QString &file_path)
{
    QFile file(file_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
    {
        QMessageBox::warning(this,
                             tr("Save Failed"),
                             tr("Unable to write \"%1\"\n\n%2").arg(file_path, file.errorString()));
        return false;
    }

    QTextStream stream(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    stream.setEncoding(QStringConverter::Utf8);
#else
    stream.setCodec("UTF-8");
#endif
    stream.setGenerateByteOrderMark(false);

    const int rowCount = ui->subtitleTable->rowCount();
    int subtitleIndex = 1;
    for (int row = 0; row < rowCount; ++row)
    {
        const QString start = ui->subtitleTable->item(row, 0) ? ui->subtitleTable->item(row, 0)->text() : QString();
        const QString end = ui->subtitleTable->item(row, 1) ? ui->subtitleTable->item(row, 1)->text() : QString();
        const QString text = ui->subtitleTable->item(row, 3) ? ui->subtitleTable->item(row, 3)->text() : QString();

        if (start.isEmpty() || end.isEmpty())
        {
            continue;
        }

        const QString duration = computeDurationString(start, end);
        if (!duration.isEmpty())
        {
            QTableWidgetItem *durationItem = ui->subtitleTable->item(row, 2);
            if (!durationItem)
            {
                durationItem = new QTableWidgetItem();
                ui->subtitleTable->setItem(row, 2, durationItem);
            }
            durationItem->setText(duration);
        }

        if (subtitleIndex > 1)
        {
            stream << "\r\n";
        }

        stream << subtitleIndex++ << "\r\n";
        stream << start << " --> " << end << "\r\n";
        const QStringList textLines = text.split(QLatin1Char('\n'));
        for (const QString &textLine : textLines)
        {
            stream << textLine << "\r\n";
        }
    }

    return true;
}

void MainWindow::add_subtitle()
{
    ui->subtitleTable->insertRow(ui->subtitleTable->rowCount());
    ui->statusbar->showMessage(QString("Subtitle %1 is added.").arg(ui->subtitleTable->rowCount()));
}

void MainWindow::remove_subtitle()
{
    QModelIndexList selected = ui->subtitleTable->selectionModel()->selectedRows();
    for (const QModelIndex &index : selected)
    {
        qDebug() << "Selected row:" << index.row();
        ui->statusbar->showMessage(QString("Remove subtitle %1").arg(index.row() + 1));
        ui->subtitleTable->removeRow(index.row());
    }
}

void MainWindow::open_translator_window()
{
    TranslatorWindow dialog(this);
    dialog.exec();
}

void MainWindow::open_portfolio_website()
{
    QDesktopServices::openUrl(QUrl(QStringLiteral("https://truonghaidang.com")));
}
