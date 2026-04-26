extern "C" {
#include "kiss_fft.h"
}

#include "MicroGP.h"
#include <QStringList>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <future>
#include <thread>
#include <functional>
#include <cstdlib>

std::unique_ptr<GPNode> GPNode::clone() const {
    auto n = std::make_unique<GPNode>(type, value);
    if (left) n->left = left->clone();
    if (right) n->right = right->clone();
    return n;
}

int GPNode::getSize() const {
    int size = 1;
    if (left) size += left->getSize();
    if (right) size += right->getSize();
    return size;
}


QString GPNode::serialize() const {
    QStringList tokens;

    tokens << QString::number((int)type) << QString::number(value, 'f', 6);
    if (left) tokens << left->serialize();
    else tokens << "-1";
    if (right) tokens << right->serialize();
    else tokens << "-1";
    return tokens.join(" ");
}

std::unique_ptr<GPNode> GPNode::deserialize(QTextStream& stream) {
    QString typeStr;
    stream >> typeStr;
    if (typeStr == "-1" || typeStr.isEmpty()) return nullptr;

    int typeInt = typeStr.toInt();
    double val;
    stream >> val;

    auto node = std::make_unique<GPNode>(static_cast<NodeType>(typeInt), val);
    node->left = deserialize(stream);
    node->right = deserialize(stream);
    return node;
}


double GPNode::evaluate(double t, double f) const {
    double pi2 = 6.28318530718;
    switch (type) {
    case NodeType::VarT: return t;
    case NodeType::VarF: return f;
    case NodeType::Const: return value;
    case NodeType::Add: return left->evaluate(t, f) + right->evaluate(t, f);
    case NodeType::Sub: return left->evaluate(t, f) - right->evaluate(t, f);
    case NodeType::Mul: return left->evaluate(t, f) * right->evaluate(t, f);
    case NodeType::Div: {
        double den = right->evaluate(t, f);
        return (std::abs(den) < 1e-6) ? 1.0 : (left->evaluate(t, f) / den);
    }
    case NodeType::Sin: return std::sin(left->evaluate(t, f));
    case NodeType::Cos: return std::cos(left->evaluate(t, f));
    case NodeType::Exp: return std::exp(std::clamp(left->evaluate(t, f), -10.0, 10.0));
    case NodeType::Log: return std::log(std::abs(left->evaluate(t, f)) + 1e-6);
    case NodeType::Abs: return std::abs(left->evaluate(t, f));
    case NodeType::Square: { double v = left->evaluate(t, f); return v * v; }
    case NodeType::Cube: { double v = left->evaluate(t, f); return v * v * v; }
    case NodeType::Tanh: return std::tanh(left->evaluate(t, f));

    case NodeType::Saw: {
        double phase = std::fmod(left->evaluate(t, f), pi2);
        return (phase / 3.14159265359) - 1.0;
    }
    case NodeType::Sqr: return std::sin(left->evaluate(t, f)) > 0 ? 1.0 : -1.0;
    case NodeType::Tri: {
        double phase = std::fmod(left->evaluate(t, f), pi2);
        return 2.0 * std::abs((phase / 3.14159265359) - 1.0) - 1.0;
    }
    case NodeType::EnvDecay: return std::exp(-std::abs(left->evaluate(t, f)) * t);
    case NodeType::FmOp: return std::sin(left->evaluate(t, f) + right->evaluate(t, f));
    case NodeType::Noise: {
        double noise = std::sin(t * 12.9898) * 43758.5453;
        return (noise - std::floor(noise)) * 2.0 - 1.0;
    }
    case NodeType::SoftClip: return std::tanh(left->evaluate(t, f) * 2.5);

    case NodeType::EnvStab: {
        double env = 0.0;
        if (t < 0.005) env = t / 0.005;
        else if (t < 0.07) env = 1.0 - (t - 0.005) / 0.065;
        return left->evaluate(t, f) * env;
    }
    case NodeType::ChordStack: {
        double base = left->evaluate(t, f);
        double m3 = left->evaluate(t, f * 1.1892);
        double p5 = left->evaluate(t, f * 1.4983);
        double m7 = left->evaluate(t, f * 1.7817);
        return (base * 0.4) + (m3 * 0.2) + (p5 * 0.2) + (m7 * 0.2);
    }
    case NodeType::PitchKick: {
        return std::sin(t * f * std::exp(-t * 60.0) * pi2) * std::exp(-t * 10.0);
    }
    case NodeType::StabEnv: {
        double filterPluck = std::exp(-t * 25.0);
        double ampEnv = std::exp(-t * 5.0);
        double complexBody = left->evaluate(t, f) * filterPluck;
        double sub = std::sin(t * f * pi2) * ampEnv * 0.5;
        return complexBody + sub;
    }
    case NodeType::Bitcrush: {
        double rawAudio = left->evaluate(t, f);
        double steps = std::clamp(std::abs(right ? right->evaluate(t, f) : 16.0), 4.0, 256.0);
        return std::floor(rawAudio * steps) / steps;
    }
    case NodeType::FmRaveBass: {
        double modIndex = std::abs(left ? left->evaluate(t, f) : 2.0);
        double op1 = std::sin(t * f * 2.0 * pi2) * std::exp(-t * 20.0) * modIndex;
        double op2 = std::sin((t * f * 1.0 * pi2) + op1) * std::exp(-t * 10.0) * (modIndex * 0.5);
        double carrier = std::sin((t * f * 1.0 * pi2) + op2);
        return carrier * std::exp(-t * 4.0);
    }
    case NodeType::OrganStab: {
        double fund = std::sin(t * f * pi2);
        double oct  = std::sin(t * f * 2.0 * pi2) * 0.8;
        double p5   = std::sin(t * f * 3.0 * pi2) * 0.6;
        double click = std::sin(t * f * 12.0 * pi2) * std::exp(-t * 80.0) * 0.5;
        double customEnv = std::clamp(left ? left->evaluate(t, f) : std::exp(-t*5.0), 0.0, 1.0);
        return ((fund + oct + p5) * customEnv) + click;
    }

    default: return 0.0;
    }
}

