#include "text_to_speech_window.h"

TextToSpeechWindow::TextToSpeechWindow(QWidget *parent) : QDialog(parent), ui(std::make_unique<Ui::TextToSpeechWindow>())
{
    ui->setupUi(this);
    connect(ui->btnCancle, &QPushButton::clicked, this, &TextToSpeechWindow::close);

    auto *header = ui->textTable->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->textTable->setColumnWidth(2, 110);
}

TextToSpeechWindow::~TextToSpeechWindow()
{
}
