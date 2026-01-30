#include "ModularSynth.h"
#include "mainwindow.h"
#include <QPainter>
#include <QDebug>
#include <QtMath>
#include <QTimer>

// --- ConnectionPath ---
ConnectionPath::ConnectionPath(QPointF start, QPointF end, QGraphicsItem* parent)
    : QGraphicsPathItem(parent) {
    setPen(QPen(QColor(200, 200, 200), 2));
    setZValue(-1);
    updatePosition(start, end);
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

// --- SynthNode ---
SynthNode::SynthNode(QString title, int in, int out, QGraphicsItem* parent)
    : QGraphicsRectItem(0, 0, 100, 40 + (std::max(in, out) * 20)),
    m_title(title), m_numInputs(in), m_numOutputs(out) {

    setFlags(ItemIsMovable | ItemIsSelectable | ItemSendsScenePositionChanges);
    setBrush(QColor(60, 60, 70));
    setPen(QPen(Qt::black));
    for(int i=0; i<5; i++) inputs[i] = nullptr;
}

QPointF SynthNode::getInputPos(int index) {
    return mapToScene(0, 30 + index * 20);
}

QPointF SynthNode::getOutputPos(int index) {
    return mapToScene(rect().width(), 30 + index * 20);
}

void SynthNode::addInputConnection(int index, ConnectionPath* conn) {
    if (inputs[index]) {
        scene()->removeItem(inputs[index]);
        delete inputs[index];
    }
    inputs[index] = conn;
}

QVariant SynthNode::itemChange(GraphicsItemChange change, const QVariant &value) {
    if (change == ItemScenePositionHasChanged) {
        for (int i=0; i<m_numInputs; i++) {
            if (inputs[i]) inputs[i]->updatePosition(inputs[i]->startNode->getOutputPos(0), getInputPos(i));
        }
    }
    return QGraphicsItem::itemChange(change, value);
}

void SynthNode::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(option); Q_UNUSED(widget);

    // Draw Node Body
    painter->setBrush(brush());
    painter->setPen(QPen(QColor(30, 30, 30), 2));
    painter->drawRoundedRect(rect(), 4, 4);

    // Header
    painter->setBrush(QColor(40, 40, 50));
    painter->drawRoundedRect(0, 0, 100, 20, 4, 4);
    painter->setPen(Qt::white);
    painter->drawText(QRectF(0,0,100,20), Qt::AlignCenter, m_title);

    // Ports
    painter->setBrush(Qt::green);
    painter->setPen(Qt::black);
    for(int i=0; i<m_numInputs; i++) painter->drawEllipse(QPointF(0, 30 + i*20), 4, 4);

    painter->setBrush(Qt::red);
    for(int i=0; i<m_numOutputs; i++) painter->drawEllipse(QPointF(100, 30 + i*20), 4, 4);
}

// --- Specific Nodes ---

OutputNode::OutputNode() : SynthNode("MASTER OUT", 1, 0) {
    setBrush(QColor(50, 20, 20)); // Dark Red
}

QString OutputNode::getExpression() {
    if (!inputs[0]) return "0";
    return inputs[0]->startNode->getExpression();
}

double OutputNode::evaluate(double t, double freq) {
    if (!inputs[0]) return 0.0;
    return inputs[0]->startNode->evaluate(t, freq);
}

OscillatorNode::OscillatorNode() : SynthNode("OSC", 2, 1) {
    setBrush(QColor(20, 50, 50)); // Dark Cyan
    m_title = "Sine";
}

QString OscillatorNode::getExpression() {
    QString func = "sinew";
    if (currentWave == 1) func = "trianglew";
    else if (currentWave == 2) func = "saww";
    else if (currentWave == 3) func = "squarew";

    QString freq = "f";
    // FM Input (0)
    if (inputs[0]) freq = QString("(f + 100 * %1)").arg(inputs[0]->startNode->getExpression());

    return QString("%1(integrate(%2))").arg(func).arg(freq);
}

double OscillatorNode::evaluate(double t, double freq) {
    double mod = 0.0;
    // Input 0 = Frequency Mod
    if(inputs[0]) mod = inputs[0]->startNode->evaluate(t, freq) * 100.0;

    // Use Phase Modulation approximation for stateless preview (simulates FM)
    // phase = t * (freq + mod) * 2PI
    double phase = t * (freq + mod) * 6.283185307;
    double val = 0.0;

    // Sine
    if (currentWave == 0) val = std::sin(phase);
    // Tri
    else if (currentWave == 1) val = (2.0/3.14159) * std::asin(std::sin(phase));
    // Saw
    else if (currentWave == 2) val = 2.0 * (std::fmod(phase / 6.283185, 1.0)) - 1.0;
    // Square
    else if (currentWave == 3) val = std::sin(phase) > 0 ? 1.0 : -1.0;

    return val;
}

void OscillatorNode::setWaveform(int index) {
    currentWave = index;
    if(index == 0) m_title = "Sine";
    else if(index == 1) m_title = "Tri";
    else if(index == 2) m_title = "Saw";
    else if(index == 3) m_title = "Square";
    update();
}

// --- ModularScene ---