QString GPNode::toString(bool legacy) const {
    QString t_str = "t";
    QString f_str = "f";
    QString l_str = left ? left->toString(legacy) : "1.0";
    QString r_str = right ? right->toString(legacy) : "16.0";
    QString sin_str = legacy ? "sinew" : "sin";

    switch (type) {
    case NodeType::VarT: return t_str;
    case NodeType::VarF: return f_str;
    case NodeType::Const: return QString::number(value, 'f', 6);
    case NodeType::Add: return "(" + l_str + " + " + r_str + ")";
    case NodeType::Sub: return "(" + l_str + " - " + r_str + ")";
    case NodeType::Mul: return "(" + l_str + " * " + r_str + ")";
    case NodeType::Div: return "(" + l_str + " / " + r_str + ")";

    case NodeType::Sin: return sin_str + "(" + l_str + ")";
    case NodeType::Cos: return "cos(" + l_str + ")";
    case NodeType::Exp: return "exp(" + l_str + ")";
    case NodeType::Log: return "log(" + l_str + ")";
    case NodeType::Abs: return "abs(" + l_str + ")";
    case NodeType::Square: return "((" + l_str + ") * (" + l_str + "))";
    case NodeType::Cube: return "((" + l_str + ") * (" + l_str + ") * (" + l_str + "))";
    case NodeType::Tanh: return "tanh(" + l_str + ")";

    case NodeType::Saw:
        if (legacy) return "saww((" + l_str + ") / 6.283185)";
        return "((fmod((" + l_str + "), 6.283185) / 3.14159) - 1.0)";
    case NodeType::Sqr:
        if (legacy) return "squarew((" + l_str + ") / 6.283185)";
        return "(" + sin_str + "(" + l_str + ") > 0 ? 1.0 : -1.0)";
    case NodeType::Tri:
        if (legacy) return "trianglew((" + l_str + ") / 6.283185)";
        return "(2.0 * abs((fmod((" + l_str + "), 6.283185) / 3.14159) - 1.0) - 1.0)";

    case NodeType::EnvDecay: return "exp(-abs(" + l_str + ") * " + t_str + ")";
    case NodeType::FmOp: return sin_str + "((" + l_str + ") + (" + r_str + "))";

    case NodeType::Noise:
        if (legacy) return "randv(" + t_str + " * srate)";
        return "((sin(" + t_str + " * 12.9898) * 43758.5453 - floor(sin(" + t_str + " * 12.9898) * 43758.5453)) * 2.0 - 1.0)";

    case NodeType::SoftClip: return "tanh((" + l_str + ") * 2.5)";

    case NodeType::EnvStab: {
        QString andOp = legacy ? "&" : "and";
        return "(" + l_str + " * ((" + t_str + " < 0.005)*(" + t_str + "/0.005) + (" + t_str + " >= 0.005 " + andOp + " " + t_str + " < 0.07)*(1 - (" + t_str + "-0.005)/0.065) + (" + t_str + " >= 0.07)*0))";
    }
    case NodeType::ChordStack: {
        QString m3Str = l_str; m3Str.replace(f_str, "(" + f_str + "*1.1892)");
        QString p5Str = l_str; p5Str.replace(f_str, "(" + f_str + "*1.4983)");
        QString m7Str = l_str; m7Str.replace(f_str, "(" + f_str + "*1.7817)");
        return "((" + l_str + "*0.4) + (" + m3Str + "*0.2) + (" + p5Str + "*0.2) + (" + m7Str + "*0.2))";
    }
    case NodeType::PitchKick:
        if (legacy) return "(sinew(" + t_str + " * " + f_str + " * exp(-" + t_str + " * 60.0)) * exp(-" + t_str + " * 10.0))";
        return "(sin(" + t_str + " * " + f_str + " * exp(-" + t_str + " * 60.0) * 6.283185) * exp(-" + t_str + " * 10.0))";
    case NodeType::StabEnv:
        return "((" + l_str + " * exp(-" + t_str + " * 25.0)) + (" + sin_str + "(" + t_str + " * " + f_str + " * 6.28318) * exp(-" + t_str + " * 5.0) * 0.5))";
    case NodeType::Bitcrush:
        return "(floor((" + l_str + ") * clamp(abs(" + r_str + "), 4.0, 256.0)) / clamp(abs(" + r_str + "), 4.0, 256.0))";
    case NodeType::FmRaveBass: {
        QString index = "abs(" + l_str + ")";
        QString op1 = "(" + sin_str + "(" + t_str + "*" + f_str + "*12.566)*exp(-" + t_str + "*20.0)*" + index + ")";
        QString op2 = "(" + sin_str + "(" + t_str + "*" + f_str + "*6.283+" + op1 + ")*exp(-" + t_str + "*10.0)*(" + index + "*0.5))";
        return "(" + sin_str + "(" + t_str + "*" + f_str + "*6.283+" + op2 + ") * exp(-" + t_str + "*4.0))";
    }
    case NodeType::OrganStab: {
        QString fund = sin_str + "(" + t_str + "*" + f_str + "*6.28318)";
        QString oct = "(" + sin_str + "(" + t_str + "*" + f_str + "*12.56637)*0.8)";
        QString p5 = "(" + sin_str + "(" + t_str + "*" + f_str + "*18.84955)*0.6)";
        QString click = "(" + sin_str + "(" + t_str + "*" + f_str + "*75.398)*exp(-" + t_str + "*80.0)*0.5)";
        QString env = "clamp(" + l_str + ", 0.0, 1.0)";
        return "(((" + fund + "+" + oct + "+" + p5 + ") * " + env + ") + " + click + ")";
    }

    default: return "";
    }
}

