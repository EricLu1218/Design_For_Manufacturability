#include "DensityManager.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

std::pair<size_t, size_t> DensityManager::getTileIdx(int64_t x, int64_t y, std::function<double(double)> func) const
{
    size_t rowIdx = func(static_cast<double>(y - db->chipBoundary.y1) / tileSize);
    size_t colIdx = func(static_cast<double>(x - db->chipBoundary.x1) / tileSize);
    return {rowIdx, colIdx};
}

std::tuple<size_t, size_t, size_t, size_t> DensityManager::getTileIdx(const geometry::Rectangle &boundary) const
{
    size_t beginRowIdx = std::floor(static_cast<double>(boundary.y1 - db->chipBoundary.y1) / tileSize);
    size_t beginColIdx = std::floor(static_cast<double>(boundary.x1 - db->chipBoundary.x1) / tileSize);
    size_t endRowIdx = std::ceil(static_cast<double>(boundary.y2 - db->chipBoundary.y1) / tileSize);
    size_t endColIdx = std::ceil(static_cast<double>(boundary.x2 - db->chipBoundary.x1) / tileSize);
    return {beginRowIdx, beginColIdx, endRowIdx, endColIdx};
}

std::pair<int64_t, int64_t> DensityManager::getTilePos(size_t rowIdx, size_t colIdx) const
{
    int64_t x = db->chipBoundary.x1 + colIdx * tileSize;
    int64_t y = db->chipBoundary.y1 + rowIdx * tileSize;
    return {x, y};
}

void DensityManager::updateAllWindowMetalArea()
{
    for (size_t rowIdx = 0; rowIdx < numWindowRow; ++rowIdx)
    {
        for (size_t colIdx = 0; colIdx < numWindowCol; ++colIdx)
        {
            int64_t occupyArea = 0;
            for (size_t r = 0; r < numTileForWindow; ++r)
                for (size_t c = 0; c < numTileForWindow; ++c)
                    occupyArea += tileGrid[rowIdx + r][colIdx + c].occupyArea();
            windowGrid[rowIdx][colIdx] = occupyArea;
        }
    }
}

std::pair<int64_t, int64_t> DensityManager::getMinMaxWindowMetalArea() const
{
    int64_t minArea = windowArea;
    int64_t maxArea = 0;
    for (const auto &row : windowGrid)
    {
        for (const auto &area : row)
        {
            minArea = std::min(minArea, area);
            maxArea = std::max(maxArea, area);
        }
    }
    return {minArea, maxArea};
}

std::pair<double, double> DensityManager::getMinMaxWindowMetalDensity() const
{
    auto [minArea, maxArea] = getMinMaxWindowMetalArea();
    return {static_cast<double>(minArea) / windowArea, static_cast<double>(maxArea) / windowArea};
}

int64_t DensityManager::getConductorArea(const process::Tile &tile) const
{
    int64_t conductorArea = 0;
    // directly add conductor areas
    std::unordered_map<int64_t, std::vector<process::Conductor *>> netIdToConductors;
    for (const auto conductor : tile.conductors)
    {
        conductorArea += geometry::getIntersectRegion(tile, *conductor).area();
        netIdToConductors[conductor->netId].emplace_back(conductor);
    }

    // handle the area of overlapping conductors
    for (const auto &[_, conductors] : netIdToConductors)
    {
        if (conductors.size() <= 1)
            continue;

        std::vector<std::pair<geometry::Rectangle, size_t>> regions; // (boundary, index in conductor vector)
        for (size_t i = 0; i < conductors.size(); ++i)
            regions.emplace_back(geometry::getIntersectRegion(tile, *conductors[i]), i);

        // inclusion-exclusion principle
        int64_t sign = -1;
        while (!regions.empty())
        {
            std::vector<std::pair<geometry::Rectangle, size_t>> intersectRegions;
            for (const auto &[region, idx] : regions)
            {
                for (size_t i = idx + 1; i < conductors.size(); ++i)
                {
                    auto intersectRegion = geometry::getIntersectRegion(region, *conductors[i]);
                    if (intersectRegion.area() == 0)
                        continue;

                    intersectRegions.emplace_back(intersectRegion, i);
                    conductorArea += sign * intersectRegion.area();
                }
            }
            sign = -sign;
            regions.swap(intersectRegions);
        };
    }
    return conductorArea;
}

void DensityManager::initProcessLayer(process::Layer *layer_)
{
    layer = layer_;
    double halfSpacing = static_cast<double>(layer->minSpacing) / 2;
    lowerLeftSpacing = std::floor(halfSpacing);
    upperRightSpacing = std::ceil(halfSpacing);
    minMetalAreaConstraint = std::ceil(windowArea * layer->minMetalDensity);
    maxMetalAreaConstraint = std::floor(windowArea * layer->maxMetalDensity);
}

