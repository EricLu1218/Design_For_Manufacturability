#pragma once
#include "../Geometry/Geometry.hpp"
#include <memory>

namespace raw
{
    struct Layer
    {
        using ptr = std::unique_ptr<Layer>;

        int64_t id, minFillWidth, maxFillWidth, minSpacing;
        double minMetalDensity, maxMetalDensity, weight;

        Layer() : id(0), minFillWidth(0), maxFillWidth(0), minSpacing(0),
                  minMetalDensity(0), maxMetalDensity(0), weight(0) {}
    };

    struct Conductor : geometry::Rectangle
    {
        using ptr = std::unique_ptr<Conductor>;

        int64_t id, netId, layerId;

        Conductor() : id(0), netId(0), layerId(0) {}
    };
}
