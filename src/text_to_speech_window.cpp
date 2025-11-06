#include "text_to_speech_window.h"

#include <algorithm>

#include <QHeaderView>
#include <QComboBox>
#include <QSlider>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QFileDialog>
#include <QDir>
#include <QTableWidgetItem>
#include <QPushButton>
#include <QtGlobal>

TextToSpeechWindow::TextToSpeechWindow(QWidget *parent)
    : QDialog(parent), ui(std::make_unique<Ui::TextToSpeechWindow>())
{
    ui->setupUi(this);
    defaultOutputDirButtonText_ = ui->pushButtonOutputDir->text();

    connect(ui->btnCancle, &QPushButton::clicked, this, &TextToSpeechWindow::close);
    connect(ui->pushButtonOutputDir, &QPushButton::clicked, this, &TextToSpeechWindow::select_output_directory);

    auto *header = ui->textTable->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    ui->textTable->setColumnWidth(3, 110);

    init_general_settings();
    init_openai_settings();
    init_elevenlabs_settings();
}

TextToSpeechWindow::~TextToSpeechWindow() = default;

void TextToSpeechWindow::init_general_settings()
{
    ui->comboBoxVoices->clear();
    ui->comboBoxVoices->addItems(openaiVoices);

    ui->comboBoxModels->clear();
    ui->comboBoxModels->addItems(openaiModels);

    const QString voiceKey = QStringLiteral("tts/general/voice");
    const QString storedVoice = settings.value(voiceKey).toString();
    if (!storedVoice.isEmpty() && ui->comboBoxVoices->findText(storedVoice) == -1)
    {
        ui->comboBoxVoices->addItem(storedVoice);
    }
    const int voiceIndex = ui->comboBoxVoices->findText(storedVoice);
    ui->comboBoxVoices->setCurrentIndex(voiceIndex == -1 ? 0 : voiceIndex);

    const QString modelKey = QStringLiteral("tts/general/model");
    const QString storedModel = settings.value(modelKey).toString();
    if (!storedModel.isEmpty() && ui->comboBoxModels->findText(storedModel) == -1)
    {
        ui->comboBoxModels->addItem(storedModel);
    }
    const int modelIndex = ui->comboBoxModels->findText(storedModel);
    ui->comboBoxModels->setCurrentIndex(modelIndex == -1 ? 0 : modelIndex);

    const QString outputKey = QStringLiteral("tts/general/output_format");
    const QString storedOutput = settings.value(outputKey, ui->comboBoxOutputType->itemText(0)).toString();
    if (ui->comboBoxOutputType->findText(storedOutput) == -1)
    {
        ui->comboBoxOutputType->addItem(storedOutput);
    }
    const int outputIndex = ui->comboBoxOutputType->findText(storedOutput);
    ui->comboBoxOutputType->setCurrentIndex(outputIndex == -1 ? 0 : outputIndex);

    const QString speedKey = QStringLiteral("tts/general/speed");
    const int defaultSpeed = 100;
    const int storedSpeed = settings.value(speedKey, defaultSpeed).toInt();
    const int boundedSpeed = std::clamp(storedSpeed,
                                        ui->horizontalSliderSpeed->minimum(),
                                        ui->horizontalSliderSpeed->maximum());
    ui->horizontalSliderSpeed->setValue(boundedSpeed);
    update_speed_label(boundedSpeed);

    connect(ui->comboBoxVoices, &QComboBox::currentTextChanged, this, [this, voiceKey](const QString &value) {
        settings.setValue(voiceKey, value);
        settings.sync();
    });

    connect(ui->comboBoxModels, &QComboBox::currentTextChanged, this, [this, modelKey](const QString &value) {
        settings.setValue(modelKey, value);
        settings.sync();
    });

    connect(ui->comboBoxOutputType, &QComboBox::currentTextChanged, this, [this, outputKey](const QString &value) {
        settings.setValue(outputKey, value);
        settings.sync();
    });

    connect(ui->horizontalSliderSpeed, &QSlider::valueChanged, this, [this, speedKey](int value) {
        update_speed_label(value);
        settings.setValue(speedKey, value);
        settings.sync();
    });

    refresh_output_directory_button();
}

void TextToSpeechWindow::init_openai_settings()
{
    const QString instructionsKey = QStringLiteral("tts/openai/instructions");
    ui->plainTextEdit->setPlainText(settings.value(instructionsKey).toString());

    connect(ui->plainTextEdit, &QPlainTextEdit::textChanged, this, [this, instructionsKey]() {
        settings.setValue(instructionsKey, ui->plainTextEdit->toPlainText());
        settings.sync();
    });
}

