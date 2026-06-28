#ifndef VECTORPROCESSINGTAB_H
#define VECTORPROCESSINGTAB_H

#include <QWidget>
#include <vector>
#include <QString>

class QComboBox;
class QSlider;
class QSpinBox;
class QPushButton;
class QTextEdit;
class QLabel;
class QGroupBox;
class SynthEngine;
class UniversalScope;

class VectorProcessingTab : public QWidget {
    Q_OBJECT

public:
    explicit VectorProcessingTab(SynthEngine* ghostSynth, QWidget *parent = nullptr);
    ~VectorProcessingTab() override;

private slots:
    void onGenerate();
    void togglePlay(bool checked);
    void onTopologyChanged(int index);
    void onExperimentChanged(int index);
    void onNormalizeKernel();
    void updateUI();

private:
    void setupUI();


    QString generateProcessor(QString baseCode, int experiment, bool isIIR, bool isNightly);

    SynthEngine* m_ghostSynth = nullptr;
    UniversalScope* m_scope = nullptr;


    QSlider* m_kernelSliders[16];
    QLabel* m_kernelLabels[16];
    QPushButton* m_btnNormalize;
    std::vector<double> m_kernel;


    QComboBox* m_cmbInputSource;
    QComboBox* m_cmbExperiment;
    QComboBox* m_cmbTopology;
    QComboBox* m_cmbSyntax;
    QSpinBox*  m_spinTapSpacing;

    QLabel* m_lblCaution;
    QLabel* m_lblIntegrateNote;
    QLabel* m_lblDspNote1;
    QLabel* m_lblDspNote2;

    QTextEdit* m_txtInputO1;
    QTextEdit* m_txtInputO2;
    QTextEdit* m_txtOutputO1;
    QTextEdit* m_txtOutputO2;

    QPushButton* m_btnGenerate;
    QPushButton* m_btnPlay;
};

#endif // VECTORPROCESSINGTAB_H