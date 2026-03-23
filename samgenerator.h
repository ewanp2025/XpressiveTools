#ifndef SAMGENERATOR_H
#define SAMGENERATOR_H

#include <QWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QSlider>
#include <QMap>
#include <QStringList>
// SAM V2 NOT USED

enum PhonemeClass {
    CLASS_VOWEL,
    CLASS_VOICED_CONS,
    CLASS_UNVOICED_CONS,
    CLASS_STOP_CONS,
    CLASS_PUNCT,
    CLASS_UNKNOWN
};

struct SAMPhoneme {
    QString name;
    PhonemeClass pClass;
    int f1, f2, f3;
    bool voiced;
    int a1 = 15;
    int a2 = 10;
    int a3 = 5;
    int length = 10;
};


struct SequenceNode {
    SAMPhoneme data;
    double duration;
    int stress;
};

class SamGeneratorTab : public QWidget {
    Q_OBJECT

public:
    explicit SamGeneratorTab(QWidget *parent = nullptr);

private slots:
    void translateEnglishToPhonemes();
    void generateXpressiveMath();
    void updateLabels();

private:
    QLineEdit *englishInput;
    QPushButton *translateBtn;

    QTextEdit *phonemeInput;
    QPushButton *generateBtn;
    QComboBox *parserModeCombo;
    QCheckBox *nightlyCheckBox;

    QSlider *mouthSlider;
    QLabel *mouthLabel;
    QSlider *throatSlider;
    QLabel *throatLabel;

    QTextEdit *w1Output;
    QTextEdit *o1Output;

    QMap<QString, SAMPhoneme> samLibrary;
    QMap<QString, QString> englishDictionary;

    void setupUI();
    void initSamLibrary();
    void initEnglishDictionary();

    QString textToPhonemes(const QString& englishText);
    QString parseAndGenerateString(const QStringList& inputTokens, bool nightly, bool lofi);
};

#endif // SAMGENERATOR_H
