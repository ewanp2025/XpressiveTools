#ifndef OSCILLOSCOPETAB_H
#define OSCILLOSCOPETAB_H

#include <QWidget>
#include <vector>
#include <QString>

class QSlider;
class QSpinBox;
class QDoubleSpinBox;
class QCheckBox;
class QPushButton;
class QComboBox;
class QTextEdit;

class OscilloscopePlot : public QWidget {
    Q_OBJECT
public:
    explicit OscilloscopePlot(QWidget *parent = nullptr);
    void setParameters(int fx, int fy, float phase);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int m_fx = 1;
    int m_fy = 1;
    float m_phase = 0.0f;
};

class OscilloscopeTab : public QWidget {
    Q_OBJECT
public:
    explicit OscilloscopeTab(QWidget *parent = nullptr);

signals:
    void timbreGenerated(const std::vector<float>& bufferX, const std::vector<float>& bufferY);

private slots:
    void updateParameters();
    void generateTimbre();
    void loadPreset(int index);
    void generateString();

private:
    QComboBox* presetCombo;
    QComboBox* waveformCombo;
    QSpinBox* freqXSpin;
    QSpinBox* freqYSpin;
    QSlider* phaseStartSlider;
    QSlider* phaseEndSlider;
    QDoubleSpinBox* timeSpin;
    QCheckBox* pingPongCheck;

    QTextEdit* stringOutput;

    OscilloscopePlot* plotWidget;
    QPushButton* generateBtn;
    QPushButton* generateStringBtn;
};

#endif // OSCILLOSCOPETAB_H
