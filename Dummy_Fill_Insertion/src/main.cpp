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
    if (!parser.parse(argParser.inputFilepath))
        return 1;
    process::Database::ptr db = parser.createDatabase();

    timer.stopTimer("parse input");
    timer.startTimer("processing");

    DensityManager densityManager(db.get());
    ResultWriter::ptr result = densityManager.solve();

    timer.stopTimer("processing");
    timer.startTimer("write output");

    result->write(argParser.outputFilepath);

    timer.stopTimer("write output");
    timer.stopTimer("runtime");

    timer.printTime("parse input");
    timer.printTime("processing");
    timer.printTime("write output");
    timer.printTime("runtime");
    return 0;
}
