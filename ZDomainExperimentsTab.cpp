#include "ZDomainExperimentsTab.h"
#include "synthengine.h"
#include <QFormLayout>
#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <cmath>


ZDomainCanvas::ZDomainCanvas(QWidget *parent) : QWidget(parent) {
    setMinimumHeight(350);
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);
}

int ZDomainCanvas::mapYToDelay(int y) {

    double normalized = 1.0 - (double)std::clamp(y, 0, height()) / height();

    double delay = std::pow(normalized, 3.0) * m_maxSampleDelay;
    return std::max(1, (int)delay);
}

int ZDomainCanvas::mapDelayToY(int delay) {
    double normalized = std::cbrt((double)delay / m_maxSampleDelay);
    return height() - (int)(normalized * height());
}

QString ZDomainCanvas::getZoneTheory(int delay) {
    if (delay < 10) return "FIR & Phase Eq: Creates sharp phase cancellation and comb filtering.";
    if (delay < 200) return "Comb & Metallic Resonators: Introduces robotic, metallic, or vocal formant resonances.";
    if (delay < 1500) return "Physical Modeling (Karplus): Tunes the feedback loop to specific acoustic pitches.";
    if (delay < 4000) return "Haas Effect: Creates psychoacoustic spatial widening without distinct echoes.";
    return "Time-Domain Echo: Generates distinct bucket-brigade and rhythmic delays.";
}

void ZDomainCanvas::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_isDrawing = true;
        m_drawPath.clear();
        m_drawPath.moveTo(event->pos());
        update();
    }
}

void ZDomainCanvas::mouseMoveEvent(QMouseEvent *event) {
    m_pointerPos = event->pos();
    m_currentDelayHover = mapYToDelay(event->y());
    
    QString zoneName;
    if (m_currentDelayHover < 10) zoneName = "[ ZONE 1: FIR FILTERING ]";
    else if (m_currentDelayHover < 200) zoneName = "[ ZONE 2: METALLIC COMB ]";
    else if (m_currentDelayHover < 1500) zoneName = "[ ZONE 3: KARPLUS-STRONG ]";
    else if (m_currentDelayHover < 4000) zoneName = "[ ZONE 4: HAAS SPREAD ]";
    else zoneName = "[ ZONE 5: MACRO ECHO ]";

    emit pointerMoved(m_currentDelayHover, zoneName, getZoneTheory(m_currentDelayHover));

    if (m_isDrawing) {
        m_drawPath.lineTo(event->pos());
    }
    update();
}

void ZDomainCanvas::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_isDrawing = false;
        emit pathCompleted();
        update();
    }
}

std::vector<int> ZDomainCanvas::getPathSamples(int numSteps) {
    std::vector<int> samples;
    if (m_drawPath.isEmpty()) {
        samples.push_back(4410);
        return samples;
    }

    double wStep = (double)width() / numSteps;
    for (int i = 0; i < numSteps; ++i) {
        double targetX = i * wStep;
        

        double foundY = height() / 2.0; 
        for (int p = 1; p <= 100; p++) {
            double percent = p / 100.0;
            QPointF pt = m_drawPath.pointAtPercent(percent);
            if (pt.x() >= targetX) {
                foundY = pt.y();
                break;
            }
        }
        samples.push_back(mapYToDelay(foundY));
    }
    return samples;
}

void ZDomainCanvas::clearPath() {
    m_drawPath.clear();
    update();
}

