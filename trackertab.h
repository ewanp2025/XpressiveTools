#ifndef TRACKERTAB_H
#define TRACKERTAB_H

#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QTextEdit>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QSpinBox>
#include <vector>

struct TrackerEvent {
    int step;
    double freq;
};

struct TrackerInstrument {
    QString w1 = "squarew(t)";
    QString w2 = "saww(t)";
    QString o1 = "W1(integrate(f))";
    double decay = 8.0;
    double volume = 0.8;
};

class TrackerTab : public QWidget {
    Q_OBJECT

public:
    explicit TrackerTab(QWidget *parent = nullptr);

private slots:
    void onInstrumentChanged(int index);
    void saveCurrentInstrument();
    void onGenerateClicked();
    void onLoadDemoClicked();
    void onCellChanged(int row, int column);

private:
    void setupUI();
    double noteToFreq(const QString& noteStr);
    QString parseMacro(QString expr, const QString& w1, const QString& w2);

    TrackerInstrument m_instruments[8];
    int m_currentInstIndex = 0;

    QComboBox* m_cmbInstSelect;
    QTextEdit* m_txtW1;
    QTextEdit* m_txtW2;
    QTextEdit* m_txtO1;
    QSlider* m_sldDecay;
    QSlider* m_sldVolume;
    
    QComboBox* m_cmbSyntax;
    QSpinBox* m_spinBPM;

    QTableWidget* m_grid;
    QTextEdit* m_txtOutput;
    
    bool m_isUpdatingUI = false;
};

#endif // TRACKERTAB_H