void DensityManager::initGrid()
{
    allCandidateRegions.clear();
    allCandidateRegions.shrink_to_fit();

    allFillers.clear();
    allFillers.shrink_to_fit();

    tileGrid.clear();
    tileGrid.shrink_to_fit();
    tileGrid.resize(numTileRow, std::vector<process::Tile>(numTileCol));
    for (size_t rowIdx = 0; rowIdx < numTileRow; ++rowIdx)
    {
        for (size_t colIdx = 0; colIdx < numTileCol; ++colIdx)
        {
            auto [tileX, tileY] = getTilePos(rowIdx, colIdx);
            tileGrid[rowIdx][colIdx].setCoordinates(tileX, tileY, tileX + tileSize, tileY + tileSize);
        }
    }

    windowGrid.clear();
    windowGrid.shrink_to_fit();
    windowGrid.resize(numWindowRow, std::vector<int64_t>(numWindowCol));
    for (size_t rowIdx = 0; rowIdx < numWindowRow; ++rowIdx)
        for (size_t colIdx = 0; colIdx < numWindowCol; ++colIdx)
            for (size_t r = 0; r < numTileForWindow; ++r)
                for (size_t c = 0; c < numTileForWindow; ++c)
                    tileGrid[rowIdx + r][colIdx + c].windows.emplace_back(&windowGrid[rowIdx][colIdx]);

    // add conductor to intersecting tiles
    for (const auto &conductor : layer->conductors)
    {
        size_t beginRowIdx, beginColIdx, endRowIdx, endColIdx;
        std::tie(beginRowIdx, beginColIdx, endRowIdx, endColIdx) = getTileIdx(*conductor);
        for (size_t rowIdx = beginRowIdx; rowIdx < endRowIdx; ++rowIdx)
            for (size_t colIdx = beginColIdx; colIdx < endColIdx; ++colIdx)
                tileGrid[rowIdx][colIdx].conductors.emplace_back(conductor.get());
    }

    // calculate the total area occupied by conductors in each tile
    for (auto &row : tileGrid)
        for (auto &tile : row)
            tile.conductorArea = getConductorArea(tile);

    updateAllWindowMetalArea();
}

void DensityManager::recordFreeRegion(geometry::Rectangle *freeRegion)
{
    size_t beginRowIdx, beginColIdx, endRowIdx, endColIdx;
    std::tie(beginRowIdx, beginColIdx, endRowIdx, endColIdx) = getTileIdx(*freeRegion);
    for (size_t rowIdx = beginRowIdx; rowIdx < endRowIdx; ++rowIdx)
        for (size_t colIdx = beginColIdx; colIdx < endColIdx; ++colIdx)
            tileGrid[rowIdx][colIdx].candidateRegions.emplace_back(freeRegion);
}

void DensityManager::insertFiller(process::Filler *filler)
{
    size_t beginRowIdx, beginColIdx, endRowIdx, endColIdx;
    std::tie(beginRowIdx, beginColIdx, endRowIdx, endColIdx) = getTileIdx(*filler);
    for (size_t rowIdx = beginRowIdx; rowIdx < endRowIdx; ++rowIdx)
    {
        for (size_t colIdx = beginColIdx; colIdx < endColIdx; ++colIdx)
        {
            auto &tile = tileGrid[rowIdx][colIdx];
            tile.candidateFillerSet.erase(filler);
            tile.fillerSet.emplace(filler);
            auto area = geometry::getIntersectRegion(tile, *filler).area();
            tile.fillerArea += area;
            for (auto window : tile.windows)
                *window += area;
        }
    }
}

void DensityManager::removeFiller(process::Filler *filler)
{
    size_t beginRowIdx, beginColIdx, endRowIdx, endColIdx;
    std::tie(beginRowIdx, beginColIdx, endRowIdx, endColIdx) = getTileIdx(*filler);
    for (size_t rowIdx = beginRowIdx; rowIdx < endRowIdx; ++rowIdx)
    {
        for (size_t colIdx = beginColIdx; colIdx < endColIdx; ++colIdx)
        {
            auto &tile = tileGrid[rowIdx][colIdx];
            tile.fillerSet.erase(filler);
            tile.candidateFillerSet.emplace(filler);
            auto area = geometry::getIntersectRegion(tile, *filler).area();
            tile.fillerArea -= area;
            for (auto window : tile.windows)
                *window -= area;
        }
    }
}

