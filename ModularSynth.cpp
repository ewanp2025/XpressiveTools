#include "ModularSynth.h"
#include "mainwindow.h"
#include <QPainter>
#include <QDebug>
#include <QtMath>
#include <QGraphicsSceneContextMenuEvent>

// =========================================================
// 1. CONNECTION LOGIC
// =========================================================

ConnectionPath::ConnectionPath(QPointF start, QPointF end, QGraphicsItem* parent)
    : QGraphicsPathItem(parent) {
    setPen(QPen(QColor(255, 200, 0, 180), 3));
    setZValue(-1);
    updatePosition(start, end);
}

ConnectionPath::~ConnectionPath() {
    detach();
}

void ConnectionPath::updatePosition(QPointF start, QPointF end) {
    QPainterPath p;
    p.moveTo(start);
    double dx = end.x() - start.x();
    QPointF c1(start.x() + dx * 0.5, start.y());
    QPointF c2(end.x() - dx * 0.5, end.y());
    p.cubicTo(c1, c2, end);
    setPath(p);
}

void ConnectionPath::detach() {
    if (endNode && inputIndex != -1) {
        endNode->removeInputConnection(inputIndex);
        endNode = nullptr;
    }
    if (scene()) scene()->removeItem(this);
}

void ConnectionPath::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if (event->button() == Qt::RightButton) {
        ModularScene* sc = dynamic_cast<ModularScene*>(scene());
        delete this;
        if(sc) emit sc->graphChanged();
    } else {
        QGraphicsPathItem::mousePressEvent(event);
    }
}

// =========================================================
// 2. NODE BASE LOGIC
// =========================================================

SynthNode::SynthNode(QString title, int in, int out, QGraphicsItem* parent)
    : QGraphicsRectItem(0, 0, 100, 50 + (std::max(in, out) * 20)),
    m_title(title), m_numInputs(in), m_numOutputs(out) {

    setFlags(ItemIsMovable | ItemIsSelectable | ItemSendsScenePositionChanges);
    setBrush(QColor(40, 40, 50));
    setPen(QPen(QColor(20, 20, 20), 2));
    for(int i=0; i<8; i++) inputs[i] = nullptr;
}

SynthNode::~SynthNode() {
    for(int i=0; i<m_numInputs; i++) {
        if(inputs[i]) {
            delete inputs[i];
            inputs[i] = nullptr;
        }
    }
    if(scene()) {
        QList<QGraphicsItem*> items = scene()->items();
        for(auto* item : items) {
            if(ConnectionPath* conn = dynamic_cast<ConnectionPath*>(item)) {
                if(conn->startNode == this) delete conn;
            }
        }
    }
}

QPointF SynthNode::getInputPos(int index) { return mapToScene(0, 30 + index * 20); }
QPointF SynthNode::getOutputPos(int index) { return mapToScene(rect().width(), 30 + index * 20); }

void SynthNode::addInputConnection(int index, ConnectionPath* conn) {
    if (inputs[index]) delete inputs[index];
    inputs[index] = conn;
    conn->inputIndex = index;
}

void SynthNode::removeInputConnection(int index) {
    inputs[index] = nullptr;
}

QVariant SynthNode::itemChange(GraphicsItemChange change, const QVariant &value) {
    if (change == ItemScenePositionHasChanged) {
        for (int i=0; i<m_numInputs; i++) {
            if (inputs[i]) inputs[i]->updatePosition(inputs[i]->startNode->getOutputPos(0), getInputPos(i));
        }
        if(scene()) {
            for(auto* item : scene()->items()) {
                if(ConnectionPath* conn = dynamic_cast<ConnectionPath*>(item)) {
                    if(conn->startNode == this) {
                        conn->updatePosition(getOutputPos(0), conn->endNode->getInputPos(conn->inputIndex));
                    }
                }
            }
        }
    }
    return QGraphicsItem::itemChange(change, value);
}

