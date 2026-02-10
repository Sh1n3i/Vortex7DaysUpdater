#pragma once

#include "../Console/Console.h"
#include <string>
#include <vector>
#include <utility>

std::vector<std::pair<std::string, std::string>> ParseUpdateInfo(const std::string& text);
LogColor ColorForKey(const std::string& key);