std::vector<geometry::Rectangle> DensityManager::getAllFreeRegion(size_t rowIdx, size_t colIdx) const
{
    geometry::Rectangle boundary;
    std::vector<geometry::Rectangle> conductors;
    if (rowIdx == numTileRow && colIdx == numTileCol)
    {
        boundary = db->chipBoundary;
        for (const auto &conductor : layer->conductors)
        {
            geometry::Rectangle newConductor(*conductor.get());
            newConductor.expand(lowerLeftSpacing, upperRightSpacing);
            conductors.emplace_back(newConductor);
        }
    }
    else
    {
        boundary = tileGrid[rowIdx][colIdx];
        std::unordered_set<process::Conductor *> conductorSet;
        size_t beginRowIdx = (rowIdx > 0) ? rowIdx - 1 : rowIdx;
        size_t beginColIdx = (colIdx > 0) ? colIdx - 1 : colIdx;
        size_t endRowIdx = (rowIdx + 1 < numTileRow) ? rowIdx + 1 : rowIdx;
        size_t endColIdx = (colIdx + 1 < numTileCol) ? colIdx + 1 : colIdx;
        geometry::Rectangle extendBoundary(boundary);
        extendBoundary.expand(upperRightSpacing, lowerLeftSpacing);
        for (size_t r = beginRowIdx; r <= endRowIdx; ++r)
            for (size_t c = beginColIdx; c <= endColIdx; ++c)
                for (const auto &conductor : tileGrid[r][c].conductors)
                    if (geometry::isIntersect(extendBoundary, *conductor))
                        conductorSet.emplace(conductor);
        for (const auto conductor : conductorSet)
        {
            geometry::Rectangle newConductor(*conductor);
            newConductor.expand(lowerLeftSpacing, upperRightSpacing);
            conductors.emplace_back(newConductor);
        }
    }

    if (layer->direction == process::Layer::Direction::VERTICAL)
    {
        boundary.transform();
        for (auto &conductor : conductors)
            conductor.transform();
    }

    std::map<int64_t, std::pair<std::vector<geometry::Rectangle *>,
                                std::vector<geometry::Rectangle *>>>
        conductorSweepLines; // {x, (right borders, left borders)}
    conductorSweepLines[boundary.x1];
    conductorSweepLines[boundary.x2];
    for (auto &conductor : conductors)
    {
        conductorSweepLines[conductor.x1].second.emplace_back(&conductor); // left border
        conductorSweepLines[conductor.x2].first.emplace_back(&conductor);  // rigth border
    }
    auto cmp = [](const geometry::Rectangle *a, geometry::Rectangle *b) -> bool
    {
        if (a->y1 != b->y1)
            return a->y1 < b->y1;
        else if (a->y2 != b->y2)
            return a->y2 < b->y2;
        else if (a->x1 != b->x1)
            return a->x1 < b->x1;
        else
            return a->x2 < b->x2;
    };
    std::set<geometry::Rectangle *, decltype(cmp)> cutConductorSet(cmp); // save conductors that cut by the current sweep line

    int64_t minRegionWidth = 1;
    std::vector<geometry::Rectangle> freeRegions;
    std::unordered_set<geometry::Rectangle *> tempRegionSet;
    for (const auto &[x, borders] : conductorSweepLines)
    {
        for (const auto conductor : borders.first)
            cutConductorSet.erase(conductor);
        for (const auto conductor : borders.second)
            cutConductorSet.emplace(conductor);

        if (boundary.x1 <= x && x < boundary.x2)
        {
            std::set<std::pair<int64_t, int64_t>> freeIntervalSet;
            int64_t maxY = boundary.y1;
            for (const auto conductor : cutConductorSet)
            {
                if (conductor->y1 - maxY >= minRegionWidth)
                    freeIntervalSet.emplace(maxY, conductor->y1);
                maxY = std::max(maxY, conductor->y2);
            }
            if (boundary.y2 - maxY >= minRegionWidth)
                freeIntervalSet.emplace(maxY, boundary.y2);

            for (auto it = tempRegionSet.begin(); it != tempRegionSet.end();)
            {
                auto tempRegion = *it;
                if (freeIntervalSet.count({tempRegion->y1, tempRegion->y2}))
                {
                    freeIntervalSet.erase({tempRegion->y1, tempRegion->y2});
                    ++it;
                }
                else
                {
                    tempRegion->x2 = x;
                    if (tempRegion->width() >= minRegionWidth)
                        freeRegions.emplace_back(*tempRegion);
                    delete tempRegion;
                    it = tempRegionSet.erase(it);
                }
            }
            for (auto [y1, y2] : freeIntervalSet)
                tempRegionSet.emplace(new geometry::Rectangle(x, y1, 0, y2));
        }
        else if (x == boundary.x2)
        {
            for (auto tempRegion : tempRegionSet)
            {
                tempRegion->x2 = x;
                if (tempRegion->width() >= minRegionWidth)
                    freeRegions.emplace_back(*tempRegion);
                delete tempRegion;
            }
            break;
        }
    }

    if (layer->direction == process::Layer::Direction::VERTICAL)
        for (auto &freeRegion : freeRegions)
            freeRegion.transform();
    return freeRegions;
}

