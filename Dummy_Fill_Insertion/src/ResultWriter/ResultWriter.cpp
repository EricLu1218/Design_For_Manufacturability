#include "ResultWriter.hpp"
#include <fstream>
#include <iostream>

ResultWriter::ResultWriter() {}

void ResultWriter::addFiller(const geometry::Rectangle &filler, int64_t layerId)
{
    layerToFillers[layerId].emplace_back(filler);
}

bool ResultWriter::write(const std::string &filepath) const
{
    std::ofstream fout(filepath);
    if (!fout.is_open())
    {
        std::cerr << "[Error] Cannot open \"" << filepath << "\".\n";
        return false;
    }

    for (const auto &[layerId, fillers] : layerToFillers)
        for (const auto &filler : fillers)
            fout << filler.dumpCoordinates() << " " << layerId << "\n";
    return true;
}
