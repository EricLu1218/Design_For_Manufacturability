#include "Parser.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

void Parser::readChipInfo(std::istream &input)
{
    std::string buff;
    std::getline(input, buff);
    std::stringstream buffStream(buff);

    buffStream >> chipBoundary.x1 >> chipBoundary.y1 >> chipBoundary.x2 >> chipBoundary.y2;
    buffStream >> windowSize;
}

void Parser::readNum(std::istream &input)
{
    std::string buff;
    std::getline(input, buff);
    std::stringstream buffStream(buff);

    buffStream >> numCriticalNet >> numLayer >> numConductor;
}

void Parser::readCriticalNet(std::istream &input)
{
    criticalNets.reserve(numCriticalNet);
    for (size_t i = 0; i < numCriticalNet; ++i)
    {
        std::string buff;
        std::getline(input, buff);
        std::stringstream buffStream(buff);

        int64_t criticalNetId;
        buffStream >> criticalNetId;
        criticalNets.emplace_back(criticalNetId);
    }
}

void Parser::readLayer(std::istream &input)
{
    layers.reserve(numLayer);
    for (size_t i = 0; i < numLayer; ++i)
    {
        std::string buff;
        std::getline(input, buff);
        std::stringstream buffStream(buff);

        auto layer = new raw::Layer();
        buffStream >> layer->id;
        buffStream >> layer->minFillWidth >> layer->minSpacing >> layer->maxFillWidth;
        buffStream >> layer->minMetalDensity >> layer->maxMetalDensity >> layer->weight;
        layers.emplace_back(layer);
    }
}

void Parser::readConductor(std::istream &input)
{
    conductors.reserve(numConductor);
    for (size_t i = 0; i < numConductor; ++i)
    {
        std::string buff;
        std::getline(input, buff);
        std::stringstream buffStream(buff);

        auto conductor = new raw::Conductor();
        buffStream >> conductor->id;
        buffStream >> conductor->x1 >> conductor->y1 >> conductor->x2 >> conductor->y2;
        buffStream >> conductor->netId >> conductor->layerId;
        conductors.emplace_back(conductor);
    }
}

void Parser::writeChipInfo(std::ostream &output) const
{
    output << chipBoundary.x1 << " " << chipBoundary.y1 << " " << chipBoundary.x2 << " " << chipBoundary.y2 << " "
           << windowSize << "\n";
}

void Parser::writeNum(std::ostream &output) const
{
    output << numCriticalNet << " " << numLayer << " " << numConductor << "\n";
}

void Parser::writeCriticalNet(std::ostream &output) const
{
    for (auto criticalNetId : criticalNets)
        output << criticalNetId << "\n";
}

void Parser::writeLayer(std::ostream &output) const
{
    for (const auto &layer : layers)
        output << layer->id << " "
               << layer->minFillWidth << " " << layer->minSpacing << " " << layer->maxFillWidth << " "
               << layer->minMetalDensity << " " << layer->maxMetalDensity << " " << layer->weight << "\n";
}

void Parser::writeConductor(std::ostream &output) const
{
    for (const auto &conductor : conductors)
        output << conductor->id << " "
               << conductor->x1 << " " << conductor->y1 << " " << conductor->x2 << " " << conductor->y2 << " "
               << conductor->netId << " " << conductor->layerId << "\n";
}

Parser::Parser() : numCriticalNet(0), numLayer(0), numConductor(0) {}

bool Parser::parse(const std::string &filepath)
{
    std::ifstream fin(filepath);
    if (!fin.is_open())
    {
        std::cerr << "[Error] Cannot open \"" << filepath << "\".\n";
        return false;
    }

    readChipInfo(fin);
    readNum(fin);
    readCriticalNet(fin);
    readLayer(fin);
    readConductor(fin);

    std::cout << "----- DESIGN INFORMATION -----\n"
              << "Design width:   " << chipBoundary.x2 - chipBoundary.x1 << "\n"
              << "Design height:  " << chipBoundary.y2 - chipBoundary.y1 << "\n"
              << "Window size:    " << windowSize << "\n"
              << "#layers:        " << numLayer << "\n"
              << "#conductors:    " << numConductor << "\n"
              << "#critical nets: " << numCriticalNet << "\n"
              << "\n";
    return true;
}

bool Parser::write(const std::string &filepath) const
{
    std::ofstream fout(filepath);
    if (!fout.is_open())
    {
        std::cerr << "[Error] Cannot open \"" << filepath << "\".\n";
        return false;
    }

    writeChipInfo(fout);
    writeNum(fout);
    writeCriticalNet(fout);
    writeLayer(fout);
    writeConductor(fout);
    return true;
}

process::Database::ptr Parser::createDatabase() const
{
    auto database = new process::Database();
    database->chipBoundary = chipBoundary;
    database->windowSize = windowSize;

    std::unordered_map<int64_t, process::Layer *> idToLayer;
    for (const auto &layer : layers)
    {
        auto newLayer = new process::Layer(*layer.get());
        database->layers.emplace_back(newLayer);
        idToLayer.emplace(layer->id, newLayer);
    }

    std::unordered_set<int64_t> criticalNetSet(criticalNets.begin(), criticalNets.end());
    for (const auto &conductor : conductors)
    {
        auto newConductor = new process::Conductor(conductor.get());
        idToLayer[conductor->layerId]->conductors.emplace_back(newConductor);
        if (criticalNetSet.count(conductor->netId))
            newConductor->isCritical = true;
    }

    for (auto &layer : database->layers)
    {
        double aspectRatio = 0;
        for (const auto &conductor : layer->conductors)
            aspectRatio += conductor->aspectRatio();
        aspectRatio /= layer->conductors.size();
        if (aspectRatio >= 1)
            layer->direction = process::Layer::Direction::HORIZONTAL;
        else
            layer->direction = process::Layer::Direction::VERTICAL;
    }
    return std::unique_ptr<process::Database>(database);
}