std::vector<geometry::Rectangle> DensityManager::refineFreeRegion(const std::vector<geometry::Rectangle> &freeRegions) const
{
    int64_t minRegionWidth = layer->minFillWidth + lowerLeftSpacing + upperRightSpacing;
    std::map<int64_t, std::pair<std::unordered_set<process::sweepline::Region *>,
                                std::unordered_set<process::sweepline::Region *>>>
        regionSweepLines; // {x, (right border, left border)}

    for (auto freeRegion : freeRegions)
    {
        if (layer->direction == process::Layer::Direction::VERTICAL)
            freeRegion.transform();

        if (freeRegion.height() < minRegionWidth)
            continue;

        auto region = new process::sweepline::Region(freeRegion, freeRegion.width() >= minRegionWidth);
        regionSweepLines[region->x1].second.emplace(region); // left border
        regionSweepLines[region->x2].first.emplace(region);  // right border
    }

    for (auto &[_, borders] : regionSweepLines)
    {
        for (auto formerIt = borders.first.begin(); formerIt != borders.first.end(); ++formerIt)
        {
            auto former = *formerIt;
            for (auto latterIt = borders.second.begin(); latterIt != borders.second.end();)
            {
                auto latter = *latterIt;
                if (former->isLegal && latter->isLegal)
                {
                    if (former->y1 == latter->y1 && former->y2 == latter->y2)
                    {
                        former->x2 = latter->x2;
                        regionSweepLines[former->x2].first.emplace(former);
                        regionSweepLines[latter->x1].second.erase(latter);
                        regionSweepLines[latter->x2].first.erase(latter);
                        delete latter;
                        break;
                    }
                }
                else if (former->isLegal && !latter->isLegal)
                {
                    if (latter->y1 <= former->y1 && former->y2 <= latter->y2)
                    {
                        former->x2 = latter->x2;
                        regionSweepLines[former->x2].first.emplace(former);
                        if (former->y1 - latter->y1 >= minRegionWidth)
                        {
                            auto newLatter = new process::sweepline::Region(*latter);
                            newLatter->y2 = former->y1;
                            regionSweepLines[newLatter->x1].second.emplace(newLatter);
                            regionSweepLines[newLatter->x2].first.emplace(newLatter);
                        }
                        if (latter->y2 - former->y2 >= minRegionWidth)
                        {
                            latter->y1 = former->y2;
                        }
                        else
                        {
                            regionSweepLines[latter->x1].second.erase(latter);
                            regionSweepLines[latter->x2].first.erase(latter);
                        }
                        break;
                    }
                }
                else if (!former->isLegal && latter->isLegal)
                {
                    if (former->y1 <= latter->y1 && latter->y2 <= former->y2)
                    {
                        latter->x1 = former->x1;
                        regionSweepLines[latter->x1].second.emplace(latter);
                        latterIt = borders.second.erase(latterIt);
                        continue;
                    }
                }
                ++latterIt;
            }
        }
    }

    std::vector<geometry::Rectangle> refinedRegions;
    for (auto &[_, borders] : regionSweepLines)
    {
        for (auto region : borders.second)
        {
            if (region->width() >= minRegionWidth && region->height() >= minRegionWidth)
                refinedRegions.emplace_back(*region);
            delete region;
        }
    }

    if (layer->direction == process::Layer::Direction::VERTICAL)
        for (auto &refinedRegion : refinedRegions)
            refinedRegion.transform();
    return refinedRegions;
}

std::vector<geometry::Rectangle> DensityManager::filterIllegalRegion(const std::vector<geometry::Rectangle> &freeRegions) const
{
    int64_t minRegionWidth = layer->minFillWidth + lowerLeftSpacing + upperRightSpacing;
    std::vector<geometry::Rectangle> legalRegions;
    for (const auto &freeRegion : freeRegions)
    {
        if (freeRegion.width() >= minRegionWidth && freeRegion.height() >= minRegionWidth)
            legalRegions.emplace_back(freeRegion);
    }
    return legalRegions;
}