void SynthNode::contextMenuEvent(QGraphicsSceneContextMenuEvent *event) {
    if (m_title == "MASTER OUT") return;

    QMenu menu;
    QAction *delAction = menu.addAction("Delete Module");
    QAction *selected = menu.exec(event->screenPos());
    if (selected == delAction) {
        ModularScene* sc = dynamic_cast<ModularScene*>(scene());
        delete this;
        if(sc) emit sc->graphChanged();
    }
}

// --- BASE INTERACTION ---
void SynthNode::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    m_lastMousePos = event->scenePos();
    // FIX: Do NOT set m_isKnobDrag = true here!
    // Base class only handles moving the node. Subclasses must request knob drag.
    QGraphicsRectItem::mousePressEvent(event);
}
void SynthNode::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    QGraphicsRectItem::mouseMoveEvent(event);
}
void SynthNode::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    m_isKnobDrag = false;
    QGraphicsRectItem::mouseReleaseEvent(event);
}

void SynthNode::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(option); Q_UNUSED(widget);

    QLinearGradient grad(0, 0, 0, rect().height());
    grad.setColorAt(0, brush().color());
    grad.setColorAt(1, brush().color().darker(150));
    painter->setBrush(grad);
    painter->setPen(pen());
    painter->drawRoundedRect(rect(), 5, 5);

    // Header
    painter->setBrush(QColor(30, 30, 35));
    painter->drawRoundedRect(0, 0, rect().width(), 25, 5, 5); // Width matches node
    painter->setPen(Qt::white);
    painter->setFont(QFont("Arial", 8, QFont::Bold));
    painter->drawText(QRectF(0,0, rect().width(), 25), Qt::AlignCenter, m_title);

    // Ports
    for(int i=0; i<m_numInputs; i++) {
        painter->setBrush(inputs[i] ? Qt::yellow : QColor(80, 80, 80));
        painter->setPen(Qt::black);
        painter->drawEllipse(QPointF(0, 30 + i*20), 5, 5);
        if(m_title.startsWith("VCO") && i < 3) {
            painter->setPen(Qt::white);
            QString label = (i==0) ? "FM" : (i==1) ? "AM" : "PWM";
            painter->drawText(QPointF(8, 33 + i*20), label);
        }
    }

    painter->setBrush(Qt::red);
    // Draw output dots on the far right edge
    for(int i=0; i<m_numOutputs; i++)
        painter->drawEllipse(QPointF(rect().width(), 30 + i*20), 5, 5);
}

// =========================================================
// 3. MODULE IMPLEMENTATIONS
// =========================================================

// --- OUTPUT ---
OutputNode::OutputNode() : SynthNode("MASTER OUT", 1, 0) { setBrush(QColor(100, 30, 30)); }
QString OutputNode::getExpression(bool nightly) { return inputs[0] ? inputs[0]->startNode->getExpression(nightly) : "0"; }
double OutputNode::evaluate(double t, double freq) { return inputs[0] ? inputs[0]->startNode->evaluate(t, freq) : 0.0; }

