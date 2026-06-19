#ifndef SONGEXTRACTORTAB_H
#define SONGEXTRACTORTAB_H

#include <QWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QTableWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <vector>
#include <QString>
#include <set>

struct ExtractedTrack {
    int id;
    QString rawExpression;
    QString cleanedCore;
    std::set<int> activeSteps;
    int sequenceLength;
};

class SongExtractorTab : public QWidget {
    Q_OBJECT

public:
    explicit SongExtractorTab(QWidget *parent = nullptr);
    ~SongExtractorTab() override;

private slots:
    void onAutoSplitClicked();
    void onAnalyzeClicked();
    void onExportClicked();

private:
    void setupUI();
    QString cleanCore(QString expr);
    QString stripOuterParens(QString expr);
    std::vector<QString> splitAtRoot(const QString& expr, QChar delimiter);


    bool evaluateStepLogic(QString logicStr, int s);
    bool evalBooleanExpr(QString expr);

    void writeMmpFile(const QString& filePath);

    QLineEdit* m_inW1;
    QLineEdit* m_inW2;
    QLineEdit* m_inW3;
    QTextEdit* m_inO1;

    QPushButton* m_btnAutoSplit;
    QPushButton* m_btnAnalyze;
    QPushButton* m_btnExport;
    QSpinBox* m_spinBpm;
    QTableWidget* m_trackTable;
    QLabel* m_lblStatus;


    std::vector<ExtractedTrack> m_extractedTracks;
    int m_masterSequenceLength = 16;
};

#endif // SONGEXTRACTORTAB_H