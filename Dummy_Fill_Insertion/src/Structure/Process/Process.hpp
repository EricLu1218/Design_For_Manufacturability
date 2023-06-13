#pragma once
#include "../Geometry/Geometry.hpp"
#include "../Raw/Raw.hpp"
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace process
{
    struct Conductor : geometry::Rectangle
    {
        using ptr = std::unique_ptr<Conductor>;

        int64_t netId;
        bool isCritical;

        Conductor() : netId(0), isCritical(false) {}
        Conductor(const raw::Conductor *conductor) : netId(conductor->netId), isCritical(false)
        {
            x1 = conductor->x1;
            y1 = conductor->y1;
            x2 = conductor->x2;
            y2 = conductor->y2;
        }
    };

    struct Layer
    {
        enum class Direction
        {
            NONE = 0,
            HORIZONTAL,
            VERTICAL
        };

        using ptr = std::unique_ptr<Layer>;

        int64_t id, minFillWidth, maxFillWidth, minSpacing;
        double minMetalDensity, maxMetalDensity, weight;
        Direction direction;
        std::vector<Conductor::ptr> conductors;

        Layer() : minFillWidth(0), maxFillWidth(0), minSpacing(0),
                  minMetalDensity(0), maxMetalDensity(0), weight(0), direction(Direction::NONE) {}
        Layer(const raw::Layer &layer)
        {
            id = layer.id;
            minFillWidth = layer.minFillWidth;
            maxFillWidth = layer.maxFillWidth;
            minSpacing = layer.minSpacing;
            minMetalDensity = layer.minMetalDensity;
            maxMetalDensity = layer.maxMetalDensity;
            weight = layer.weight;
        }

        std::string directionName() const
        {
            if (direction == Direction::HORIZONTAL)
                return "Horizontal";
            else if (direction == Direction::VERTICAL)
                return "Vertical";
            else
                return "N/A";
        }
    };

    struct Database
    {
        using ptr = std::unique_ptr<Database>;

        geometry::Rectangle chipBoundary;
        int64_t windowSize;
        std::vector<Layer::ptr> layers;

        Database() : windowSize(0) {}
    };

    struct Filler : geometry::Rectangle
    {
        using ptr = std::unique_ptr<Filler>;

        double cost;
        bool inTile;

        Filler() : cost(0), inTile(false) {}
        Filler(const geometry::Rectangle &rectangle, bool inTile_) : cost(0), inTile(inTile_)
        {
            x1 = rectangle.x1;
            y1 = rectangle.y1;
            x2 = rectangle.x2;
            y2 = rectangle.y2;
        }
    };

    struct Tile : geometry::Rectangle
    {
        using ptr = std::unique_ptr<Tile>;

        int64_t conductorArea, fillerArea;
        std::vector<int64_t *> windows;
        std::vector<Conductor *> conductors;
        std::vector<geometry::Rectangle *> candidateRegions;
        std::unordered_set<Filler *> candidateFillerSet, fillerSet;

        Tile() : conductorArea(0), fillerArea(0) {}
        void setCoordinates(int64_t x1_, int64_t y1_, int64_t x2_, int64_t y2_)
        {
            x1 = x1_;
            y1 = y1_;
            x2 = x2_;
            y2 = y2_;
        }
        int64_t occupyArea() const
        {
            return conductorArea + fillerArea;
        }
        double density() const
        {
            return static_cast<double>(occupyArea()) / area();
        }
    };

    namespace sweepline
    {
        struct Region : geometry::Rectangle
        {
            using ptr = std::unique_ptr<Region>;

            bool isLegal;

            Region() : isLegal(true) {}
            Region(const geometry::Rectangle &rectangle, bool isLegal)
                : isLegal(isLegal)
            {
                x1 = rectangle.x1;
                y1 = rectangle.y1;
                x2 = rectangle.x2;
                y2 = rectangle.y2;
            }
        };
    }
}