std::vector<geometry::Rectangle> DensityManager::generateAllFiller(const std::vector<geometry::Rectangle> &freeRegions) const
{
    std::vector<geometry::Rectangle> fillers;
    for (const auto &freeRegion : freeRegions)
    {
        size_t numRow = std::ceil(static_cast<double>(freeRegion.height()) / layer->maxFillWidth);
        size_t numCol = std::ceil(static_cast<double>(freeRegion.width()) / layer->maxFillWidth);
        int64_t height = freeRegion.height() / numRow;
        int64_t width = freeRegion.width() / numCol;
        for (size_t row = 0; row < numRow; ++row)
        {
            for (size_t col = 0; col < numCol; ++col)
            {
                int64_t x1 = freeRegion.x1 + col * width;
                int64_t y1 = freeRegion.y1 + row * height;
                geometry::Rectangle filler(x1, y1, x1 + width, y1 + height);
                filler.expand(-lowerLeftSpacing, -upperRightSpacing);
                fillers.emplace_back(filler);
            }
        }
    }
    return fillers;
}

void DensityManager::removeCriticalNetFiller()
{
    std::unordered_set<process::Filler *> candidateRemoveSet;
    for (const auto &criticalConductor : layer->conductors)
    {
        if (!criticalConductor->isCritical)
            continue;

        geometry::Rectangle boundary(*criticalConductor);
        boundary.expand(layer->minSpacing * 2, layer->minSpacing * 2);

        size_t beginRowIdx, beginColIdx, endRowIdx, endColIdx;
        std::tie(beginRowIdx, beginColIdx, endRowIdx, endColIdx) = getTileIdx(*criticalConductor);
        beginRowIdx = (beginRowIdx > 0) ? beginRowIdx - 1 : beginRowIdx;
        beginColIdx = (beginColIdx > 0) ? beginColIdx - 1 : beginColIdx;
        endRowIdx = (endRowIdx + 1 < numTileRow) ? endRowIdx + 1 : endRowIdx;
        endColIdx = (endColIdx + 1 < numTileCol) ? endColIdx + 1 : endColIdx;
        for (size_t rowIdx = beginRowIdx; rowIdx < endRowIdx; ++rowIdx)
        {
            for (size_t colIdx = beginColIdx; colIdx < endColIdx; ++colIdx)
            {
                auto &tile = tileGrid[rowIdx][colIdx];
                for (const auto filler : tile.fillerSet)
                {
                    if (!geometry::isIntersect(boundary, *filler))
                        continue;

                    candidateRemoveSet.emplace(filler);
                    filler->cost += static_cast<double>(geometry::getParallelLength(*criticalConductor, *filler)) /
                                    geometry::getDistance(*criticalConductor, *filler);
                }
            }
        }
    }

    std::vector<process::Filler *> candidateRemove(candidateRemoveSet.begin(), candidateRemoveSet.end());
    std::sort(candidateRemove.begin(), candidateRemove.end(), [](const process::Filler *a, const process::Filler *b)
              { return a->cost > b->cost || (a->cost == b->cost && a->area() < b->area()); });
    for (const auto &filler : candidateRemove)
    {
        removeFiller(filler);
        if (getMinMaxWindowMetalArea().first < minMetalAreaConstraint)
            insertFiller(filler);
    }
}

void DensityManager::meetDensityConstraint()
{
    auto [minMetalArea, maxMatelArea] = getMinMaxWindowMetalArea();
    if (minMetalArea >= minMetalAreaConstraint && maxMatelArea <= maxMetalAreaConstraint)
        return;

    for (auto &row : tileGrid)
    {
        for (auto &tile : row)
        {
            int64_t minOccupyArea = windowArea;
            int64_t maxOccupyArea = 0;
            for (const auto occupyArea : tile.windows)
            {
                minOccupyArea = std::min(minOccupyArea, *occupyArea);
                maxOccupyArea = std::max(maxOccupyArea, *occupyArea);
            }

            if (maxOccupyArea <= maxMetalAreaConstraint)
                continue;

            int64_t maxRemoveArea = minOccupyArea - minMetalAreaConstraint;
            int64_t minRemoveArea = maxOccupyArea - maxMetalAreaConstraint;

            int64_t removeArea = 0;
            std::vector<process::Filler *> fillers(tile.fillerSet.begin(), tile.fillerSet.end());
            std::sort(fillers.begin(), fillers.end(), [](const process::Filler *a, const process::Filler *b)
                      { return a->area() < b->area(); });
            for (const auto &filler : fillers)
            {
                if (removeArea >= minRemoveArea)
                    break;

                if (filler->inTile)
                {
                    int64_t area = filler->area();
                    if (removeArea + area <= maxRemoveArea)
                    {
                        removeFiller(filler);
                        removeArea += area;
                    }
                }
                else
                {
                    int64_t area = geometry::getIntersectRegion(tile, *filler).area();
                    if (removeArea + area <= maxRemoveArea)
                    {
                        if (getMinMaxWindowMetalArea().second > maxMetalAreaConstraint)
                        {
                            removeFiller(filler);
                            removeArea += area;
                        }
                        if (getMinMaxWindowMetalArea().first < minMetalAreaConstraint)
                        {
                            insertFiller(filler);
                            removeArea -= area;
                        }
                    }
                }
            }
        }
    }
}

