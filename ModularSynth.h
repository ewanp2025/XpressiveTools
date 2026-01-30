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
#include <QMenu>
#include <functional>

// Forward declarations
class SynthNode;
class ConnectionPath;
class UniversalScope;

// =========================================================
// 1. CONNECTION PATH (The Wire)
// =========================================================
class ConnectionPath : public QGraphicsPathItem {
public:
    ConnectionPath(QPointF start, QPointF end, QGraphicsItem* parent = nullptr);
    ~ConnectionPath();
    void updatePosition(QPointF start, QPointF end);
    void detach();

    SynthNode* startNode = nullptr;
    SynthNode* endNode = nullptr;
    int inputIndex = -1;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
};

// =========================================================
// 2. SYNTH NODE (The Base Class)
// =========================================================
class SynthNode : public QGraphicsRectItem {
public:
    SynthNode(QString title, int inputs, int outputs, QGraphicsItem* parent = nullptr);
    virtual ~SynthNode();

    enum { Type = UserType + 1 };
    int type() const override { return Type; }

    // Logic: Now takes a 'nightly' flag
    virtual QString getExpression(bool nightly) = 0;
    virtual double evaluate(double t, double freq) = 0;

    QPointF getInputPos(int index);
    QPointF getOutputPos(int index);
    void addInputConnection(int index, ConnectionPath* conn);
    void removeInputConnection(int index);

    ConnectionPath* inputs[8];

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;

    // Base Interaction
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

    QString m_title;
    int m_numInputs;
    int m_numOutputs;

    bool m_isKnobDrag = false;
    QPointF m_lastMousePos;
};

// =========================================================
// 3. MODULES
// =========================================================

class OutputNode : public SynthNode {
public:
    OutputNode();
    QString getExpression(bool nightly) override;
    double evaluate(double t, double freq) override;
};

class OscillatorNode : public SynthNode {
public:
    OscillatorNode();
    QString getExpression(bool nightly) override;
    double evaluate(double t, double freq) override;
    void setWaveform(int index);
    int currentWave = 0;
};

class LFONode : public SynthNode {
public:
    LFONode();
    QString getExpression(bool nightly) override;
    double evaluate(double t, double freq) override;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

    double m_freq = 1.0;
};

class SequencerNode : public SynthNode {
public:
    SequencerNode();
    QString getExpression(bool nightly) override;
    double evaluate(double t, double freq) override;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

    double steps[8];
};

class QuantizerNode : public SynthNode {
public:
    QuantizerNode();
    QString getExpression(bool nightly) override;
    double evaluate(double t, double freq) override;
};

class SampleHoldNode : public SynthNode {
public:
    SampleHoldNode();
    QString getExpression(bool nightly) override;
    double evaluate(double t, double freq) override;
};

class NoiseNode : public SynthNode {
public:
    NoiseNode();
    QString getExpression(bool nightly) override;
    double evaluate(double t, double freq) override;
};

class MathNode : public SynthNode {
public:
    MathNode();
    QString getExpression(bool nightly) override;
    double evaluate(double t, double freq) override;
    void setMode(int mode);
    int currentMode = 0;
};

class WaveFolderNode : public SynthNode {
public:
    WaveFolderNode();
    QString getExpression(bool nightly) override;
    double evaluate(double t, double freq) override;
};

class BitCrushNode : public SynthNode {
public:
    BitCrushNode();
    QString getExpression(bool nightly) override;
    double evaluate(double t, double freq) override;
};

class DelayNode : public SynthNode {
public:
    DelayNode();
    QString getExpression(bool nightly) override;
    double evaluate(double t, double freq) override;
};

class LogicNode : public SynthNode {
public:
    LogicNode();
    QString getExpression(bool nightly) override;
    double evaluate(double t, double freq) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    int logicType = 0;
};

class ClockDivNode : public SynthNode {
public:
    ClockDivNode();
    QString getExpression(bool nightly) override;
    double evaluate(double t, double freq) override;
};

// =========================================================
// 4. SCENE & TAB
// =========================================================

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
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;

private:
    ConnectionPath* m_tempPath = nullptr;
    SynthNode* m_sourceNode = nullptr;
};

class ModularSynthTab : public QWidget {
    Q_OBJECT
public:
    explicit ModularSynthTab(QWidget *parent = nullptr);

signals:
    void expressionGenerated(QString code);
    void startPreview(std::function<double(double)> func);
    void stopPreview();

public slots:
    void generateCode();
    void updateVisuals();
    void createNode(QString type, QPointF pos);

private slots:
    void togglePlay(bool checked);

private:
    ModularScene* m_scene;
    QGraphicsView* m_view;
    UniversalScope* m_scope;
    QPushButton* m_btnPlay;
    QComboBox* m_buildMode; // NEW: The Switch
};

#endif // MODULARSYNTH_H
