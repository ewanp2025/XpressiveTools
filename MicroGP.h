#pragma once
#include <QString>
#include <vector>
#include <memory>
#include <random>

enum class NodeType {
    VarT, Const, Add, Sub, Mul, Div, Sin, Cos, Exp, Log, Abs, Square, Cube, Tanh
};

struct GPNode {
    NodeType type;
    double value;
    std::unique_ptr<GPNode> left;
    std::unique_ptr<GPNode> right;

    GPNode(NodeType t, double v = 0.0) : type(t), value(v) {}

    std::unique_ptr<GPNode> clone() const;
    double evaluate(double t) const;
    QString toString(bool legacy) const;
};

class MicroGP {
public:
    struct Config {
        int populationSize = 300;
        int generations = 60;
        int maxDepth = 6;
        double sampleRate = 8000.0;
        bool legacySyntax = false;
        bool dechord = false;
    };

    struct DiscoveryResult {
        QString expression;
        std::unique_ptr<GPNode> tree;
    };

    MicroGP(Config config);
    QString discoverExpression(const std::vector<double>& targetAudio);
    std::unique_ptr<GPNode> m_bestTree;

private:
    Config m_config;
    std::mt19937 m_rng;

    std::unique_ptr<GPNode> generateRandomTree(int currentDepth);
    std::unique_ptr<GPNode> mutate(const GPNode& node);
    std::unique_ptr<GPNode> crossover(const GPNode& parent1, const GPNode& parent2);

    double calculateFitness(const GPNode& tree, const std::vector<double>& targetAudio);
    GPNode* getRandomNode(GPNode* root);
    int getTreeSize(const GPNode* root);
};
