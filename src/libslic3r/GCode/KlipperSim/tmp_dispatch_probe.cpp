#include "KlipperSimMain.hpp"

#include <fstream>
#include <string>
#include <vector>

using namespace Slic3r::KlipperSim;

int main(int argc, char** argv)
{
    if (argc < 2)
        return 2;
    std::ifstream in(argv[1]);
    if (!in)
        return 1;
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line))
        lines.push_back(line + "\n");
    SimConfig cfg;
    LineAnalysisState st;
    analyze_gcode_lines(lines, cfg, 0.50, st);
    return 0;
}
