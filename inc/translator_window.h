#pragma once

#include <QDialog>
#include <memory>

namespace Ui
{
    class TranslatorWindow;
}

class TranslatorWindow : public QDialog
{
    Q_OBJECT

public:
    explicit TranslatorWindow(QWidget *parent = nullptr);
    ~TranslatorWindow() override;

private:
    std::unique_ptr<Ui::TranslatorWindow> ui;
};
