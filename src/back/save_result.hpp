#pragma once
#include <string>
#include <vector>
#include "statistics.hpp"

using std::string;
using std::vector;

void saveHTMLWithAdvancedSearch(const std::vector<SearchItem>& items, const std::string& filename);
std::string generateHTMLCharts(const GlobalStats& stats);