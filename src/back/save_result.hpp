#pragma once
#include "statistics.hpp"
#include <string>
#include <vector>

using std::string;
using std::vector;

// void saveHTMLWithAdvancedSearch(const std::vector<SearchItem> &items,
//                                const std::string &filename);
// std::string generateHTMLCharts(const GlobalStats &stats);

void saveAnalysis(const std::string path, std::vector<SearchItem> &items,
                  GlobalStats &stats);