// --- OSCILLATOR ---
OscillatorNode::OscillatorNode() : SynthNode("VCO", 3, 1) {
    setBrush(QColor(40, 80, 100));
    m_title = "VCO: Sine";
}
void OscillatorNode::setWaveform(int index) {
    currentWave = index;
    QString names[] = {"VCO: Sine", "VCO: Tri", "VCO: Saw", "VCO: Sqr", "VCO: PWM"};
    m_title = names[index % 5];
    update();
}
QString OscillatorNode::getExpression(bool nightly) {
    QString funcs[] = {"sinew", "trianglew", "saww", "squarew", "PWM"};
    QString fExpr = "f";
    if (inputs[0]) fExpr = QString("(f + 100 * %1)").arg(inputs[0]->startNode->getExpression(nightly));
    QString am = inputs[1] ? QString("* %1").arg(inputs[1]->startNode->getExpression(nightly)) : "";

    if (currentWave == 4) {
        QString width = "0.5";
        if (inputs[2]) width = QString("clamp(0.05, (1.0 + %1) * 0.5, 0.95)").arg(inputs[2]->startNode->getExpression(nightly));
        return QString("(sgn(mod(t, 1.0/%1) < (%2 / %1)) * 2.0 - 1.0) %3").arg(fExpr).arg(width).arg(am);
    }
    return QString("%1(integrate(%2)) %3").arg(funcs[currentWave]).arg(fExpr).arg(am);
}
double OscillatorNode::evaluate(double t, double freq) {
    double fm = inputs[0] ? inputs[0]->startNode->evaluate(t, freq) * 100.0 : 0.0;
    double am = inputs[1] ? inputs[1]->startNode->evaluate(t, freq) : 1.0;
    double effFreq = freq + fm; if(effFreq < 0.1) effFreq = 0.1;

    if (currentWave == 4) {
        double width = 0.5;
        if(inputs[2]) {
            double in = inputs[2]->startNode->evaluate(t, freq);
            width = (in + 1.0) * 0.5;
            if(width < 0.05) width = 0.05; if(width > 0.95) width = 0.95;
        }
        double period = 1.0 / effFreq;
        double ramp = std::fmod(t, period);
        if(ramp < 0) ramp += period;
        return (ramp < (width * period) ? 1.0 : -1.0) * am;
    }
    double phase = t * effFreq * 6.28318;
    if (currentWave == 0) return std::sin(phase) * am;
    if (currentWave == 1) return (2.0/3.14159)*std::asin(std::sin(phase)) * am;
    if (currentWave == 2) return (2.0*(std::fmod(phase/6.28318, 1.0))-1.0) * am;
    return (std::sin(phase) > 0 ? 1.0 : -1.0) * am;
}

// --- LFO ---
LFONode::LFONode() : SynthNode("LFO", 0, 1) {
    setBrush(QColor(30, 80, 30));
    m_freq = 1.0;
}
void LFONode::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    SynthNode::paint(painter, option, widget);
    painter->setBrush(QColor(20, 20, 20)); painter->setPen(QColor(200, 200, 200));
    painter->drawEllipse(35, 35, 30, 30);
    double ratio = (m_freq - 0.1) / 19.9;
    double angle = -135 + (ratio * 270);
    double rad = qDegreesToRadians(angle);
    painter->setPen(QPen(Qt::white, 2));
    painter->drawLine(QPointF(50, 50), QPointF(50 + 12*std::sin(rad), 50 - 12*std::cos(rad)));
    painter->setFont(QFont("Arial", 7));
    painter->drawText(QRectF(0, 70, 100, 15), Qt::AlignCenter, QString::number(m_freq, 'f', 1) + " Hz");
}
// FIX: Header Click Logic
void LFONode::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if (event->pos().y() < 25) {
        // Clicked header -> Move
        SynthNode::mousePressEvent(event);
        return;
    }
    // Clicked body -> Drag Knob
    m_isKnobDrag = true;
    m_lastMousePos = event->scenePos();
    event->accept();
}
void LFONode::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    if (m_isKnobDrag) {
        double dy = m_lastMousePos.y() - event->scenePos().y();
        m_freq += dy * 0.1;
        if(m_freq < 0.1) m_freq = 0.1; if(m_freq > 20.0) m_freq = 20.0;
        m_lastMousePos = event->scenePos();
        update();
        if(scene()) { ModularScene* sc = dynamic_cast<ModularScene*>(scene()); if(sc) emit sc->graphChanged(); }
        event->accept();
    } else {
        SynthNode::mouseMoveEvent(event);
    }
}
void LFONode::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    m_isKnobDrag = false;
    SynthNode::mouseReleaseEvent(event);
}
QString LFONode::getExpression(bool nightly) { Q_UNUSED(nightly); return QString("sinew(t * %1)").arg(m_freq); }
double LFONode::evaluate(double t, double freq) { return std::sin(t * m_freq * 6.28); }

