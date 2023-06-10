#pragma once
#include "../ResultWriter/ResultWriter.hpp"
#include "../Structure/Process/Process.hpp"
#include <cmath>
#include <functional>
#include <ostream>
#include <utility>
#include <vector>

class DensityManager
{
    process::Database *db;
    size_t numTileForWindow; // window size(width) / step size(width)

    int64_t tileSize; // equal to step size
    int64_t tileArea, windowArea;
    size_t numTileRow, numTileCol;
    size_t numWindowRow, numWindowCol;

    process::Layer *layer;
    int64_t lowerLeftSpacing, upperRightSpacing;            // for spacing buffer expanding
    int64_t minMetalAreaConstraint, maxMetalAreaConstraint; // min/max metal area constraint for a window

    std::vector<geometry::Rectangle::ptr> allCandidateRegions;
    std::vector<process::Filler::ptr> allFillers;
    std::vector<std::vector<process::Tile>> tileGrid;
    std::vector<std::vector<int64_t>> windowGrid; // record window metal area

    std::pair<size_t, size_t> getTileIdx(
        int64_t x, int64_t y, std::function<double(double)> func = [](double d)
                              { return std::floor(d); }) const;
    std::tuple<size_t, size_t, size_t, size_t> getTileIdx(const geometry::Rectangle &boundary) const;
    bool coverByOneTile(const geometry::Rectangle &boundary) const;
    std::pair<int64_t, int64_t> getTilePos(size_t rowIdx, size_t colIdx) const;
    void updateAllWindowMetalArea();
    std::pair<int64_t, int64_t> getMinMaxWindowMetalArea() const;
    std::pair<double, double> getMinMaxWindowMetalDensity() const;
    int64_t getConductorArea(const process::Tile &tile) const;

    void initProcessLayer(process::Layer *layer_);
    void initGrid();
    void recordFreeRegion(geometry::Rectangle *freeRegion);
    void insertFiller(process::Filler *filler);
    void removeFiller(process::Filler *filler);

    std::vector<geometry::Rectangle> getAllFreeRegion(size_t rowIdx, size_t colIdx) const;
    std::vector<geometry::Rectangle> refineFreeRegion(const std::vector<geometry::Rectangle> &freeRegions) const;
    std::vector<geometry::Rectangle> filterIllegalRegion(const std::vector<geometry::Rectangle> &freeRegions) const;
    std::vector<geometry::Rectangle> generateAllFiller(const std::vector<geometry::Rectangle> &freeRegions) const;
    void removeCriticalNetFiller();
    void meetDensityConstraint();
    void removeMoreFiller();

    // for debug
    int64_t getOccupyAreaBruteForce(const process::Tile &tile) const;
    void drawBorder(std::vector<std::vector<char>> &detailGrid, const geometry::Rectangle &boundary,
                    const geometry::Rectangle &region, double scaling, char h = '-', char v = '|') const;
    void drawRegion(std::vector<std::vector<char>> &detailGrid, const geometry::Rectangle &boundary,
                    const geometry::Rectangle &region, double scaling, char c = '.') const;
    void drawTile(std::ostream &output, size_t rowIdx, size_t colIdx, bool drawFiller = true, double scaling = 0.2) const;
    void drawWindow(std::ostream &output, size_t rowIdx, size_t colIdx, bool drawFiller = true, double scaling = 0.05) const;

public:
    DensityManager(process::Database *db_, size_t numTileForWindow_ = 4);
    ResultWriter::ptr solve();
};