ModularScene::ModularScene(QObject* parent) : QGraphicsScene(parent) {
    outputNode = new OutputNode();
    addItem(outputNode);
    outputNode->setPos(400, 200);
}

void ModularScene::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    QGraphicsItem* item = itemAt(event->scenePos(), QTransform());

    if (SynthNode* node = dynamic_cast<SynthNode*>(item)) {
        // Right side click -> Drag wire
        if (event->scenePos().x() > node->scenePos().x() + 50) {
            m_sourceNode = node;
            m_tempPath = new ConnectionPath(node->getOutputPos(0), event->scenePos());
            addItem(m_tempPath);
            return;
        }
        // Left Click on Node Body -> Change Waveform (if Oscillator)
        if (OscillatorNode* osc = dynamic_cast<OscillatorNode*>(node)) {
            osc->setWaveform((osc->currentWave + 1) % 4);
            emit graphChanged();
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
            // Drop on Left side -> Connect
            if (event->scenePos().x() < target->scenePos().x() + 50) {
                int slot = (event->scenePos().y() - target->scenePos().y() - 30) / 20;
                if (slot < 0) slot = 0;
                if (slot > 1) slot = 1;

                m_tempPath->startNode = m_sourceNode;
                m_tempPath->endNode = target;
                m_tempPath->updatePosition(m_sourceNode->getOutputPos(0), target->getInputPos(slot));
                target->addInputConnection(slot, m_tempPath);
                m_tempPath = nullptr;

                emit graphChanged(); // Trigger Updates
                return;
            }
        }
        removeItem(m_tempPath);
        delete m_tempPath;
        m_tempPath = nullptr;
    }
    QGraphicsScene::mouseReleaseEvent(event);
}

// --- ModularSynthTab ---

ModularSynthTab::ModularSynthTab(QWidget *parent) : QWidget(parent) {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // 1. ADD OSCILLOSCOPE (Top)
    m_scope = new UniversalScope();
    m_scope->setMinimumHeight(150);
    mainLayout->addWidget(m_scope);

    // 2. TOOLBAR
    QHBoxLayout* tools = new QHBoxLayout();

    m_btnPlay = new QPushButton("▶ Play Preview");
    m_btnPlay->setCheckable(true);
    m_btnPlay->setFixedWidth(120);
    m_btnPlay->setStyleSheet("background-color: #335533; color: white; font-weight: bold; height: 30px;");

    QPushButton* btnAdd = new QPushButton("Add Oscillator Node");
    btnAdd->setFixedWidth(150);

    tools->addWidget(m_btnPlay);
    tools->addWidget(btnAdd);
    tools->addStretch();
    mainLayout->addLayout(tools);

    // 3. GRAPH VIEW
    m_scene = new ModularScene(this);
    m_view = new QGraphicsView(m_scene);
    m_view->setRenderHint(QPainter::Antialiasing);
    m_view->setBackgroundBrush(QColor(25, 25, 30));
    mainLayout->addWidget(m_view);

    // Connections
    connect(btnAdd, &QPushButton::clicked, this, &ModularSynthTab::addOscillator);
    connect(m_btnPlay, &QPushButton::toggled, this, &ModularSynthTab::togglePlay);
    connect(m_scene, &ModularScene::graphChanged, this, &ModularSynthTab::generateCode);
    connect(m_scene, &ModularScene::graphChanged, this, &ModularSynthTab::updateVisuals); // Update scope on change

    // Timer to update scope animation if play is off?
    // For now we just update once on change.
}

void ModularSynthTab::addOscillator() {
    OscillatorNode* osc = new OscillatorNode();
    m_scene->addItem(osc);
    osc->setPos(50, 50);
    updateVisuals();
}

void ModularSynthTab::generateCode() {
    QString code = m_scene->outputNode->getExpression();
    code = QString("clamp(-1, %1, 1)").arg(code);
    emit expressionGenerated(code);

    // If playing, update the sound live!
    if(m_btnPlay->isChecked()) {
        togglePlay(true);
    }
}

void ModularSynthTab::updateVisuals() {
    // Generate the math function for the scope
    std::function<double(double)> scopeFunc = [=](double t) {
        if (!m_scene || !m_scene->outputNode) return 0.0;
        return m_scene->outputNode->evaluate(t, 220.0); // Show A3
    };
    m_scope->updateScope(scopeFunc, 0.05, 1.0);
}

void ModularSynthTab::togglePlay(bool checked) {
    if (checked) {
        m_btnPlay->setText("⏹ Stop");
        m_btnPlay->setStyleSheet("background-color: #338833; color: white; font-weight: bold; height: 30px;");

        // Generate the audio function
        std::function<double(double)> audioFunc = [=](double t) {
            if (!m_scene || !m_scene->outputNode) return 0.0;
            return m_scene->outputNode->evaluate(t, 220.0); // Play A3
        };

        emit startPreview(audioFunc);
    } else {
        m_btnPlay->setText("▶ Play Preview");
        m_btnPlay->setStyleSheet("background-color: #335533; color: white; font-weight: bold; height: 30px;");
        emit stopPreview();
    }
}