// --- SEQUENCER ---
SequencerNode::SequencerNode() : SynthNode("SEQ-8", 1, 1) {
    setBrush(QColor(80, 40, 80));
    setRect(0, 0, 160, 100);
    for(int i=0; i<8; i++) steps[i] = 0.5;
}
void SequencerNode::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    SynthNode::paint(painter, option, widget);
    for(int i=0; i<8; i++) {
        double x = 10 + i * 18;
        double h = 60;
        double y = 30;
        painter->setBrush(QColor(20, 20, 20)); painter->setPen(Qt::NoPen);
        painter->drawRect(x, y, 10, h);
        double fillH = steps[i] * h;
        painter->setBrush(QColor(255, 100, 255));
        painter->drawRect(x, y + h - fillH, 10, fillH);
    }
}
// FIX: Header vs Body logic
void SequencerNode::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if (event->pos().y() < 25) {
        SynthNode::mousePressEvent(event);
        return;
    }
    m_isKnobDrag = true;
    event->accept();
    mouseMoveEvent(event);
}
void SequencerNode::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    if (m_isKnobDrag) {
        double x = event->pos().x();
        double y = event->pos().y();
        int idx = (x - 10) / 18;
        if(idx >= 0 && idx < 8) {
            double val = 1.0 - ((y - 30) / 60.0);
            if(val < 0) val = 0; if(val > 1) val = 1;
            steps[idx] = val;
            update();
            if(scene()) { ModularScene* sc = dynamic_cast<ModularScene*>(scene()); if(sc) emit sc->graphChanged(); }
        }
        event->accept();
    } else { SynthNode::mouseMoveEvent(event); }
}
void SequencerNode::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    m_isKnobDrag = false;
    SynthNode::mouseReleaseEvent(event);
}
QString SequencerNode::getExpression(bool nightly) {
    QString clock = inputs[0] ? inputs[0]->startNode->getExpression(nightly) : "t*4";
    QString body = "0";

    if (nightly) {
        // NIGHTLY: Nested Ternary
        for(int i=7; i>=0; --i) {
            body = QString("(step == %1 ? %2 : %3)").arg(i).arg(steps[i]).arg(body);
        }
        return QString("var step := floor(mod(%1, 8));\n%2").arg(clock).arg(body);
    } else {
        // LEGACY: Additive Logic
        for(int i=7; i>=0; --i) {
            body = QString("(floor(mod(%1,8))==%2 ? %3 : %4)").arg(clock).arg(i).arg(steps[i]).arg(body);
        }
        return body;
    }
}
double SequencerNode::evaluate(double t, double freq) {
    double clock = inputs[0] ? inputs[0]->startNode->evaluate(t, freq) : t*4.0;
    int step = (int)std::floor(std::fmod(clock, 8.0));
    if(step < 0) step = 0; if(step > 7) step = 7;
    return steps[step];
}

// --- QUANTIZER ---
QuantizerNode::QuantizerNode() : SynthNode("QUANTIZER", 1, 1) { setBrush(QColor(100, 80, 40)); }
QString QuantizerNode::getExpression(bool nightly) {
    QString in = inputs[0] ? inputs[0]->startNode->getExpression(nightly) : "0";
    return QString("floor(%1 * 12.0) / 12.0").arg(in);
}
double QuantizerNode::evaluate(double t, double freq) {
    double in = inputs[0] ? inputs[0]->startNode->evaluate(t, freq) : 0.0;
    return std::floor(in * 12.0) / 12.0;
}

