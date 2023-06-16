#include "Geometry.hpp"
#include <limits>
#include <sstream>

geometry::Rectangle::Rectangle()
    : x1(0), y1(0), x2(0), y2(0) {}

geometry::Rectangle::Rectangle(int64_t x1_, int64_t y1_, int64_t x2_, int64_t y2_)
    : x1(x1_), y1(y1_), x2(x2_), y2(y2_) {}

int64_t geometry::Rectangle::width() const
{
    return x2 - x1;
}

int64_t geometry::Rectangle::height() const
{
    return y2 - y1;
}

int64_t geometry::Rectangle::area() const
{
    return width() * height();
}

double geometry::Rectangle::aspectRatio() const
{
    if (height() == 0)
        return std::numeric_limits<double>::max();
    else
        return static_cast<double>(width()) / height();
}

std::string geometry::Rectangle::dumpCoordinates() const
{
    std::stringstream ss;
    ss << x1 << " " << y1 << " " << x2 << " " << y2;
    return ss.str();
}

bool geometry::Rectangle::isLegal() const
{
    return width() > 0 && height() > 0;
}

geometry::Rectangle &geometry::Rectangle::shift(int64_t offsetX, int64_t offsetY)
{
    x1 += offsetX;
    y1 += offsetY;
    x2 += offsetX;
    y2 += offsetY;
    return *this;
}

geometry::Rectangle &geometry::Rectangle::scale(double scaling)
{
    x1 *= scaling;
    y1 *= scaling;
    x2 *= scaling;
    y2 *= scaling;
    return *this;
}

geometry::Rectangle &geometry::Rectangle::expand(int64_t lowerLeft, int64_t upperRight)
{
    x1 -= lowerLeft;
    y1 -= lowerLeft;
    x2 += upperRight;
    y2 += upperRight;
    return *this;
}

geometry::Rectangle &geometry::Rectangle::expand(int64_t left, int64_t lower, int64_t right, int64_t upper)
{
    x1 -= left;
    y1 -= lower;
    x2 += right;
    y2 += upper;
    return *this;
}

geometry::Rectangle &geometry::Rectangle::transform()
{
    std::swap(x1, y1);
    std::swap(x2, y2);
    return *this;
}

bool geometry::isIntersect(const geometry::Rectangle &a, const geometry::Rectangle &b)
{
    return !(a.x2 <= b.x1 || b.x2 <= a.x1 || a.y2 <= b.y1 || b.y2 <= a.y1);
}

geometry::Rectangle geometry::getIntersectRegion(const geometry::Rectangle &a, const geometry::Rectangle &b)
{
    if (!isIntersect(a, b))
        return geometry::Rectangle();

    int64_t beginX = std::max(a.x1, b.x1);
    int64_t beginY = std::max(a.y1, b.y1);
    int64_t endX = std::min(a.x2, b.x2);
    int64_t endY = std::min(a.y2, b.y2);
    return geometry::Rectangle(beginX, beginY, endX, endY);
}

int64_t geometry::getDistance(const geometry::Rectangle &a, const geometry::Rectangle &b)
{
    int64_t lenX = std::max(a.x1, b.x1) - std::min(a.x2, b.x2);
    int64_t lenY = std::max(a.y1, b.y1) - std::min(a.y2, b.y2);
    lenX = (lenX < 0) ? 0 : lenX;
    lenY = (lenY < 0) ? 0 : lenY;
    return lenX + lenY;
}

int64_t geometry::getParallelLength(const geometry::Rectangle &a, const geometry::Rectangle &b)
{
    int64_t lenX = std::min(a.x2, b.x2) - std::max(a.x1, b.x1);
    int64_t lenY = std::min(a.y2, b.y2) - std::max(a.y1, b.y1);
    if (lenX > 0 && lenY <= 0)
        return lenX;
    else if (lenX <= 0 && lenY > 0)
        return lenY;
    return 0;
}
