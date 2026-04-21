#pragma once
#include <QWidget>
#include <QVector>
#include <QFutureWatcher>
#include <vector>
#include <memory>

#include "MicroGP.h"

class QVBoxLayout;
class QPushButton;
class QComboBox;
class QCheckBox;
class QTextEdit;
class QLabel;
class QLineEdit;
class UniversalScope;

class SymbolicRegressionTab : public QWidget
{
    Q_OBJECT
public:
    explicit SymbolicRegressionTab(QWidget* parent = nullptr);

private slots:
    void onLoadWavClicked();
    void onDiscoverClicked();
    void onCopyClicked();
    void onProcessFinished();

private:
    void setupUi();
    bool loadWavToMemory(const QString& path);
    void updateScopePreview(bool useGenerated = false);

    QVBoxLayout* m_layout;
    QPushButton* m_btnLoad;
    QLineEdit* m_txtPath;
    QComboBox* m_cmbDownsample;
    QCheckBox* m_chkDechord;
    QComboBox* m_cmbSyntax;
    QPushButton* m_btnDiscover;
    QLabel* m_lblStatus;
    QTextEdit* m_txtExpression;
    QPushButton* m_btnCopy;

    UniversalScope* m_scope = nullptr;

    QFutureWatcher<QString>* m_watcher = nullptr;

    std::vector<double> m_audioData;
    double m_sampleRate = 44100.0;
    std::unique_ptr<GPNode> m_bestTree;
};