MicroGP::MicroGP(Config config) : m_config(config) {
    std::random_device rd;
    m_rng.seed(rd());
}

std::vector<double> MicroGP::computeEnvelope(const std::vector<double>& audio) const {
    std::vector<double> env(audio.size(), 0.0);
    double alpha = 0.97;
    double current = 0.0;
    for (size_t i = 0; i < audio.size(); ++i) {
        current = alpha * current + (1.0 - alpha) * std::abs(audio[i]);
        env[i] = current;
    }
    return env;
}

void MicroGP::optimizeConstants(GPNode* tree, const std::vector<double>& targetAudio, const std::vector<double>& targetSpectrum) {
    if (!tree) return;
    std::vector<GPNode*> constants;
    std::function<void(GPNode*)> collect = [&](GPNode* n) {
        if (!n) return;
        if (n->type == NodeType::Const) constants.push_back(n);
        collect(n->left.get());
        collect(n->right.get());
    };
    collect(tree);

    if (constants.empty()) return;

    double bestFit = calculateFitness(*tree, targetAudio, targetSpectrum);
    std::normal_distribution<double> noise(0.0, 0.3);

    for (int iter = 0; iter < 60; ++iter) {
        GPNode* c = constants[m_rng() % constants.size()];
        double oldVal = c->value;
        c->value += noise(m_rng);

        double newFit = calculateFitness(*tree, targetAudio, targetSpectrum);
        if (newFit < bestFit) {
            bestFit = newFit;
        } else {
            c->value = oldVal;
        }
    }
}

