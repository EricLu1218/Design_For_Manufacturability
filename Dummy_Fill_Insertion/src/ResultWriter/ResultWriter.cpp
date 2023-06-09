#include "ResultWriter.hpp"
#include <fstream>
#include <iostream>

ResultWriter::ResultWriter() {}

void ResultWriter::addFiller(const geometry::Rectangle &filler, int64_t layerId)
{
    layerToFillers[layerId].emplace_back(filler);
}

bool ResultWriter::write(const std::string &outputFile) const
{
    std::ofstream fout(outputFile);
    if (!fout)
    {
        std::cerr << "[Error] Cannot open \"" << outputFile << "\"!\n";
        return false;
    }

    for (const auto &[layerId, fillers] : layerToFillers)
        for (const auto &filler : fillers)
            fout << filler.dumpCoordinates() << " " << layerId << "\n";
    return true;
}
