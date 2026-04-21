#include "MicroGP.h"
#include <cmath>
#include <algorithm>
#include <numeric>

std::unique_ptr<GPNode> GPNode::clone() const {
    auto n = std::make_unique<GPNode>(type, value);
    if (left) n->left = left->clone();
    if (right) n->right = right->clone();
    return n;
}

double GPNode::evaluate(double t) const {
    switch (type) {
    case NodeType::VarT: return t;
    case NodeType::Const: return value;
    case NodeType::Add: return left->evaluate(t) + right->evaluate(t);
    case NodeType::Sub: return left->evaluate(t) - right->evaluate(t);
    case NodeType::Mul: return left->evaluate(t) * right->evaluate(t);
    case NodeType::Div: {
        double den = right->evaluate(t);
        return (std::abs(den) < 1e-6) ? 1.0 : (left->evaluate(t) / den);
    }
    case NodeType::Sin: return std::sin(left->evaluate(t));
    case NodeType::Cos: return std::cos(left->evaluate(t));
    case NodeType::Exp: return std::exp(std::clamp(left->evaluate(t), -10.0, 10.0));
    case NodeType::Log: return std::log(std::abs(left->evaluate(t)) + 1e-6);
    case NodeType::Abs: return std::abs(left->evaluate(t));
    case NodeType::Square: { double v = left->evaluate(t); return v * v; }
    case NodeType::Cube: { double v = left->evaluate(t); return v * v * v; }
    case NodeType::Tanh: return std::tanh(left->evaluate(t));
    default: return 0.0;
    }
}

QString GPNode::toString(bool legacy) const {
    switch (type) {
    case NodeType::VarT: return legacy ? "t" : "$t$";
    case NodeType::Const: return QString::number(value, 'f', 4);
    case NodeType::Add: return "(" + left->toString(legacy) + " + " + right->toString(legacy) + ")";
    case NodeType::Sub: return "(" + left->toString(legacy) + " - " + right->toString(legacy) + ")";
    case NodeType::Mul: return "(" + left->toString(legacy) + " * " + right->toString(legacy) + ")";
    case NodeType::Div: return "(" + left->toString(legacy) + " / " + right->toString(legacy) + ")";
    case NodeType::Sin: return "sin(" + left->toString(legacy) + ")";
    case NodeType::Cos: return "cos(" + left->toString(legacy) + ")";
    case NodeType::Exp: return "exp(" + left->toString(legacy) + ")";
    case NodeType::Log: return "log(" + left->toString(legacy) + ")";
    case NodeType::Abs: return "abs(" + left->toString(legacy) + ")";
    case NodeType::Square: return legacy ? "(" + left->toString(legacy) + "*" + left->toString(legacy) + ")" : "square(" + left->toString(legacy) + ")";
    case NodeType::Cube: return legacy ? "(" + left->toString(legacy) + "*" + left->toString(legacy) + "*" + left->toString(legacy) + ")" : "cube(" + left->toString(legacy) + ")";
    case NodeType::Tanh: return "tanh(" + left->toString(legacy) + ")";
    default: return "";
    }
}

MicroGP::MicroGP(Config config) : m_config(config) {
    std::random_device rd;
    m_rng.seed(rd());
}

