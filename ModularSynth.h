#ifndef MODULARSYNTH_H
#define MODULARSYNTH_H

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QGraphicsPathItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QGraphicsSceneMouseEvent>
#include <functional> // Needed for audio

// Forward declarations
class SynthNode;
class ConnectionPath;
class UniversalScope; // We assume this exists in your project

// --- 1. The Wire (Connection) ---
class ConnectionPath : public QGraphicsPathItem {
public:
    ConnectionPath(QPointF start, QPointF end, QGraphicsItem* parent = nullptr);
    void updatePosition(QPointF start, QPointF end);
    SynthNode* startNode = nullptr;
    SynthNode* endNode = nullptr;
};

// --- 2. The Node (Base Class) ---
class SynthNode : public QGraphicsRectItem {
public:
    SynthNode(QString title, int inputs, int outputs, QGraphicsItem* parent = nullptr);

    enum { Type = UserType + 1 };
    int type() const override { return Type; }

    // Generates the Text Code (for export)
    virtual QString getExpression() = 0;

    // Generates the Audio Value (for Preview/Scope)
    // t = time, freq = base frequency (e.g. 220Hz)
    virtual double evaluate(double t, double freq) = 0;

    QPointF getInputPos(int index);
    QPointF getOutputPos(int index);
    void addInputConnection(int index, ConnectionPath* conn);

    ConnectionPath* inputs[5]; // Max 5 inputs

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    QString m_title;
    int m_numInputs;
    int m_numOutputs;
};

// --- 3. Specific Node Types ---

class OutputNode : public SynthNode {
public:
    OutputNode();
    QString getExpression() override;
    double evaluate(double t, double freq) override;
};

class OscillatorNode : public SynthNode {
public:
    OscillatorNode();
    QString getExpression() override;
    double evaluate(double t, double freq) override;

    void setWaveform(int index); // 0=Sin, 1=Tri, 2=Saw, 3=Sqr
    int currentWave = 0;
};

// --- 4. The Canvas (Scene) ---
class ModularScene : public QGraphicsScene {
    Q_OBJECT
public:
    ModularScene(QObject* parent = nullptr);
    OutputNode* outputNode;

signals:
    void graphChanged();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:
    ConnectionPath* m_tempPath = nullptr;
    SynthNode* m_sourceNode = nullptr;
};

// --- 5. The Tab Widget (Main Entry) ---
class ModularSynthTab : public QWidget {
    Q_OBJECT
public:
    explicit ModularSynthTab(QWidget *parent = nullptr);

signals:
    void expressionGenerated(QString code);
    // Signals to tell MainWindow to play audio
    void startPreview(std::function<double(double)> func);
    void stopPreview();

public slots:
    void generateCode();

private slots:
    void addOscillator();
    void togglePlay(bool checked);
    void updateVisuals(); // Refreshes the scope

private:
    ModularScene* m_scene;
    QGraphicsView* m_view;
    UniversalScope* m_scope; // The Oscilloscope
    QPushButton* m_btnPlay;
};

#endif // MODULARSYNTH_H
