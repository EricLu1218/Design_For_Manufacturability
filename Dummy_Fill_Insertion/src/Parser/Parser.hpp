#pragma once
#include "../Structure/Geometry/Geometry.hpp"
#include "../Structure/Process/Process.hpp"
#include "../Structure/Raw/Raw.hpp"
#include <istream>
#include <ostream>
#include <vector>

class Parser
{
    geometry::Rectangle chipBoundary;
    int64_t windowSize;
    size_t numCriticalNet, numLayer, numConductor;
    std::vector<int64_t> criticalNets;
    std::vector<raw::Layer::ptr> layers;
    std::vector<raw::Conductor::ptr> conductors;

    void readChipInfo(std::istream &input);
    void readNum(std::istream &input);
    void readCriticalNet(std::istream &input);
    void readLayer(std::istream &input);
    void readConductor(std::istream &input);

    void writeChipInfo(std::ostream &output) const;
    void writeNum(std::ostream &output) const;
    void writeCriticalNet(std::ostream &output) const;
    void writeLayer(std::ostream &output) const;
    void writeConductor(std::ostream &output) const;

public:
    Parser();
    bool parse(const std::string &inputFile);
    bool write(const std::string &outputFile) const;
    process::Database::ptr createDatabase() const;
};
