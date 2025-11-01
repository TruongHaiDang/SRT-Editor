#include "translator_window.h"

#include "ui_translator_window.h"

TranslatorWindow::TranslatorWindow(QWidget *parent)
    : QDialog(parent), ui(std::make_unique<Ui::TranslatorWindow>())
{
    ui->setupUi(this);
    ui->subtitleTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

TranslatorWindow::~TranslatorWindow() = default;
