#ifndef VECTORPROCESSINGTAB_H
#define VECTORPROCESSINGTAB_H

#include <QWidget>
#include <vector>
#include <QString>

class QComboBox;
class QSlider;
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
    void onExperimentChanged(int index);
    void onNormalizeKernel();
    void updateUI();

private:
    void setupUI();

    // Updated generator signatures
    QString generateIIRResonator(bool legacy);
    QString generateFIRConvolution(bool legacy);
    QString generateModalSynthesis(bool legacy);
    QString generateMorphingKernel(bool legacy);

    SynthEngine* m_ghostSynth = nullptr;
    UniversalScope* m_scope = nullptr;

    QComboBox* m_cmbExperiment;
    QComboBox* m_cmbSyntax;
    QComboBox* m_cmbInputSource;
    QSlider* m_kernelSliders[16];
    QLabel* m_kernelLabels[16];

    QTextEdit* m_txtOutput;
    QPushButton* m_btnGenerate;
    QPushButton* m_btnPlay;
    QPushButton* m_btnNormalize;

    std::vector<double> m_kernel;
};

#endif // VECTORPROCESSINGTAB_H