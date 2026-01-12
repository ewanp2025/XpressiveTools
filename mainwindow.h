#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QSlider>
#include <QTextEdit>
#include <QTabWidget>
#include <vector>

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
private slots:
    void loadWav();
    void saveExpr();
    void convertTxt(); // This was the missing line causing your error
private:
    void setupUI();
    QString generateLegacyExpression(const std::vector<double>& q, double sr);
    QString generateModernExpression(const std::vector<double>& q, double sr);

    QTabWidget *modeTabs;
    QDoubleSpinBox *basePitchSpin;
    QDoubleSpinBox *maxDurSpin;
    QComboBox *sampleRateCombo;
    QCheckBox *normalizeCheck;
    QSlider *dcSlider;
    QDoubleSpinBox *dcSpin;
    QTextEdit *statusBox;
    QPushButton *btnSave;

    std::vector<double> originalData;
    uint32_t fileFs = 44100;
    QString currentFileName;
};
#endif
