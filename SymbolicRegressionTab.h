#pragma once
#include <QWidget>
#include <QVector>
#include <QFutureWatcher>
#include <QStringList>
#include <vector>
#include <memory>
#include "synthengine.h"
#include "MicroGP.h"
#include <QSpinBox>

class QVBoxLayout;
class QPushButton;
class QComboBox;
class QCheckBox;
class QTextEdit;
class QLabel;
class QLineEdit;
class UniversalScope;
class QGroupBox;

class SymbolicRegressionTab : public QWidget
{
    Q_OBJECT
public:
    explicit SymbolicRegressionTab(SynthEngine* synth, QWidget* parent = nullptr);

private slots:
    void onLoadWavClicked();
    void onDiscoverClicked();
    void onCopyClicked();
    void onProcessFinished();
    void onSaveLearnedClicked();

private:
    void setupUi();
    bool loadWavToMemory(const QString& path);
    void updateScopePreview(bool useGenerated = false);
    void loadSeedsFromFile();

    QVBoxLayout* m_layout;
    QPushButton* m_btnLoad;
    QLineEdit* m_txtPath;
    QComboBox* m_cmbDownsample;
    QCheckBox* m_chkDechord;
    QComboBox* m_cmbSyntax;

    QSpinBox* m_popSizeSpin;
    QSpinBox* m_genSpin;
    QSpinBox* m_maxDepthSpin;

    QPushButton* m_btnDiscover;
    QLabel* m_lblStatus;

    QTextEdit* m_txtExpression;
    QTextEdit* m_txtDNAOutput;

    QPushButton* m_btnCopy;
    QPushButton* m_btnPlay;

    SynthEngine* m_ghostSynth = nullptr;
    UniversalScope* m_scope = nullptr;

    QFutureWatcher<MicroGP::DiscoveryResult>* m_watcher = nullptr;

    std::vector<double> m_audioData;
    double m_sampleRate = 44100.0;
    std::unique_ptr<GPNode> m_bestTree;


    QGroupBox* m_learnerGroup = nullptr;
    QTextEdit* m_txtLearnerInput = nullptr;
    QPushButton* m_btnSaveLearned = nullptr;

    QStringList m_loadedSeeds;
};
