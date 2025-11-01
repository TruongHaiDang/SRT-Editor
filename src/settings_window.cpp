#include "settings_window.h"

#include "ui_settings_window.h"

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QVBoxLayout>

SettingsWindow::SettingsWindow(QWidget *parent)
    : QDialog(parent), ui(std::make_unique<Ui::SettingsWindow>())
{
    ui->setupUi(this);
    connect(ui->settingTree, &QTreeWidget::currentItemChanged, this, &SettingsWindow::on_item_change);

    if (QTreeWidgetItem *firstItem = ui->settingTree->topLevelItem(0))
    {
        ui->settingTree->setCurrentItem(firstItem);
    }
}

SettingsWindow::~SettingsWindow() = default;

void SettingsWindow::on_item_change(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    Q_UNUSED(previous);

    if (!current)
    {
        return;
    }

    const QString text = current->text(0);
    if (text == tr("Language"))
    {
        draw_ai_provider_language();
        return;
    }

    QWidget *oldWidget = ui->settingContent->takeWidget();
    if (oldWidget)
    {
        oldWidget->deleteLater();
    }
}

void SettingsWindow::draw_ai_provider_language()
{
    QWidget *oldWidget = ui->settingContent->takeWidget();
    if (oldWidget)
    {
        oldWidget->deleteLater();
    }

    QWidget *container = new QWidget(ui->settingContent);
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    auto *providerLabel = new QLabel(tr("API Provider"), container);
    providerLabel->setObjectName(QStringLiteral("providerLabel"));

    auto *providerCombo = new QComboBox(container);
    providerCombo->setObjectName(QStringLiteral("providerCombo"));
    providerCombo->addItems({tr("OpenAI"), tr("Gemini"), tr("Github Model"), tr("Google Translate")});

    const QString providerKey = QStringLiteral("ai/provider");
    const QString storedProvider = settings_.value(providerKey, providerCombo->itemText(0)).toString();
    const int providerIndex = providerCombo->findText(storedProvider, Qt::MatchFixedString);
    providerCombo->setCurrentIndex(providerIndex == -1 ? 0 : providerIndex);

    auto *apiKeyLabel = new QLabel(tr("API Key"), container);
    apiKeyLabel->setObjectName(QStringLiteral("apiKeyLabel"));

    auto *apiKeyEdit = new QLineEdit(container);
    apiKeyEdit->setObjectName(QStringLiteral("apiKeyEdit"));
    apiKeyEdit->setEchoMode(QLineEdit::Password);
    const QString apiKeyKey = QStringLiteral("ai/apiKey");
    apiKeyEdit->setText(settings_.value(apiKeyKey).toString());

    layout->addWidget(providerLabel);
    layout->addWidget(providerCombo);
    layout->addWidget(apiKeyLabel);
    layout->addWidget(apiKeyEdit);
    layout->addStretch(1);

    container->setLayout(layout);
    ui->settingContent->setWidget(container);

    connect(providerCombo, &QComboBox::currentTextChanged, this, [this, providerKey](const QString &value) {
        settings_.setValue(providerKey, value);
        settings_.sync();
    });

    connect(apiKeyEdit, &QLineEdit::textChanged, this, [this, apiKeyKey](const QString &value) {
        settings_.setValue(apiKeyKey, value);
        settings_.sync();
    });
}