QString MicroGP::discoverExpression(const std::vector<double>& targetAudio)
{
    std::vector<std::unique_ptr<GPNode>> population;
    for (int i = 0; i < m_config.populationSize; ++i) {
        population.push_back(generateRandomTree(0));
    }

    std::unique_ptr<GPNode> bestOverall;
    double bestFitness = std::numeric_limits<double>::max();

    for (int gen = 0; gen < m_config.generations; ++gen) {
        std::vector<std::pair<double, int>> fitnessScores;

        for (int i = 0; i < m_config.populationSize; ++i) {
            double fit = calculateFitness(*population[i], targetAudio);
            fit += getTreeSize(population[i].get()) * 0.001;

            if (std::isnan(fit) || std::isinf(fit)) fit = 999999.0;
            fitnessScores.push_back({fit, i});

            if (fit < bestFitness) {
                bestFitness = fit;
                bestOverall = population[i]->clone();
            }
        }

        std::sort(fitnessScores.begin(), fitnessScores.end());
        std::vector<std::unique_ptr<GPNode>> nextGen;

        int elites = std::max(1, m_config.populationSize / 20);
        for (int i = 0; i < elites; ++i) {
            nextGen.push_back(population[fitnessScores[i].second]->clone());
        }

        std::uniform_int_distribution<int> tournDist(0, m_config.populationSize / 3);
        std::uniform_real_distribution<double> chance(0.0, 1.0);

        while (nextGen.size() < m_config.populationSize) {
            int p1_idx = fitnessScores[tournDist(m_rng)].second;
            if (chance(m_rng) < 0.7) {
                int p2_idx = fitnessScores[tournDist(m_rng)].second;
                nextGen.push_back(crossover(*population[p1_idx], *population[p2_idx]));
            } else {
                nextGen.push_back(mutate(*population[p1_idx]));
            }
        }
        population = std::move(nextGen);
    }
    m_bestTree = bestOverall ? bestOverall->clone() : nullptr;
    return bestOverall ? bestOverall->toString(m_config.legacySyntax) : "0";
}

double MicroGP::calculateFitness(const GPNode& tree, const std::vector<double>& targetAudio) {
    double errorSum = 0.0;
    int samples = targetAudio.size();
    if (samples == 0) return 999999.0;

    for (int i = 0; i < samples; ++i) {
        double t = (double)i / m_config.sampleRate;
        double predicted = tree.evaluate(t);
        if (std::isnan(predicted) || std::isinf(predicted)) return 999999.0;
        double diff = predicted - targetAudio[i];
        errorSum += (diff * diff);
    }
    return errorSum / samples;
}

std::unique_ptr<GPNode> MicroGP::generateRandomTree(int currentDepth) {
    std::uniform_int_distribution<int> typeDist(0, 13);
    std::uniform_real_distribution<double> valDist(-5.0, 5.0);

    if (currentDepth >= m_config.maxDepth) {
        if (typeDist(m_rng) % 2 == 0) return std::make_unique<GPNode>(NodeType::VarT);
        return std::make_unique<GPNode>(NodeType::Const, valDist(m_rng));
    }

    NodeType type = static_cast<NodeType>(typeDist(m_rng));

    // === DECHORD BIAS ===
    if (m_config.dechord && (typeDist(m_rng) % 3 == 0)) {
        std::uniform_int_distribution<int> trigDist(5, 7);
        type = static_cast<NodeType>(trigDist(m_rng));
    }

    if (currentDepth < 2) {
        std::uniform_int_distribution<int> basicDist(2, 5);
        type = static_cast<NodeType>(basicDist(m_rng));
    }

    auto node = std::make_unique<GPNode>(type, valDist(m_rng));

    if (type >= NodeType::Add && type <= NodeType::Div) {
        node->left = generateRandomTree(currentDepth + 1);
        node->right = generateRandomTree(currentDepth + 1);
    } else if (type >= NodeType::Sin && type <= NodeType::Tanh) {
        node->left = generateRandomTree(currentDepth + 1);
    }
    return node;
}



std::unique_ptr<GPNode> MicroGP::mutate(const GPNode& node) {
    auto clone = node.clone();
    GPNode* target = getRandomNode(clone.get());
    if (target) {
        auto newSubTree = generateRandomTree(0);
        target->type = newSubTree->type;
        target->value = newSubTree->value;
        target->left = std::move(newSubTree->left);
        target->right = std::move(newSubTree->right);
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
    return 1 + getTreeSize(root->left.get()) + getTreeSize(root->right.get());
}