// --- S&H ---
SampleHoldNode::SampleHoldNode() : SynthNode("S&H", 2, 1) { setBrush(QColor(50, 50, 50)); }
QString SampleHoldNode::getExpression(bool nightly) {
    QString sig = inputs[0] ? inputs[0]->startNode->getExpression(nightly) : "randv(t)";
    QString trig = inputs[1] ? inputs[1]->startNode->getExpression(nightly) : "floor(t*4)";
    return QString("%1").arg(sig).replace("t", QString("(%1)").arg(trig));
}
double SampleHoldNode::evaluate(double t, double freq) {
    double trig = inputs[1] ? inputs[1]->startNode->evaluate(t, freq) : t*4.0;
    double sampleTime = std::floor(trig);
    if(inputs[0]) return inputs[0]->startNode->evaluate(sampleTime, freq);
    return ((int)(sampleTime * 1000) % 100) / 50.0 - 1.0;
}

// --- LOGIC ---
LogicNode::LogicNode() : SynthNode("LOGIC: AND", 2, 1) { setBrush(QColor(100, 40, 100)); }
void LogicNode::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if (event->button() == Qt::LeftButton && event->pos().y() < 25) {
        logicType = (logicType + 1) % 3;
        if(logicType == 0) m_title = "LOGIC: AND";
        else if(logicType == 1) m_title = "LOGIC: OR";
        else m_title = "LOGIC: XOR";
        update();
        if(scene()) { ModularScene* sc = dynamic_cast<ModularScene*>(scene()); if(sc) emit sc->graphChanged(); }
    }
    SynthNode::mousePressEvent(event);
}
QString LogicNode::getExpression(bool nightly) {
    QString a = inputs[0] ? inputs[0]->startNode->getExpression(nightly) : "0";
    QString b = inputs[1] ? inputs[1]->startNode->getExpression(nightly) : "0";
    QString boolA = QString("(%1 > 0.1)").arg(a);
    QString boolB = QString("(%1 > 0.1)").arg(b);
    if (logicType == 0) return QString("(%1 * %2)").arg(boolA, boolB);
    if (logicType == 1) return QString("max(%1, %2)").arg(boolA, boolB);
    return QString("abs(%1 - %2)").arg(boolA, boolB);
}
double LogicNode::evaluate(double t, double freq) {
    double a = inputs[0] ? inputs[0]->startNode->evaluate(t, freq) : 0.0;
    double b = inputs[1] ? inputs[1]->startNode->evaluate(t, freq) : 0.0;
    bool ba = a > 0.1; bool bb = b > 0.1;
    if (logicType == 0) return (ba && bb) ? 1.0 : 0.0;
    if (logicType == 1) return (ba || bb) ? 1.0 : 0.0;
    return (ba != bb) ? 1.0 : 0.0;
}

// --- CLOCK DIVIDER ---
ClockDivNode::ClockDivNode() : SynthNode("CLK DIV", 1, 3) { setBrush(QColor(40, 40, 80)); }
QString ClockDivNode::getExpression(bool nightly) {
    QString clk = inputs[0] ? inputs[0]->startNode->getExpression(nightly) : "t";
    return QString("floor(mod(%1 / 2, 2))").arg(clk);
}
double ClockDivNode::evaluate(double t, double freq) {
    double clk = inputs[0] ? inputs[0]->startNode->evaluate(t, freq) : t;
    return ((int)clk % 2 == 0) ? 1.0 : 0.0;
}

// --- OTHERS ---
NoiseNode::NoiseNode() : SynthNode("NOISE", 1, 1) { setBrush(QColor(80, 80, 80)); }
QString NoiseNode::getExpression(bool nightly) {
    Q_UNUSED(nightly);
    QString rate = inputs[0] ? QString("1000 + 10000 * %1").arg(inputs[0]->startNode->getExpression(nightly)) : "10000";
    return QString("randv(t * %1)").arg(rate);
}
double NoiseNode::evaluate(double t, double freq) { return ((double)rand() / RAND_MAX) * 2.0 - 1.0; }