double MicroGP::calculateFitness(const GPNode& tree, const std::vector<double>& targetAudio, const std::vector<double>& targetSpectrum) {
    int samples = targetAudio.size();
    if (samples == 0) return 999999.0;

    std::vector<double> generated(samples);
    double timeDomainError = 0.0;
    double baseFrequency = 523.25;

    auto targetEnv = computeEnvelope(targetAudio);

    for (int i = 0; i < samples; ++i) {
        double t = (double)i / m_config.sampleRate;
        double val = tree.evaluate(t, baseFrequency);
        if (std::isnan(val) || std::isinf(val)) return 999999.0;
        generated[i] = val;

        double diff = val - targetAudio[i];
        double weight = (i < samples * 0.2) ? 8.0 : (i < samples * 0.4 ? 3.0 : 1.0);
        timeDomainError += (diff * diff) * weight;
    }
    timeDomainError /= samples;

    auto genEnv = computeEnvelope(generated);
    double envError = 0.0;
    for (size_t i = 0; i < targetEnv.size(); ++i) {
        double d = targetEnv[i] - genEnv[i];
        envError += d * d;
    }
    envError /= targetEnv.size();

    if (!m_config.useSpectralFitness) {
        return timeDomainError + 0.4 * envError;
    }

    std::vector<double> genSpectrum = calculateMagnitudeSpectrum(generated);
    double spectralError = 0.0;
    double totalWeight = 0.0;
    size_t limit = std::min(genSpectrum.size(), targetSpectrum.size()) / 3;

    for (size_t i = 1; i < limit; ++i) {
        double diff = genSpectrum[i] - targetSpectrum[i];
        double weight = 1.0 / std::log(2.0 + i);
        spectralError += (diff * diff) * weight;
        totalWeight += weight;
    }
    if (totalWeight > 0) spectralError /= totalWeight;

    return (timeDomainError * 0.25) + (envError * 0.35) + (spectralError * 0.40);
}

std::vector<double> MicroGP::calculateMagnitudeSpectrum(const std::vector<double>& audio) {
    if (m_config.fftType == FftType::KissFFT) {
        return calculateMagnitudeSpectrumKiss(audio);
    } else {
        return calculateMagnitudeSpectrumNative(audio);
    }
}

std::vector<double> MicroGP::calculateMagnitudeSpectrumKiss(const std::vector<double>& audio) {
    int n = 1;
    while (n < audio.size()) n *= 2;

    kiss_fft_cfg cfg = kiss_fft_alloc(n, 0, nullptr, nullptr);
    if (!cfg) return std::vector<double>(n / 2, 0.0);

    std::vector<kiss_fft_cpx> cx_in(n);
    std::vector<kiss_fft_cpx> cx_out(n);

    for (int i = 0; i < n; ++i) {
        if (i < audio.size()) {
            double window = 0.5 * (1.0 - std::cos(2.0 * 3.141592653589793 * i / (audio.size() - 1)));
            cx_in[i].r = static_cast<float>(audio[i] * window);
        } else {
            cx_in[i].r = 0.0f;
        }
        cx_in[i].i = 0.0f;
    }

    kiss_fft(cfg, cx_in.data(), cx_out.data());
    free(cfg);

    std::vector<double> magnitudes(n / 2);
    for (int i = 0; i < n / 2; ++i) {
        magnitudes[i] = std::sqrt(cx_out[i].r * cx_out[i].r + cx_out[i].i * cx_out[i].i);
    }
    return magnitudes;
}

void MicroGP::fft(std::vector<std::complex<double>>& a) {
    int n = a.size();
    if (n <= 1) return;

    for (int i = 1, j = 0; i < n; i++) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(a[i], a[j]);
    }

    for (int len = 2; len <= n; len <<= 1) {
        double angle = -2.0 * 3.14159265358979323846 / len;
        std::complex<double> wlen(std::cos(angle), std::sin(angle));
        for (int i = 0; i < n; i += len) {
            std::complex<double> w(1.0, 0.0);
            for (int j = 0; j < len / 2; j++) {
                std::complex<double> u = a[i + j];
                std::complex<double> v = a[i + j + len / 2] * w;
                a[i + j] = u + v;
                a[i + j + len / 2] = u - v;
                w *= wlen;
            }
        }
    }
}