void DensityManager::removeMoreFiller()
{
    for (auto &row : tileGrid)
    {
        for (auto &tile : row)
        {
            int64_t minOccupyArea = windowArea;
            for (const auto occupyArea : tile.windows)
                minOccupyArea = std::min(minOccupyArea, *occupyArea);

            int64_t maxRemoveArea = minOccupyArea - minMetalAreaConstraint;
            int64_t removeArea = 0;
            std::vector<process::Filler *> fillers(tile.fillerSet.begin(), tile.fillerSet.end());
            std::sort(fillers.begin(), fillers.end(), [](const process::Filler *a, const process::Filler *b)
                      { return a->area() < b->area(); });
            for (const auto &filler : fillers)
            {
                if (filler->inTile)
                {
                    int64_t area = filler->area();
                    if (removeArea + area <= maxRemoveArea)
                    {
                        removeFiller(filler);
                        removeArea += area;
                    }
                }
                else
                {
                    int64_t area = geometry::getIntersectRegion(tile, *filler).area();
                    if (removeArea + area <= maxRemoveArea)
                    {
                        if (getMinMaxWindowMetalArea().second > maxMetalAreaConstraint)
                        {
                            removeFiller(filler);
                            removeArea += area;
                        }
                        if (getMinMaxWindowMetalArea().first < minMetalAreaConstraint)
                        {
                            insertFiller(filler);
                            removeArea -= area;
                        }
                    }
                }
            }
        }
    }
}

int64_t DensityManager::getOccupyAreaBruteForce(const process::Tile &tile) const
{
    std::vector<std::vector<bool>> detailGird(tileSize, std::vector<bool>(tileSize, false));
    for (const auto &conductor : layer->conductors)
    {
        if (!geometry::isIntersect(tile, *conductor))
            continue;

        auto region = geometry::getIntersectRegion(tile, *conductor);
        region.shift(-tile.x1, -tile.y1);
        for (int64_t y = region.y1; y < region.y2; ++y)
            for (int64_t x = region.x1; x < region.x2; ++x)
                detailGird[y][x] = true;
    }

    std::unordered_set<process::Filler *> fillerSet;
    for (const auto &row : tileGrid)
        for (const auto &tile : row)
            for (const auto filler : tile.fillerSet)
                fillerSet.emplace(filler);

    for (const auto filler : fillerSet)
    {
        if (!geometry::isIntersect(tile, *filler))
            continue;

        auto region = geometry::getIntersectRegion(tile, *filler);
        region.shift(-tile.x1, -tile.y1);
        for (int64_t y = region.y1; y < region.y2; ++y)
            for (int64_t x = region.x1; x < region.x2; ++x)
                detailGird[y][x] = true;
    }

    int64_t area = 0;
    for (const auto &row : detailGird)
        for (auto col : row)
            if (col)
                ++area;
    return area;
}

void DensityManager::drawBorder(std::vector<std::vector<char>> &detailGrid, const geometry::Rectangle &boundary,
                                const geometry::Rectangle &region, double scaling, char h, char v) const
{
    geometry::Rectangle rectangle(region);
    rectangle.shift(-boundary.x1, -boundary.y1);
    rectangle.scale(scaling);

    if (!rectangle.isLegal())
        return;

    for (int64_t y = rectangle.y1; y < rectangle.y2; ++y)
        detailGrid[y][rectangle.x1] = detailGrid[y][rectangle.x2 - 1] = v;
    for (int64_t x = rectangle.x1; x < rectangle.x2; ++x)
        detailGrid[rectangle.y1][x] = detailGrid[rectangle.y2 - 1][x] = h;
}

void DensityManager::drawRegion(std::vector<std::vector<char>> &detailGrid, const geometry::Rectangle &boundary,
                                const geometry::Rectangle &region, double scaling, char c) const
{
    geometry::Rectangle rectangle(region);
    rectangle.shift(-boundary.x1, -boundary.y1);
    rectangle.scale(scaling);

    if (!rectangle.isLegal())
        return;

    for (int64_t y = rectangle.y1; y < rectangle.y2; ++y)
        for (int64_t x = rectangle.x1; x < rectangle.x2; ++x)
            detailGrid[y][x] = c;
}

