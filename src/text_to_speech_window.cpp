#include "text_to_speech_window.h"

#include "audio.h"

#include <algorithm>
#include <cmath>
#include <string>

#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QHeaderView>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSlider>
#include <QSpinBox>
#include <QTableWidgetItem>
#include <QTime>
#include <QtGlobal>

TextToSpeechWindow::TextToSpeechWindow(QWidget *parent)
    : QDialog(parent), ui(std::make_unique<Ui::TextToSpeechWindow>())
{
    ui->setupUi(this);
    defaultOutputDirButtonText_ = ui->pushButtonOutputDir->text();

    connect(ui->btnCancle, &QPushButton::clicked, this, &TextToSpeechWindow::close);
    connect(ui->pushButtonOutputDir, &QPushButton::clicked, this, &TextToSpeechWindow::select_output_directory);
    connect(ui->btnConvertAll, &QPushButton::clicked, this, &TextToSpeechWindow::convert_all_rows);

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

    const QString outputDirKey = QStringLiteral("tts/general/output_directory");
    outputDirectory_ = settings.value(outputDirKey).toString();

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

        auto createItem = [](const QString &value, bool editable) {
            auto *item = new QTableWidgetItem(value);
            if (!editable)
            {
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            }
            return item;
        };

        ui->textTable->setItem(row, 0, createItem(entry.text, false));
        ui->textTable->setItem(row, 1, createItem(entry.duration, true));
        ui->textTable->setItem(row, 2, createItem(entry.filePath, false));

        if (QWidget *existing = ui->textTable->cellWidget(row, 3))
        {
            existing->deleteLater();
        }

        auto *convertButton = new QPushButton(tr("Convert"), ui->textTable);
        ui->textTable->setCellWidget(row, 3, convertButton);
        connect(convertButton, &QPushButton::clicked, this, [this, convertButton]() {
            const int rowIndex = row_for_button(convertButton);
            if (rowIndex != -1)
            {
                convert_row(rowIndex);
            }
        });
    }
}

QVector<TextToSpeechWindow::Entry> TextToSpeechWindow::entries() const
{
    QVector<Entry> rows;
    const int rowCount = ui->textTable->rowCount();
    rows.reserve(rowCount);

    for (int row = 0; row < rowCount; ++row)
    {
        Entry entry;

        if (QTableWidgetItem *textItem = ui->textTable->item(row, 0))
        {
            entry.text = textItem->text();
        }
        if (QTableWidgetItem *durationItem = ui->textTable->item(row, 1))
        {
            entry.duration = durationItem->text();
        }
        if (QTableWidgetItem *fileItem = ui->textTable->item(row, 2))
        {
            entry.filePath = fileItem->text();
        }

        rows.push_back(entry);
    }

    return rows;
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
    const QString outputDirKey = QStringLiteral("tts/general/output_directory");
    settings.setValue(outputDirKey, outputDirectory_);
    settings.sync();
    refresh_output_directory_button();
}

bool TextToSpeechWindow::ensure_output_directory_selected()
{
    if (!outputDirectory_.isEmpty())
    {
        return true;
    }

    QMessageBox::warning(this,
                         tr("Output folder required"),
                         tr("Please choose an output folder before converting text to audio."));
    ui->pushButtonOutputDir->setFocus();
    return false;
}

QString TextToSpeechWindow::generate_output_file_path(const QString &text, int row) const
{
    if (outputDirectory_.isEmpty())
    {
        return {};
    }

    QString extension = ui->comboBoxOutputType->currentText().trimmed();
    if (extension.startsWith(QLatin1Char('.')))
    {
        extension.remove(0, 1);
    }
    if (extension.isEmpty())
    {
        extension = QStringLiteral("mp3");
    }
    extension = extension.toLower();

    QString slug = text.simplified();
    slug = slug.left(40);
    slug.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9]+")), QStringLiteral("_"));
    slug = slug.trimmed();
    if (slug.isEmpty())
    {
        slug = QStringLiteral("line_%1").arg(row + 1, 3, 10, QLatin1Char('0'));
    }

    const QString timestamp = QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd_HHmmss"));
    const QString baseName = QStringLiteral("%1_%2").arg(timestamp, slug);

    QDir dir(outputDirectory_);
    QString candidate = dir.filePath(QStringLiteral("%1.%2").arg(baseName, extension));
    int counter = 1;
    while (QFile::exists(candidate))
    {
        candidate = dir.filePath(QStringLiteral("%1_%2.%3").arg(baseName).arg(counter++).arg(extension));
    }

    return candidate;
}