void ZDomainCanvas::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);


    QRect rect = this->rect();
    painter.fillRect(rect, QColor(10, 10, 15));
    
    int y10 = mapDelayToY(10);
    int y200 = mapDelayToY(200);
    int y1500 = mapDelayToY(1500);
    int y4000 = mapDelayToY(4000);

    painter.fillRect(0, 0, width(), y4000, QColor(0, 50, 100, 30));
    painter.fillRect(0, y4000, width(), y1500-y4000, QColor(0, 100, 100, 30));
    painter.fillRect(0, y1500, width(), y200-y1500, QColor(100, 50, 100, 30));
    painter.fillRect(0, y200, width(), y10-y200, QColor(100, 100, 0, 30));
    painter.fillRect(0, y10, width(), height()-y10, QColor(150, 0, 0, 30));


    painter.setPen(QPen(QColor(50, 50, 50), 1, Qt::DashLine));
    for (int i = 1; i <= 10; ++i) {
        int x = i * (width() / 10.0);
        painter.drawLine(x, 0, x, height());
    }


    if (!m_drawPath.isEmpty()) {
        painter.setPen(QPen(QColor(0, 255, 255), 3));
        painter.drawPath(m_drawPath);
    }


    painter.setPen(QPen(QColor(255, 255, 255, 100), 1));
    painter.drawLine(0, m_pointerPos.y(), width(), m_pointerPos.y());
    painter.drawLine(m_pointerPos.x(), 0, m_pointerPos.x(), height());
    

    painter.setPen(QColor(0, 255, 255));
    painter.drawText(5, m_pointerPos.y() - 5, QString("last(%1)").arg(m_currentDelayHover));
}


ZDomainExperimentsTab::ZDomainExperimentsTab(SynthEngine* ghostSynth, QWidget *parent)
    : QWidget(parent), m_ghostSynth(ghostSynth) {
    setupUI();
}

ZDomainExperimentsTab::~ZDomainExperimentsTab() {}