std::vector<double> MicroGP::calculateMagnitudeSpectrumNative(const std::vector<double>& audio) {
    int n = 1;
    while (n < audio.size()) n *= 2;

    std::vector<std::complex<double>> buffer(n, 0.0);
    for (size_t i = 0; i < audio.size(); ++i) {
        double window = 0.5 * (1.0 - std::cos(2.0 * 3.141592653589793 * i / (audio.size() - 1)));
        buffer[i] = std::complex<double>(audio[i] * window, 0.0);
    }

    fft(buffer);

    std::vector<double> magnitudes(n / 2);
    for (int i = 0; i < n / 2; ++i) {
        magnitudes[i] = std::abs(buffer[i]);
    }
    return magnitudes;
}

MicroGP::DiscoveryResult MicroGP::discoverExpression(const std::vector<double>& targetAudio) {
    if (targetAudio.empty()) return {"0", "", nullptr};

    std::vector<double> processAudio = targetAudio;
    if (processAudio.size() > m_config.maxAnalysisSamples)
        processAudio.resize(m_config.maxAnalysisSamples);

    std::vector<double> targetSpectrum;
    if (m_config.useSpectralFitness)
        targetSpectrum = calculateMagnitudeSpectrum(processAudio);

    std::vector<std::unique_ptr<GPNode>> population;


    for (const QString& seedStr : m_config.seedStrings) {
        QString tempStr = seedStr;
        QTextStream stream(&tempStr);
        auto tree = GPNode::deserialize(stream);
        if (tree && population.size() < m_config.populationSize) {
            population.push_back(std::move(tree));
        }
    }


    while (population.size() < m_config.populationSize) {
        population.push_back(generateRandomTree(0));
    }

    std::unique_ptr<GPNode> bestOverall;
    double bestFitness = std::numeric_limits<double>::max();

    for (int gen = 0; gen < m_config.generations; ++gen) {
        std::vector<std::pair<double, int>> fitnessScores(m_config.populationSize);

        int hardwareThreads = std::max(1, (int)std::thread::hardware_concurrency());
        int chunkSize = m_config.populationSize / hardwareThreads;
        std::vector<std::future<void>> futures;

        for (int t = 0; t < hardwareThreads; ++t) {
            int start = t * chunkSize;
            int end = (t == hardwareThreads-1) ? m_config.populationSize : (t+1)*chunkSize;
            futures.push_back(std::async(std::launch::async, [&, start, end]() {
                for (int i = start; i < end; ++i) {
                    double fit = calculateFitness(*population[i], processAudio, targetSpectrum);
                    if (std::isnan(fit) || std::isinf(fit)) fit = 999999.0;
                    fitnessScores[i] = {fit, i};
                }
            }));
        }
        for (auto& f : futures) f.get();

        for (int i = 0; i < m_config.populationSize; ++i) {
            if (fitnessScores[i].first < bestFitness) {
                bestFitness = fitnessScores[i].first;
                bestOverall = population[i]->clone();
            }
        }

        std::sort(fitnessScores.begin(), fitnessScores.end());

        std::vector<std::unique_ptr<GPNode>> nextGen;
        int elites = std::max(2, m_config.populationSize / 15);

        for (int i = 0; i < elites; ++i) {
            auto eliteClone = population[fitnessScores[i].second]->clone();
            nextGen.push_back(std::move(eliteClone));
        }

        std::uniform_real_distribution<double> chance(0.0, 1.0);
        while (nextGen.size() < m_config.populationSize) {
            int p1 = tournamentSelection(fitnessScores, 5);
            if (chance(m_rng) < 0.75) {
                int p2 = tournamentSelection(fitnessScores, 5);
                nextGen.push_back(crossover(*population[p1], *population[p2]));
            } else {
                nextGen.push_back(mutate(*population[p1]));
            }
        }
        population = std::move(nextGen);
    }

    if (bestOverall) {
        optimizeConstants(bestOverall.get(), processAudio, targetSpectrum);
    }

    DiscoveryResult res;
    if (bestOverall) {
        res.expression = bestOverall->toString(m_config.legacySyntax);
        res.serializedDNA = bestOverall->serialize(); // Export the DNA
        res.tree = bestOverall->clone();
    } else {
        res.expression = "0";
        res.serializedDNA = "";
        res.tree = nullptr;
    }
    m_bestTree = bestOverall ? bestOverall->clone() : nullptr;
    return res;
}