void TextToSpeechWindow::update_table_cell(int row, int column, const QString &value)
{
    if (row < 0 || row >= ui->textTable->rowCount())
    {
        return;
    }

    QTableWidgetItem *item = ui->textTable->item(row, column);
    if (!item)
    {
        item = new QTableWidgetItem();
        ui->textTable->setItem(row, column, item);
    }

    if (column == 1)
    {
        item->setFlags(item->flags() | Qt::ItemIsEditable);
    }
    else
    {
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    }

    item->setText(value);
}

QString TextToSpeechWindow::format_duration(double seconds) const
{
    if (seconds <= 0.0)
    {
        return tr("0.000 s");
    }

    const int totalMs = static_cast<int>(std::round(seconds * 1000.0));
    QTime reference(0, 0);
    const QTime timeValue = reference.addMSecs(totalMs);
    if (timeValue.hour() > 0)
    {
        return timeValue.toString(QStringLiteral("hh:mm:ss.zzz"));
    }
    return timeValue.toString(QStringLiteral("mm:ss.zzz"));
}

int TextToSpeechWindow::row_for_button(const QWidget *button) const
{
    if (!button)
    {
        return -1;
    }

    for (int row = 0; row < ui->textTable->rowCount(); ++row)
    {
        if (ui->textTable->cellWidget(row, 3) == button)
        {
            return row;
        }
    }
    return -1;
}

void TextToSpeechWindow::convert_row(int row, bool warn_if_text_missing)
{
    if (row < 0 || row >= ui->textTable->rowCount())
    {
        return;
    }

    if (!ensure_output_directory_selected())
    {
        return;
    }

    QTableWidgetItem *textItem = ui->textTable->item(row, 0);
    const QString text = textItem ? textItem->text().trimmed() : QString();
    if (text.isEmpty())
    {
        if (warn_if_text_missing)
        {
            QMessageBox::information(this,
                                     tr("Nothing to convert"),
                                     tr("Row %1 does not contain any text.").arg(row + 1));
        }
        return;
    }

    const QString token = settings.value(QStringLiteral("ai/audio/apiKey")).toString().trimmed();
    if (token.isEmpty())
    {
        QMessageBox::warning(this,
                             tr("Missing API key"),
                             tr("Please configure an API key in Settings ▸ Audio before converting."));
        return;
    }

    const QString provider = settings.value(QStringLiteral("ai/audio/provider"), QStringLiteral("ElevenLabs")).toString();
    const QString voice = ui->comboBoxVoices->currentText();
    const QString model = ui->comboBoxModels->currentText();
    const QString filePath = generate_output_file_path(text, row);
    if (filePath.isEmpty())
    {
        return;
    }

    Audio audio;
    const std::string nativeFilePath = QDir::toNativeSeparators(filePath).toStdString();
    const std::string tokenStd = token.toStdString();

    if (provider.compare(QStringLiteral("OpenAI"), Qt::CaseInsensitive) == 0)
    {
        audio.openai_text_to_speech(text, nativeFilePath, voice, model, tokenStd);
    }
    else if (provider.compare(QStringLiteral("ElevenLabs"), Qt::CaseInsensitive) == 0)
    {
        audio.elevenlabs_text_to_speech(text, nativeFilePath, voice, model, tokenStd);
    }
    else
    {
        QMessageBox::warning(this,
                             tr("Unsupported provider"),
                             tr("Audio provider \"%1\" is not supported.").arg(provider));
        return;
    }

    if (!QFile::exists(filePath))
    {
        return;
    }

    update_table_cell(row, 2, QDir::toNativeSeparators(filePath));
    const double durationSeconds = audio.get_audio_duration_seconds(nativeFilePath);
    update_table_cell(row, 1, format_duration(durationSeconds));
}

void TextToSpeechWindow::convert_all_rows()
{
    if (!ensure_output_directory_selected())
    {
        return;
    }

    for (int row = 0; row < ui->textTable->rowCount(); ++row)
    {
        convert_row(row, false);
    }
}