MathNode::MathNode() : SynthNode("MIX (A+B)", 2, 1) { setBrush(QColor(100, 60, 20)); }
void MathNode::setMode(int mode) {
    currentMode = mode;
    m_title = (mode == 0) ? "MIX (A+B)" : "RING (A*B)";
    update();
}
QString MathNode::getExpression(bool nightly) {
    QString a = inputs[0] ? inputs[0]->startNode->getExpression(nightly) : "0";
    QString b = inputs[1] ? inputs[1]->startNode->getExpression(nightly) : "0";
    return (currentMode == 0) ? QString("(%1 + %2)").arg(a, b) : QString("(%1 * %2)").arg(a, b);
}
double MathNode::evaluate(double t, double freq) {
    double a = inputs[0] ? inputs[0]->startNode->evaluate(t, freq) : 0.0;
    double b = inputs[1] ? inputs[1]->startNode->evaluate(t, freq) : 0.0;
    return (currentMode == 0) ? (a + b) : (a * b);
}

WaveFolderNode::WaveFolderNode() : SynthNode("FOLDER", 1, 1) { setBrush(QColor(100, 20, 100)); }
QString WaveFolderNode::getExpression(bool nightly) {
    QString in = inputs[0] ? inputs[0]->startNode->getExpression(nightly) : "0";
    return QString("sinew(%1 * 5)").arg(in);
}
double WaveFolderNode::evaluate(double t, double freq) {
    double in = inputs[0] ? inputs[0]->startNode->evaluate(t, freq) : 0.0;
    return std::sin(in * 5.0);
}

BitCrushNode::BitCrushNode() : SynthNode("CRUSHER", 2, 1) { setBrush(QColor(60, 20, 20)); }
QString BitCrushNode::getExpression(bool nightly) {
    QString sig = inputs[0] ? inputs[0]->startNode->getExpression(nightly) : "0";
    QString steps = inputs[1] ? QString("4 + 12 * abs(%1)").arg(inputs[1]->startNode->getExpression(nightly)) : "4";
    return QString("floor(%1 * %2) / %2").arg(sig, steps);
}
double BitCrushNode::evaluate(double t, double freq) {
    double sig = inputs[0] ? inputs[0]->startNode->evaluate(t, freq) : 0.0;
    double mod = inputs[1] ? std::abs(inputs[1]->startNode->evaluate(t, freq)) : 0.0;
    double steps = 4.0 + (mod * 12.0);
    return std::floor(sig * steps) / steps;
}

DelayNode::DelayNode() : SynthNode("DELAY", 2, 1) { setBrush(QColor(20, 20, 80)); }
QString DelayNode::getExpression(bool nightly) {
    QString sig = inputs[0] ? inputs[0]->startNode->getExpression(nightly) : "0";
    return QString("(%1 + 0.6 * last(4000))").arg(sig);
}
double DelayNode::evaluate(double t, double freq) { return inputs[0] ? inputs[0]->startNode->evaluate(t, freq) : 0.0; }


// =========================================================
// 4. SCENE
// =========================================================

ModularScene::ModularScene(QObject* parent) : QGraphicsScene(parent) {
    outputNode = new OutputNode();
    addItem(outputNode);
    outputNode->setPos(400, 200);
}

void ModularScene::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    QGraphicsItem* item = itemAt(event->scenePos(), QTransform());
    if (SynthNode* node = dynamic_cast<SynthNode*>(item)) {

        // --- FIX: Dynamic Width Detection for Wire Dragging ---
        // Check if we clicked on the right-most edge (Port Area)
        double width = node->rect().width();
        if (event->scenePos().x() > node->scenePos().x() + width - 30) {
            m_sourceNode = node;
            m_tempPath = new ConnectionPath(node->getOutputPos(0), event->scenePos());
            addItem(m_tempPath);
            return;
        }

        if (event->button() == Qt::LeftButton) {
            if (OscillatorNode* osc = dynamic_cast<OscillatorNode*>(node)) {
                // Check if click is on Left side (Inputs) or body
                if(event->scenePos().x() > node->scenePos().x() + 20) {
                    osc->setWaveform((osc->currentWave + 1) % 5);
                    emit graphChanged();
                }
            }
            else if (MathNode* math = dynamic_cast<MathNode*>(node)) {
                math->setMode((math->currentMode + 1) % 2);
                emit graphChanged();
            }
        }
    }
    QGraphicsScene::mousePressEvent(event);
}

void ModularScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    if (m_tempPath && m_sourceNode) {
        m_tempPath->updatePosition(m_sourceNode->getOutputPos(0), event->scenePos());
    }
    QGraphicsScene::mouseMoveEvent(event);
}

void ModularScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    if (m_tempPath) {
        QGraphicsItem* item = itemAt(event->scenePos(), QTransform());
        if (SynthNode* target = dynamic_cast<SynthNode*>(item)) {
            if (event->scenePos().x() < target->scenePos().x() + 50) {
                int slot = (event->scenePos().y() - target->scenePos().y() - 30) / 20;
                if (slot >= 0 && slot < 8) {
                    m_tempPath->startNode = m_sourceNode;
                    m_tempPath->endNode = target;
                    m_tempPath->updatePosition(m_sourceNode->getOutputPos(0), target->getInputPos(slot));
                    target->addInputConnection(slot, m_tempPath);
                    m_tempPath = nullptr;
                    emit graphChanged();
                    return;
                }
            }
        }
        removeItem(m_tempPath);
        delete m_tempPath;
        m_tempPath = nullptr;
    }
    QGraphicsScene::mouseReleaseEvent(event);
}

void ModularScene::contextMenuEvent(QGraphicsSceneContextMenuEvent *event) {
    QGraphicsItem* item = itemAt(event->scenePos(), QTransform());
    if (item) {
        QGraphicsScene::contextMenuEvent(event);
        emit graphChanged();
        return;
    }

    QMenu menu;
    QAction* addVCO = menu.addAction("Add VCO (Oscillator)");
    QAction* addLFO = menu.addAction("Add LFO (Low Freq)");
    QAction* addNoise = menu.addAction("Add Noise Generator");
    menu.addSeparator();
    QAction* addSeq = menu.addAction("Add 8-Step Sequencer");
    QAction* addQuant = menu.addAction("Add Quantizer (Semitones)");
    QAction* addSH = menu.addAction("Add Sample & Hold");
    QAction* addLogic = menu.addAction("Add Logic (AND/OR/XOR)");
    QAction* addDiv = menu.addAction("Add Clock Divider");
    menu.addSeparator();
    QAction* addMix = menu.addAction("Add Mixer / RingMod");
    QAction* addFold = menu.addAction("Add Wavefolder");
    QAction* addCrush = menu.addAction("Add Bitcrusher");
    QAction* addDelay = menu.addAction("Add Delay Line");

    QAction* selected = menu.exec(event->screenPos());

    ModularSynthTab* tab = qobject_cast<ModularSynthTab*>(views().first()->parentWidget());
    if(tab) {
        if(selected == addVCO) tab->createNode("VCO", event->scenePos());
        if(selected == addLFO) tab->createNode("LFO", event->scenePos());
        if(selected == addNoise) tab->createNode("NOISE", event->scenePos());
        if(selected == addMix) tab->createNode("MIX", event->scenePos());
        if(selected == addFold) tab->createNode("FOLD", event->scenePos());
        if(selected == addCrush) tab->createNode("CRUSH", event->scenePos());
        if(selected == addDelay) tab->createNode("DELAY", event->scenePos());
        if(selected == addSeq) tab->createNode("SEQ", event->scenePos());
        if(selected == addQuant) tab->createNode("QUANT", event->scenePos());
        if(selected == addSH) tab->createNode("S&H", event->scenePos());
        if(selected == addLogic) tab->createNode("LOGIC", event->scenePos());
        if(selected == addDiv) tab->createNode("DIV", event->scenePos());
    }
}

// =========================================================
// 5. TAB WIDGET
// =========================================================

