#pragma once
#include "../Structure/Geometry/Geometry.hpp"
#include <map>
#include <memory>
#include <string>
#include <vector>

class ResultWriter
{
    std::map<int64_t, std::vector<geometry::Rectangle>> layerToFillers;

public:
    using ptr = std::unique_ptr<ResultWriter>;

    ResultWriter();
    void addFiller(const geometry::Rectangle &filler, int64_t layerId);
    bool write(const std::string &outputFile) const;
};