void DensityManager::drawTile(std::ostream &output, size_t rowIdx, size_t colIdx, bool drawFiller, double scaling) const
{
    assert(rowIdx < numTileRow && colIdx < numTileCol);
    const auto &tile = tileGrid[rowIdx][colIdx];
    std::vector<std::vector<char>> detailGrid(tile.height() * scaling,
                                              std::vector<char>(tile.width() * scaling, ' '));
    drawBorder(detailGrid, tile, tile, scaling);

    for (const auto conductor : tile.conductors)
        if (!conductor->isCritical)
            drawBorder(detailGrid, tile, geometry::getIntersectRegion(tile, *conductor), scaling, '.', '.');
        else
            drawRegion(detailGrid, tile, geometry::getIntersectRegion(tile, *conductor), scaling, '.');

    if (drawFiller)
    {
        for (const auto filler : tile.fillerSet)
            drawBorder(detailGrid, tile, geometry::getIntersectRegion(tile, *filler), scaling, '#', '#');
    }
    else
    {
        for (const auto candidateRegion : tile.candidateRegions)
            drawBorder(detailGrid, tile, geometry::getIntersectRegion(tile, *candidateRegion), scaling, '#', '#');
    }

    output << "Layer id:           " << layer->id << "\n"
           << "Tile row/col index: " << rowIdx << " " << colIdx << "\n"
           << "Density:            " << tile.density() << "\n"
           << "#conductors:        " << tile.conductors.size() << "\n";
    if (drawFiller)
        output << "#fillers:           " << tile.fillerSet.size() << "\n";
    else
        output << "#candidate regions: " << tile.candidateRegions.size() << "\n";
    for (auto rowIt = detailGrid.rbegin(); rowIt != detailGrid.rend(); ++rowIt)
    {
        for (auto col : *rowIt)
            output << col;
        output << "\n";
    }
}

void DensityManager::drawWindow(std::ostream &output, size_t rowIdx, size_t colIdx, bool drawFiller, double scaling) const
{
    assert(rowIdx < numWindowRow && colIdx < numWindowCol);
    const auto &[windowX, windowY] = getTilePos(rowIdx, colIdx);
    geometry::Rectangle window(windowX, windowY, windowX + db->windowSize, windowY + db->windowSize);
    std::vector<std::vector<char>> detailGrid(window.height() * scaling,
                                              std::vector<char>(window.width() * scaling, ' '));

    drawBorder(detailGrid, window, window, scaling);

    std::unordered_set<process::Conductor *> conductors;
    std::unordered_set<geometry::Rectangle *> fillers, candidateRegions;
    for (size_t r = 0; r < numTileForWindow; ++r)
    {
        for (size_t c = 0; c < numTileForWindow; ++c)
        {
            const auto &tile = tileGrid[rowIdx + r][colIdx + c];
            for (const auto conductor : tile.conductors)
            {
                conductors.emplace(conductor);
                if (!conductor->isCritical)
                    drawBorder(detailGrid, window, geometry::getIntersectRegion(window, *conductor), scaling, '.', '.');
                else
                    drawRegion(detailGrid, window, geometry::getIntersectRegion(window, *conductor), scaling, '.');
            }
            if (drawFiller)
            {
                for (const auto filler : tile.fillerSet)
                {
                    fillers.emplace(filler);
                    drawBorder(detailGrid, window, geometry::getIntersectRegion(window, *filler), scaling, '#', '#');
                }
            }
            else
            {
                for (const auto candidateRegion : tile.candidateRegions)
                {
                    candidateRegions.emplace(candidateRegion);
                    drawBorder(detailGrid, window, geometry::getIntersectRegion(window, *candidateRegion), scaling, '#', '#');
                }
            }
            drawBorder(detailGrid, window, tile, scaling);
        }
    }

    output << "Layer id:             " << layer->id << "\n"
           << "Window row/col index: " << rowIdx << " " << colIdx << "\n"
           << "Density:              " << static_cast<double>(windowGrid[rowIdx][colIdx]) / windowArea << "\n"
           << "#conductors:          " << conductors.size() << "\n";
    if (drawFiller)
        output << "#fillers:             " << fillers.size() << "\n";
    else
        output << "#candidate regions:   " << candidateRegions.size() << "\n";
    for (auto rowIt = detailGrid.rbegin(); rowIt != detailGrid.rend(); ++rowIt)
    {
        for (auto col : *rowIt)
            output << col;
        output << "\n";
    }
}