ModularSynthTab::ModularSynthTab(QWidget *parent) : QWidget(parent) {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    m_scope = new UniversalScope();
    m_scope->setMinimumHeight(150);
    mainLayout->addWidget(m_scope);

    QHBoxLayout* tools = new QHBoxLayout();

    // NEW: Build Mode Selector
    m_buildMode = new QComboBox();
    m_buildMode->addItems({"Nightly (Variables)", "Legacy (Inline)"});
    m_buildMode->setFixedWidth(150);

    m_btnPlay = new QPushButton("▶ Play Preview");
    m_btnPlay->setCheckable(true);
    m_btnPlay->setFixedWidth(120);
    m_btnPlay->setStyleSheet("background-color: #335533; color: white; font-weight: bold; height: 30px;");

    QLabel* hint = new QLabel("Right-Click background to add modules!");
    hint->setStyleSheet("color: #AAA; font-style: italic;");

    tools->addWidget(m_btnPlay);
    tools->addWidget(m_buildMode); // Add switch here
    tools->addWidget(hint);
    tools->addStretch();
    mainLayout->addLayout(tools);

    m_scene = new ModularScene(this);
    m_view = new QGraphicsView(m_scene);
    m_view->setRenderHint(QPainter::Antialiasing);
    m_view->setBackgroundBrush(QColor(25, 25, 30));
    mainLayout->addWidget(m_view);

    connect(m_btnPlay, &QPushButton::toggled, this, &ModularSynthTab::togglePlay);
    connect(m_scene, &ModularScene::graphChanged, this, &ModularSynthTab::generateCode);
    connect(m_scene, &ModularScene::graphChanged, this, &ModularSynthTab::updateVisuals);

    // Re-generate if mode changes
    connect(m_buildMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ModularSynthTab::generateCode);
}

void ModularSynthTab::createNode(QString type, QPointF pos) {
    SynthNode* node = nullptr;
    if (type == "VCO") node = new OscillatorNode();
    else if (type == "LFO") node = new LFONode();
    else if (type == "NOISE") node = new NoiseNode();
    else if (type == "MIX") node = new MathNode();
    else if (type == "FOLD") node = new WaveFolderNode();
    else if (type == "CRUSH") node = new BitCrushNode();
    else if (type == "DELAY") node = new DelayNode();
    else if (type == "SEQ") node = new SequencerNode();
    else if (type == "QUANT") node = new QuantizerNode();
    else if (type == "S&H") node = new SampleHoldNode();
    else if (type == "LOGIC") node = new LogicNode();
    else if (type == "DIV") node = new ClockDivNode();

    if (node) {
        m_scene->addItem(node);
        node->setPos(pos);
        updateVisuals();
    }
}

void ModularSynthTab::generateCode() {
    bool nightly = (m_buildMode->currentIndex() == 0);
    QString code = m_scene->outputNode->getExpression(nightly);
    code = QString("clamp(-1, %1, 1)").arg(code);
    emit expressionGenerated(code);
    if(m_btnPlay->isChecked()) togglePlay(true);
}

void ModularSynthTab::updateVisuals() {
    std::function<double(double)> scopeFunc = [=](double t) {
        if (!m_scene || !m_scene->outputNode) return 0.0;
        return m_scene->outputNode->evaluate(t, 220.0);
    };
    m_scope->updateScope(scopeFunc, 0.05, 1.0);
}

void ModularSynthTab::togglePlay(bool checked) {
    if (checked) {
        m_btnPlay->setText("⏹ Stop");
        m_btnPlay->setStyleSheet("background-color: #338833; color: white;");
        std::function<double(double)> audioFunc = [=](double t) {
            if (!m_scene || !m_scene->outputNode) return 0.0;
            return m_scene->outputNode->evaluate(t, 220.0);
        };
        emit startPreview(audioFunc);
    } else {
        m_btnPlay->setText("▶ Play Preview");
        m_btnPlay->setStyleSheet("background-color: #335533; color: white;");
        emit stopPreview();
    }
}