void ZDomainExperimentsTab::setupUI() {
    this->setStyleSheet(R"(
        QWidget { background-color: #0a0a0a; color: #00ffcc; font-family: "Consolas", monospace; }
        QGroupBox { border: 1px solid #005555; font-weight: bold; margin-top: 10px; }
        QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 3px; color: #00ffff; }
        QTextEdit { background-color: #111111; border: 1px solid #00aaaa; color: #ffffff; }
        QPushButton { background-color: #004444; border: 1px solid #00ffcc; padding: 6px; font-weight: bold; }
        QPushButton:hover { background-color: #006666; }
        QPushButton:checked { background-color: #cc0000; color: #ffffff; border: 1px solid #ff0000; }
    )");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    QHBoxLayout* topLayout = new QHBoxLayout();


    QGroupBox* grpControls = new QGroupBox("1. Setup & Expression");
    QVBoxLayout* controlLayout = new QVBoxLayout(grpControls);

    m_cmbSyntax = new QComboBox();
    m_cmbSyntax->addItems({"Nightly (Variables & Math Strings)", "Legacy (Ternary Logic)"});
    controlLayout->addWidget(new QLabel("Syntax Compiler Target:"));
    controlLayout->addWidget(m_cmbSyntax);

    controlLayout->addWidget(new QLabel("Base Geometric Expression (Dry):"));
    m_txtBaseO1 = new QTextEdit("squarew(integrate(f))");
    m_txtBaseO1->setMaximumHeight(40);
    controlLayout->addWidget(m_txtBaseO1);

    QHBoxLayout* seqLayout = new QHBoxLayout();
    m_spinDuration = new QDoubleSpinBox();
    m_spinDuration->setRange(0.1, 10.0);
    m_spinDuration->setValue(1.0);
    m_spinDuration->setSingleStep(0.1);
    seqLayout->addWidget(new QLabel("Sequence Duration (s):"));
    seqLayout->addWidget(m_spinDuration);
    controlLayout->addLayout(seqLayout);

    QHBoxLayout* fbLayout = new QHBoxLayout();
    m_sldFeedback = new QSlider(Qt::Horizontal);
    m_sldFeedback->setRange(0, 99);
    m_sldFeedback->setValue(70);
    m_lblFeedback = new QLabel("Feedback: 70%");
    connect(m_sldFeedback, &QSlider::valueChanged, [=](int v){ m_lblFeedback->setText(QString("Feedback: %1%").arg(v)); });
    fbLayout->addWidget(m_lblFeedback);
    fbLayout->addWidget(m_sldFeedback);
    controlLayout->addLayout(fbLayout);


    QGroupBox* grpExp = new QGroupBox("Experimental Z-Domain Topology");
    QVBoxLayout* expLayout = new QVBoxLayout(grpExp);
    
    m_chkKarplusExciter = new QCheckBox("Karplus-Strong Exciter (Pluck Injector)");
    m_chkDampen = new QCheckBox("Recursive IIR Dampening (Analog Bucket-Brigade)");
    m_chkDoppler = new QCheckBox("Doppler LFO Index Modulation (Chorus/Flutter)");
    
    expLayout->addWidget(m_chkKarplusExciter);
    expLayout->addWidget(m_chkDampen);
    expLayout->addWidget(m_chkDoppler);
    
    m_sldDopplerDepth = new QSlider(Qt::Horizontal);
    m_sldDopplerDepth->setRange(1, 100);
    m_sldDopplerDepth->setValue(10);
    expLayout->addWidget(new QLabel("Doppler Depth (Samples):"));
    expLayout->addWidget(m_sldDopplerDepth);
    
    controlLayout->addWidget(grpExp);
    controlLayout->addStretch();
    topLayout->addWidget(grpControls, 1);

    QGroupBox* grpCanvas = new QGroupBox("2. Draw Z-Domain Timeline (X=Time, Y=Delay)");
    QVBoxLayout* canvasLayout = new QVBoxLayout(grpCanvas);
    
    m_canvas = new ZDomainCanvas();
    canvasLayout->addWidget(m_canvas, 1);
    
    QHBoxLayout* readoutLayout = new QHBoxLayout();
    m_lblCurrentZone = new QLabel("[ ZONE: NONE ]");
    m_lblCurrentTheory = new QLabel("Hover over the canvas to explore Z-Domain acoustic theory.");
    m_lblCurrentZone->setStyleSheet("color: #ffaa00; font-weight: bold;");
    readoutLayout->addWidget(m_lblCurrentZone);
    readoutLayout->addWidget(m_lblCurrentTheory, 1);
    canvasLayout->addLayout(readoutLayout);
    
    topLayout->addWidget(grpCanvas, 2);
    mainLayout->addLayout(topLayout, 2);


    QGroupBox* grpOutput = new QGroupBox("3. Compiled Z-Domain Math String");
    QVBoxLayout* outLayout = new QVBoxLayout(grpOutput);
    
    m_txtOutputCode = new QTextEdit();
    m_txtOutputCode->setMinimumHeight(100);
    outLayout->addWidget(m_txtOutputCode);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    m_btnGenerate = new QPushButton("⚙ Generate Sequence");
    m_btnClear = new QPushButton("🗑 Clear Timeline");
    m_btnPlay = new QPushButton("▶ Test Math Output");
    m_btnPlay->setCheckable(true);
    
    btnLayout->addWidget(m_btnGenerate);
    btnLayout->addWidget(m_btnClear);
    btnLayout->addWidget(m_btnPlay);
    outLayout->addLayout(btnLayout);

    mainLayout->addWidget(grpOutput, 1);


    connect(m_canvas, &ZDomainCanvas::pointerMoved, this, &ZDomainExperimentsTab::updateDSPReadout);
    connect(m_canvas, &ZDomainCanvas::pathCompleted, this, &ZDomainExperimentsTab::onGenerateCode);
    connect(m_btnGenerate, &QPushButton::clicked, this, &ZDomainExperimentsTab::onGenerateCode);
    connect(m_btnClear, &QPushButton::clicked, this, &ZDomainExperimentsTab::onClearCanvas);
    connect(m_btnPlay, &QPushButton::toggled, this, &ZDomainExperimentsTab::togglePlay);
}

void ZDomainExperimentsTab::updateDSPReadout(int delaySamples, QString zoneName, QString dspTheory) {
    m_lblCurrentZone->setText(zoneName);
    m_lblCurrentTheory->setText(QString("N=%1 | %2").arg(delaySamples).arg(dspTheory));
}

void ZDomainExperimentsTab::onClearCanvas() {
    m_canvas->clearPath();
    m_txtOutputCode->clear();
}

void ZDomainExperimentsTab::onGenerateCode() {
    int numSteps = 16; 
    std::vector<int> pathDelays = m_canvas->getPathSamples(numSteps);
    
    QString finalMath;
    if (m_cmbSyntax->currentIndex() == 0) {
        finalMath = generateNightlyLogic(pathDelays);
    } else {
        finalMath = generateLegacyLogic(pathDelays);
    }

    m_txtOutputCode->setText(finalMath);
    QApplication::clipboard()->setText(finalMath);
}

QString ZDomainExperimentsTab::generateNightlyLogic(const std::vector<int>& delays) {
    double duration = m_spinDuration->value();
    double stepTime = duration / delays.size();
    
    QString baseExpr = m_txtBaseO1->toPlainText();
    double fb = m_sldFeedback->value() / 100.0;
    

    if (m_chkKarplusExciter->isChecked()) {
        baseExpr = QString("((randv(t*srate) * (t < 0.05)) + %1)").arg(baseExpr);
    }

    QString logic = "// Z-Domain Dynamic Sequence\nvar z_delay = 0;\n";
    

    for (size_t i = 0; i < delays.size(); ++i) {
        double startT = i * stepTime;
        double endT = (i + 1) * stepTime;
        

        QString timeCond = (i == delays.size() - 1) 
                           ? QString("(t >= %1)").arg(startT, 0, 'f', 2) 
                           : QString("(t >= %1 & t < %2)").arg(startT, 0, 'f', 2).arg(endT, 0, 'f', 2);
                           
        logic += QString("z_delay += %1 * %2;\n").arg(timeCond).arg(delays[i]);
    }


    QString finalZ = "z_delay";
    if (m_chkDoppler->isChecked()) {
        finalZ = QString("(z_delay + %1 * sinew(t*4))").arg(m_sldDopplerDepth->value());
    }

    QString lastStr = QString("last(%1)").arg(finalZ);
    

    if (m_chkDampen->isChecked()) {
        lastStr = QString("((last(%1) + last(%1 + 1)) * 0.5)").arg(finalZ);
    }

    logic += QString("\n// Feedback Mix Matrix\nclamp(-1, %1 + %2 * %3, 1);\n")
                .arg(baseExpr).arg(fb).arg(lastStr);
                
    return logic;
}

QString ZDomainExperimentsTab::generateLegacyLogic(const std::vector<int>& delays) {

    double duration = m_spinDuration->value();
    double stepTime = duration / delays.size();
    double fb = m_sldFeedback->value() / 100.0;
    QString baseExpr = m_txtBaseO1->toPlainText();

    if (m_chkKarplusExciter->isChecked()) {
         baseExpr = QString("((randv(t*srate) * (t < 0.05)) + %1)").arg(baseExpr);
    }

    std::function<QString(int)> buildTernary = [&](int index) -> QString {
        if (index == (int)delays.size() - 1) {
            return QString::number(delays[index]);
        }
        double threshold = (index + 1) * stepTime;
        return QString("((t < %1) ? %2 : %3)")
            .arg(threshold, 0, 'f', 2)
            .arg(delays[index])
            .arg(buildTernary(index + 1));
    };

    QString zTernary = buildTernary(0);
    
    if (m_chkDoppler->isChecked()) {
        zTernary = QString("(%1 + %2 * sin(t*25.13))").arg(zTernary).arg(m_sldDopplerDepth->value());
    }

    QString lastStr = QString("last(%1)").arg(zTernary);
    
    if (m_chkDampen->isChecked()) {
        lastStr = QString("((last(%1) + last(%1 + 1)) * 0.5)").arg(zTernary);
    }

    return QString("max(-1.0, min(%1 + %2 * %3, 1.0))")
                .arg(baseExpr).arg(fb).arg(lastStr);
}

void ZDomainExperimentsTab::togglePlay(bool checked) {
    if (checked) {
        onGenerateCode(); // Ensure code is fresh
        m_btnPlay->setText("⏹ Stop");
        

        if(m_ghostSynth) {
            m_ghostSynth->setExpression(m_txtOutputCode->toPlainText());
            m_ghostSynth->start();
        }
    } else {
        m_btnPlay->setText("▶ Test Math Output");
        if(m_ghostSynth) m_ghostSynth->stop();
    }
}