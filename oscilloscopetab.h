#ifndef OSCILLOSCOPETAB_H
#define OSCILLOSCOPETAB_H

#include <QWidget>
#include <vector>
#include <QString>
#include <QTimer>

class QSlider;
class QSpinBox;
class QDoubleSpinBox;
class QCheckBox;
class QPushButton;
class QComboBox;
class QTextEdit;
class QLineEdit;

class OscilloscopePlot : public QWidget {
    Q_OBJECT
public:
    explicit OscilloscopePlot(QWidget *parent = nullptr);
    void setParameters(int fx, int fy, float phase);
    void setCustomVectors(const std::vector<float>& x, const std::vector<float>& y);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int m_fx = 1;
    int m_fy = 1;
    float m_phase = 0.0f;
    std::vector<float> m_customX;
    std::vector<float> m_customY;
    bool m_useCustom = false;
};

struct Vec3 { float x, y, z; };

class OscilloscopeTab : public QWidget {
    Q_OBJECT
public:
    explicit OscilloscopeTab(QWidget *parent = nullptr);

signals:
    void timbreGenerated(const std::vector<float>& bufferX, const std::vector<float>& bufferY);

private slots:
    void updateParameters();
    //void generateTimbre();
    void loadPreset(int index);
    void generateString();
    void importVectors();
    void importObj();
    void updateRotation();
    void exportWav();
    void updateCsvScale();

private:
    QComboBox* presetCombo;
    QComboBox* waveformCombo;
    QSpinBox* freqXSpin;
    QSpinBox* freqYSpin;
    QSlider* phaseStartSlider;
    QSlider* phaseEndSlider;
    QDoubleSpinBox* timeSpin;
    QCheckBox* pingPongCheck;
    QComboBox* buildModeCombo;

    QLineEdit* sharedExprEdit;
    QCheckBox* mixSharedExprCheck;

    QTextEdit* stringOutput;

    OscilloscopePlot* plotWidget;
    QPushButton* generateStringBtn;
    QPushButton* importBtn;
    QPushButton* importObjBtn;
    QTimer* rotationTimer;
    QPushButton* exportWavBtn;
    QSlider* objectScaleSlider;

    bool m_isCustomMode = false;
    std::vector<float> m_activeX;
    std::vector<float> m_activeY;

    std::vector<Vec3> m_vertices3D;
    std::vector<int> m_drawPath;

    float m_angleX = 0.0f;
    float m_angleY = 0.0f;

    QCheckBox* vibrateCheck;
    QCheckBox* loopSweepCheck;
    QCheckBox* rotateCustomCheck;
    QCheckBox* rotate3dCheck;

    QSlider* csvScaleSlider;
    std::vector<float> m_originalX;
    std::vector<float> m_originalY;

};

#endif // OSCILLOSCOPETAB_H