void TextToSpeechWindow::init_elevenlabs_settings()
{
    ui->spinBoxStability->setRange(0, 100);
    ui->spinBoxSimilarityBoost->setRange(0, 100);
    ui->spinBoxStyle->setRange(0, 100);

    const QString languageKey = QStringLiteral("tts/elevenlabs/language_code");
    const QString stabilityKey = QStringLiteral("tts/elevenlabs/stability");
    const QString speakerBoostKey = QStringLiteral("tts/elevenlabs/use_speaker_boost");
    const QString similarityKey = QStringLiteral("tts/elevenlabs/similarity_boost");
    const QString styleKey = QStringLiteral("tts/elevenlabs/style");
    const QString improvePrevKey = QStringLiteral("tts/elevenlabs/improve_previous");
    const QString improveNextKey = QStringLiteral("tts/elevenlabs/improve_next");
    const QString textNormalizationKey = QStringLiteral("tts/elevenlabs/text_normalization");
    const QString langNormalizationKey = QStringLiteral("tts/elevenlabs/language_text_normalization");

    ui->lineEditLanguageCode->setText(settings.value(languageKey).toString());
    ui->spinBoxStability->setValue(settings.value(stabilityKey, 50).toInt());
    ui->checkBoxUseSpeakerBoost->setChecked(settings.value(speakerBoostKey, false).toBool());
    ui->spinBoxSimilarityBoost->setValue(settings.value(similarityKey, 50).toInt());
    ui->spinBoxStyle->setValue(settings.value(styleKey, 50).toInt());
    ui->checkBoxImproveByPrevious->setChecked(settings.value(improvePrevKey, false).toBool());
    ui->checkBoxImproveByNext->setChecked(settings.value(improveNextKey, false).toBool());
    ui->checkBoxApplyTextNormalization->setChecked(settings.value(textNormalizationKey, false).toBool());
    ui->checkBoxApplyLanguageTextNormalization->setChecked(settings.value(langNormalizationKey, false).toBool());

    connect(ui->lineEditLanguageCode, &QLineEdit::textChanged, this, [this, languageKey](const QString &value) {
        settings.setValue(languageKey, value);
        settings.sync();
    });

    connect(ui->spinBoxStability, qOverload<int>(&QSpinBox::valueChanged), this, [this, stabilityKey](int value) {
        settings.setValue(stabilityKey, value);
        settings.sync();
    });

    connect(ui->checkBoxUseSpeakerBoost, &QCheckBox::toggled, this, [this, speakerBoostKey](bool checked) {
        settings.setValue(speakerBoostKey, checked);
        settings.sync();
    });

    connect(ui->spinBoxSimilarityBoost, qOverload<int>(&QSpinBox::valueChanged), this, [this, similarityKey](int value) {
        settings.setValue(similarityKey, value);
        settings.sync();
    });

    connect(ui->spinBoxStyle, qOverload<int>(&QSpinBox::valueChanged), this, [this, styleKey](int value) {
        settings.setValue(styleKey, value);
        settings.sync();
    });

    connect(ui->checkBoxImproveByPrevious, &QCheckBox::toggled, this, [this, improvePrevKey](bool checked) {
        settings.setValue(improvePrevKey, checked);
        settings.sync();
    });

    connect(ui->checkBoxImproveByNext, &QCheckBox::toggled, this, [this, improveNextKey](bool checked) {
        settings.setValue(improveNextKey, checked);
        settings.sync();
    });

    connect(ui->checkBoxApplyTextNormalization, &QCheckBox::toggled, this, [this, textNormalizationKey](bool checked) {
        settings.setValue(textNormalizationKey, checked);
        settings.sync();
    });

    connect(ui->checkBoxApplyLanguageTextNormalization, &QCheckBox::toggled, this, [this, langNormalizationKey](bool checked) {
        settings.setValue(langNormalizationKey, checked);
        settings.sync();
    });
}

void TextToSpeechWindow::update_speed_label(int value)
{
    ui->labelSpeedValue->setText(QString::number(value));
}

void TextToSpeechWindow::set_entries(const QVector<Entry> &entries)
{
    ui->textTable->clearContents();
    ui->textTable->setRowCount(entries.size());

    for (int row = 0; row < entries.size(); ++row)
    {
        const Entry &entry = entries.at(row);

        auto createItem = [](const QString &value) {
            auto *item = new QTableWidgetItem(value);
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            return item;
        };

        ui->textTable->setItem(row, 0, createItem(entry.text));
        ui->textTable->setItem(row, 1, createItem(entry.duration));
        ui->textTable->setItem(row, 2, createItem(entry.filePath));

        if (QWidget *existing = ui->textTable->cellWidget(row, 3))
        {
            existing->deleteLater();
        }

        auto *convertButton = new QPushButton(tr("Convert"), ui->textTable);
        ui->textTable->setCellWidget(row, 3, convertButton);
    }
}

void TextToSpeechWindow::refresh_output_directory_button()
{
    QString buttonText = defaultOutputDirButtonText_;
    QString tooltip;
    if (!outputDirectory_.isEmpty())
    {
        const QString nativePath = QDir::toNativeSeparators(outputDirectory_);
        buttonText = nativePath;
        tooltip = nativePath;
    }

    ui->pushButtonOutputDir->setText(buttonText);
    ui->pushButtonOutputDir->setToolTip(tooltip);
}

void TextToSpeechWindow::select_output_directory()
{
    const QString initialDir = outputDirectory_.isEmpty() ? QDir::homePath() : outputDirectory_;
    const QString chosenDir = QFileDialog::getExistingDirectory(this,
                                                               tr("Select Output Folder"),
                                                               initialDir,
                                                               QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (chosenDir.isEmpty())
    {
        return;
    }

    outputDirectory_ = chosenDir;
    refresh_output_directory_button();
}