int MicroGP::tournamentSelection(const std::vector<std::pair<double, int>>& fitnessScores, int k) {
    std::uniform_int_distribution<int> dist(0, m_config.populationSize - 1);
    int bestIdx = dist(m_rng);
    double bestFit = fitnessScores[bestIdx].first;

    for (int i = 1; i < k; ++i) {
        int cand = dist(m_rng);
        if (fitnessScores[cand].first < bestFit) {
            bestFit = fitnessScores[cand].first;
            bestIdx = cand;
        }
    }
    return fitnessScores[bestIdx].second;
}

std::unique_ptr<GPNode> MicroGP::generateRandomTree(int currentDepth) {
    std::uniform_real_distribution<double> valDist(-5.0, 5.0);
    std::uniform_int_distribution<int> typeDist(0, static_cast<int>(NodeType::_Count) - 1);

    if (currentDepth >= m_config.maxDepth) {
        int r = typeDist(m_rng) % 3;
        if (r == 0) return std::make_unique<GPNode>(NodeType::VarT);
        if (r == 1) return std::make_unique<GPNode>(NodeType::VarF);
        return std::make_unique<GPNode>(NodeType::Const, valDist(m_rng));
    }

    NodeType type;

    if (m_config.dechord) {
        NodeType synthNodes[] = {
            NodeType::Add, NodeType::Mul, NodeType::VarF,
            NodeType::Saw, NodeType::Sqr, NodeType::FmOp,
            NodeType::EnvDecay, NodeType::StabEnv,
            NodeType::SoftClip, NodeType::OrganStab, NodeType::Bitcrush,
            NodeType::ChordStack, NodeType::FmRaveBass
        };
        std::uniform_int_distribution<int> synthDist(0, 12);
        type = synthNodes[synthDist(m_rng)];
    } else {
        type = static_cast<NodeType>(typeDist(m_rng));
    }

    auto node = std::make_unique<GPNode>(type, valDist(m_rng));

    if (type == NodeType::Add || type == NodeType::Sub || type == NodeType::Mul || type == NodeType::Div || type == NodeType::FmOp || type == NodeType::Bitcrush) {
        node->left = generateRandomTree(currentDepth + 1);
        node->right = generateRandomTree(currentDepth + 1);
    }
    else if (type == NodeType::VarT || type == NodeType::VarF || type == NodeType::Const || type == NodeType::Noise || type == NodeType::PitchKick || type == NodeType::OrganStab) {
    }
    else {
        node->left = generateRandomTree(currentDepth + 1);
    }

    return node;
}

std::unique_ptr<GPNode> MicroGP::mutate(const GPNode& node) {
    auto clone = node.clone();
    GPNode* target = getRandomNode(clone.get());

    if (target) {
        std::uniform_real_distribution<double> chance(0.0, 1.0);

        if (target->type == NodeType::Const) {
            std::normal_distribution<double> creep(0.0, 0.5);
            target->value += creep(m_rng);
        }
        else if (chance(m_rng) < 0.3) {
            if (target->type == NodeType::Add) target->type = NodeType::Mul;
            else if (target->type == NodeType::Mul) target->type = NodeType::Add;
            else if (target->type == NodeType::Sin) target->type = NodeType::Tanh;
            else if (target->type == NodeType::Saw) target->type = NodeType::Sqr;
        }
        else {
            auto newSubTree = generateRandomTree(0);
            target->type = newSubTree->type;
            target->value = newSubTree->value;
            target->left = std::move(newSubTree->left);
            target->right = std::move(newSubTree->right);
        }
    }
    return clone;
}

std::unique_ptr<GPNode> MicroGP::crossover(const GPNode& parent1, const GPNode& parent2) {
    auto clone1 = parent1.clone();
    auto clone2 = parent2.clone();

    GPNode* target1 = getRandomNode(clone1.get());
    GPNode* target2 = getRandomNode(clone2.get());

    if (target1 && target2) {
        std::swap(target1->type, target2->type);
        std::swap(target1->value, target2->value);
        std::swap(target1->left, target2->left);
        std::swap(target1->right, target2->right);
    }
    return clone1;
}

GPNode* MicroGP::getRandomNode(GPNode* root) {
    std::vector<GPNode*> nodes;
    std::function<void(GPNode*)> collect = [&](GPNode* n) {
        if (!n) return;
        nodes.push_back(n);
        collect(n->left.get());
        collect(n->right.get());
    };
    collect(root);
    if (nodes.empty()) return nullptr;
    std::uniform_int_distribution<int> dist(0, nodes.size() - 1);
    return nodes[dist(m_rng)];
}

int MicroGP::getTreeSize(const GPNode* root) {
    if (!root) return 0;
    return root->getSize();
}