DensityManager::DensityManager(process::Database *db_, size_t numTileForWindow_)
    : db(db_), numTileForWindow(numTileForWindow_),
      tileSize(db->windowSize / numTileForWindow),
      tileArea(tileSize * tileSize),
      windowArea(db->windowSize * db->windowSize),
      numTileRow(db->chipBoundary.height() / tileSize),
      numTileCol(db->chipBoundary.width() / tileSize),
      numWindowRow(numTileRow - numTileForWindow + 1),
      numWindowCol(numTileCol - numTileForWindow + 1)
{
    std::cout << "----- TILE GRID INFORMATION -----\n"
              << "Window size:     " << db->windowSize << "\n"
              << "Tile size:       " << tileSize << "\n"
              << "#tile row/col:   " << numTileRow << " " << numTileCol << "\n"
              << "#window row/col: " << numWindowRow << " " << numWindowCol << "\n"
              << "\n";
}

ResultWriter::ptr DensityManager::solve()
{
    auto resultWriter = new ResultWriter();
    for (const auto &curLayer : db->layers)
    {
        initProcessLayer(curLayer.get());
        std::cout << "----- LAYER " << layer->id << " -----\n"
                  << "Layer Direction:                      " << layer->directionName() << "\n";
        printf("Min/Max density constraint:           %.4lf %.4lf\n", layer->minMetalDensity, layer->maxMetalDensity);

        initGrid();
        auto minMaxDensity = getMinMaxWindowMetalDensity();
        printf("Min/Max density (original):           %.4lf %.4lf\n", minMaxDensity.first, minMaxDensity.second);

        for (size_t rowIdx = 0; rowIdx < numTileRow; ++rowIdx)
        {
            for (size_t colIdx = 0; colIdx < numTileCol; ++colIdx)
            {
                auto freeRegions = getAllFreeRegion(rowIdx, colIdx);
                freeRegions = refineFreeRegion(freeRegions);
                for (const auto &freeRegion : freeRegions)
                {
                    auto newFreeRegion = new geometry::Rectangle(freeRegion);
                    allCandidateRegions.emplace_back(newFreeRegion);
                    recordFreeRegion(newFreeRegion);
                }

                auto fillers = generateAllFiller(freeRegions);
                for (const auto &filler : fillers)
                {
                    auto newFiller = new process::Filler(filler, true);
                    allFillers.emplace_back(newFiller);
                    insertFiller(newFiller);
                }
            }
        }

        if (getMinMaxWindowMetalArea().first < minMetalAreaConstraint)
        {
            initGrid();

            auto freeRegions = getAllFreeRegion(numTileRow, numTileCol);
            freeRegions = refineFreeRegion(freeRegions);
            for (const auto &freeRegion : freeRegions)
            {
                auto newFreeRegion = new geometry::Rectangle(freeRegion);
                allCandidateRegions.emplace_back(newFreeRegion);
                recordFreeRegion(newFreeRegion);
            }

            auto fillers = generateAllFiller(freeRegions);
            for (const auto &filler : fillers)
            {
                auto newFiller = new process::Filler(filler, false);
                allFillers.emplace_back(newFiller);
                insertFiller(newFiller);
            }
        }
        minMaxDensity = getMinMaxWindowMetalDensity();
        printf("Min/Max density (fill all fillers):   %.4lf %.4lf\n", minMaxDensity.first, minMaxDensity.second);

        removeCriticalNetFiller();
        minMaxDensity = getMinMaxWindowMetalDensity();
        printf("Min/Max density (reduce capacitance): %.4lf %.4lf\n", minMaxDensity.first, minMaxDensity.second);

        meetDensityConstraint();
        minMaxDensity = getMinMaxWindowMetalDensity();
        printf("Min/Max density (meet metal density): %.4lf %.4lf\n", minMaxDensity.first, minMaxDensity.second);

        removeMoreFiller();
        minMaxDensity = getMinMaxWindowMetalDensity();
        printf("Min/Max density (reduce filler):      %.4lf %.4lf\n", minMaxDensity.first, minMaxDensity.second);

        std::cout << "\n";

        std::unordered_set<geometry::Rectangle *> fillerSet;
        for (const auto &row : tileGrid)
            for (const auto &tile : row)
                for (const auto filler : tile.fillerSet)
                    fillerSet.emplace(filler);
        for (const auto filler : fillerSet)
            resultWriter->addFiller(*filler, layer->id);
    }
    return std::unique_ptr<ResultWriter>(resultWriter);
}
