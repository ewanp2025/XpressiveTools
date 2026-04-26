#pragma once
#include <QString>
#include <QTextStream>
#include <vector>
#include <memory>
#include <random>
#include <complex>

enum class NodeType {
    VarT, Const, Add, Sub, Mul, Div, Sin, Cos, Exp, Log, Abs, Square, Cube, Tanh,
    Saw, Sqr, Tri, EnvDecay, FmOp, Noise, SoftClip,
    VarF,
    EnvStab,
    ChordStack,
    PitchKick,
    StabEnv,
    Bitcrush,
    FmRaveBass,
    OrganStab,
    _Count
};

enum class FftType {
    NativeIterative,
    KissFFT
};

struct GPNode {
    NodeType type;
    double value;
    std::unique_ptr<GPNode> left;
    std::unique_ptr<GPNode> right;

    GPNode(NodeType t, double v = 0.0) : type(t), value(v) {}

    std::unique_ptr<GPNode> clone() const;
    double evaluate(double t, double f = 523.25) const;
    QString toString(bool legacy) const;
    int getSize() const;


    QString serialize() const;
    static std::unique_ptr<GPNode> deserialize(QTextStream& stream);
};

class MicroGP {
public:
    struct Config {
        int populationSize = 250;
        int generations = 40;
        int maxDepth = 14;
        double sampleRate = 44100.0;
        bool legacySyntax = false;
        bool dechord = false;
        bool useSpectralFitness = true;

        FftType fftType = FftType::KissFFT;
        int maxAnalysisSamples = 4096;


        std::vector<QString> seedStrings;
    };

    struct DiscoveryResult {
        QString expression;
        QString serializedDNA;
        std::unique_ptr<GPNode> tree;

        DiscoveryResult() = default;
        DiscoveryResult(QString expr, QString dna, std::unique_ptr<GPNode> t)
            : expression(std::move(expr)), serializedDNA(std::move(dna)), tree(std::move(t)) {}

        DiscoveryResult(DiscoveryResult&&) noexcept = default;
        DiscoveryResult& operator=(DiscoveryResult&&) noexcept = default;

        DiscoveryResult(const DiscoveryResult& other) {
            expression = other.expression;
            serializedDNA = other.serializedDNA;
            if (other.tree) tree = other.tree->clone();
        }
    };

    MicroGP(Config config);
    DiscoveryResult discoverExpression(const std::vector<double>& targetAudio);

private:
    Config m_config;
    std::mt19937 m_rng;
    std::unique_ptr<GPNode> m_bestTree;

    std::unique_ptr<GPNode> generateRandomTree(int currentDepth);
    std::unique_ptr<GPNode> mutate(const GPNode& node);
    std::unique_ptr<GPNode> crossover(const GPNode& parent1, const GPNode& parent2);

    double calculateFitness(const GPNode& tree, const std::vector<double>& targetAudio, const std::vector<double>& targetSpectrum);

    std::vector<double> computeEnvelope(const std::vector<double>& audio) const;
    void optimizeConstants(GPNode* tree, const std::vector<double>& targetAudio, const std::vector<double>& targetSpectrum);

    void fft(std::vector<std::complex<double>>& x);
    std::vector<double> calculateMagnitudeSpectrum(const std::vector<double>& audio);
    std::vector<double> calculateMagnitudeSpectrumNative(const std::vector<double>& audio);
    std::vector<double> calculateMagnitudeSpectrumKiss(const std::vector<double>& audio);

    GPNode* getRandomNode(GPNode* root);
    int getTreeSize(const GPNode* root);
    int tournamentSelection(const std::vector<std::pair<double, int>>& fitnessScores, int k = 5);
};
