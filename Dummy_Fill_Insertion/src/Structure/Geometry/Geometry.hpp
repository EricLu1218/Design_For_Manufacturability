#pragma once
#include <memory>
#include <string>

namespace geometry
{
    struct Rectangle
    {
        using ptr = std::unique_ptr<Rectangle>;

        int64_t x1, y1, x2, y2; // lower-left coordinate (x1, y1), upper-right coordinate (x2, y2)

        Rectangle();
        Rectangle(int64_t x1_, int64_t y1_, int64_t x2_, int64_t y2_);
        int64_t width() const;
        int64_t height() const;
        int64_t area() const;
        double aspectRatio() const;
        std::string dumpCoordinates() const;
        bool isLegal() const;
        Rectangle &shift(int64_t offsetX, int64_t offsetY);
        Rectangle &scale(double scaling = 1);
        Rectangle &expand(int64_t lowerLeft, int64_t upperRight);
        Rectangle &expand(int64_t left, int64_t lower, int64_t right, int64_t upper);
        Rectangle &transform();
    };

    // utility function
    bool isIntersect(const Rectangle &a, const Rectangle &b);
    Rectangle getIntersectRegion(const Rectangle &a, const Rectangle &b);
    int64_t getDistance(const Rectangle &a, const Rectangle &b);
    int64_t getParallelLength(const Rectangle &a, const Rectangle &b);
}
