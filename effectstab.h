#ifndef EFFECTSTAB_H
#define EFFECTSTAB_H

#include <QWidget>
#include <QTextEdit>
#include <QComboBox>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>

class EffectsTab : public QWidget {
    Q_OBJECT

public:
    explicit EffectsTab(QWidget *parent = nullptr);

private slots:
    void generateExpression();
    void updateLabels();

private:
    QTextEdit *inputExpression;
    QComboBox *effectSelector;

    QLabel *param1Label;
    QSlider *param1Slider;

    QLabel *param2Label;
    QSlider *param2Slider;

    QLabel *param3Label;
    QSlider *param3Slider;

    QCheckBox *nightlyCheckBox;
    QPushButton *generateButton;
    QTextEdit *outputExpression;

    void setupUI();
};

#endif // EFFECTSTAB_H
