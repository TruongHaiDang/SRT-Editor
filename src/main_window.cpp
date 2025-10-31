#include "main_window.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(std::make_unique<Ui::MainWindow>())
{
    ui->setupUi(this);
    ui->subtitleTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    resize(1280, 800);
    centerOnPrimaryScreen();

    connect(ui->actionClose, &QAction::triggered, this, &MainWindow::close);
    connect(ui->actionAdd_subtitle, &QAction::triggered, this, &MainWindow::add_subtitle);

    ui->statusbar->showMessage("Ready!");
}

MainWindow::~MainWindow() = default;

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

void MainWindow::add_subtitle()
{
    ui->subtitleTable->insertRow(ui->subtitleTable->rowCount());
}

void MainWindow::remove_subtitle()
{

}
