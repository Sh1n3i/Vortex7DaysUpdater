#include "Parser.h"
#include <sstream>

std::vector<std::pair<std::string, std::string>> ParseUpdateInfo(const std::string& text)
{
    std::vector<std::pair<std::string, std::string>> entries;
    std::istringstream stream(text);
    std::string line;

    while (std::getline(stream, line))
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        auto pos = line.find(':');
        if (pos == std::string::npos || pos == 0)
            continue;

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        size_t start = value.find_first_not_of(' ');
        if (start != std::string::npos)
            value = value.substr(start);

        if (value.find("//") == 0 || value.find("https") != std::string::npos ||
            value.find("http") != std::string::npos)
        {
            value = line.substr(pos + 1);
            start = value.find_first_not_of(' ');
            if (start != std::string::npos)
                value = value.substr(start);
        }

        entries.emplace_back(key, value);
    }
    return entries;
}

LogColor ColorForKey(const std::string& key)
{
    if (key == "Version")  return LogColor::Green;
    if (key == "Date")     return LogColor::Yellow;
    if (key == "Telegram") return LogColor::Cyan;
    if (key == "Steam")    return LogColor::Cyan;
    if (key == "Url")      return LogColor::Magenta;
    return LogColor::White;
}
