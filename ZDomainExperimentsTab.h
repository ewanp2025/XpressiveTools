#ifndef ZDOMAINEXPERIMENTSTAB_H
#define ZDOMAINEXPERIMENTSTAB_H

#include <QWidget>
#include <QTextEdit>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPainterPath>
#include <QMouseEvent>
#include <QPainter>
#include <vector>

class SynthEngine;

class ZDomainCanvas : public QWidget {
    Q_OBJECT
public:
    explicit ZDomainCanvas(QWidget *parent = nullptr);
    
    std::vector<int> getPathSamples(int numSteps);
    void clearPath();

signals:
    void pointerMoved(int delaySamples, QString zoneName, QString dspTheory);
    void pathCompleted();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    int mapYToDelay(int y);
    int mapDelayToY(int delay);
    QString getZoneTheory(int delay);

    QPainterPath m_drawPath;
    bool m_isDrawing = false;
    int m_currentDelayHover = 0;
    QPoint m_pointerPos;
    int m_maxSampleDelay = 44100;
};


class ZDomainExperimentsTab : public QWidget {
    Q_OBJECT

public:
    explicit ZDomainExperimentsTab(SynthEngine* ghostSynth, QWidget *parent = nullptr);
    ~ZDomainExperimentsTab();

private slots:
    void updateDSPReadout(int delaySamples, QString zoneName, QString dspTheory);
    void onGenerateCode();
    void togglePlay(bool checked);
    void onClearCanvas();

private:
    void setupUI();
    QString generateNightlyLogic(const std::vector<int>& delays);
    QString generateLegacyLogic(const std::vector<int>& delays);

    SynthEngine* m_ghostSynth;

    ZDomainCanvas* m_canvas;
    QLabel* m_lblCurrentZone;
    QLabel* m_lblCurrentTheory;
    QLabel* m_lblDelayHover;

    QTextEdit* m_txtBaseO1;
    QTextEdit* m_txtOutputCode;

    QComboBox* m_cmbSyntax;
    QDoubleSpinBox* m_spinDuration;
    QSlider* m_sldFeedback;
    QLabel* m_lblFeedback;


    QCheckBox* m_chkKarplusExciter;
    QCheckBox* m_chkDampen;
    QCheckBox* m_chkDoppler;
    QSlider* m_sldDopplerDepth;
    QSlider* m_sldDopplerRate;

    QPushButton* m_btnGenerate;
    QPushButton* m_btnClear;
    QPushButton* m_btnPlay;
};

#endif // ZDOMAINEXPERIMENTSTAB_H