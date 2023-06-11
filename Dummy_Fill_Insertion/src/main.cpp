#include "DensityManager/DensityManager.hpp"
#include "Parser/ArgumentParser.hpp"
#include "Parser/Parser.hpp"
#include "Timer/Timer.hpp"

int main(int argc, char *argv[])
{
    ArgumentParser argParser;
    if (!argParser.parse(argc, argv))
        return 1;

    Timer timer(10 * 60 - 10);
    timer.startTimer("runtime");
    timer.startTimer("parse input");

    Parser parser;
    parser.parse(argParser.inputFile);
    auto db = parser.createDatabase();

    timer.stopTimer("parse input");
    timer.startTimer("processing");

    DensityManager densityManager(db.get());
    auto result = densityManager.solve();

    timer.stopTimer("processing");
    timer.startTimer("write output");

    result->write(argParser.outputFile);

    timer.stopTimer("write output");
    timer.stopTimer("runtime");

    timer.printTime("parse input");
    timer.printTime("processing");
    timer.printTime("write output");
    timer.printTime("runtime");
    return 0;
}
