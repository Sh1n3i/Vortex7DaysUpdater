#include "Steam.h"
#include <Windows.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

// Read a registry string value. Returns empty on failure.
static std::wstring ReadRegistryString(HKEY root, const wchar_t* subKey, const wchar_t* valueName)
{
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(root, subKey, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return {};

    wchar_t buf[MAX_PATH]{};
    DWORD size = sizeof(buf);
    DWORD type = 0;
    LSTATUS status = RegQueryValueExW(hKey, valueName, nullptr, &type,
                                      reinterpret_cast<LPBYTE>(buf), &size);
    RegCloseKey(hKey);

    if (status != ERROR_SUCCESS || type != REG_SZ)
        return {};

    return std::wstring(buf);
}

// Get the Steam installation directory from the Windows registry.
static std::wstring GetSteamInstallDir()
{
    // Try 64-bit registry view first (most common on modern systems)
    std::wstring path = ReadRegistryString(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\WOW6432Node\\Valve\\Steam",
        L"InstallPath");

    if (!path.empty()) return path;

    // Fallback: current user
    path = ReadRegistryString(
        HKEY_CURRENT_USER,
        L"SOFTWARE\\Valve\\Steam",
        L"SteamPath");

    return path;
}

// Trim leading/trailing whitespace and quotes from a string.
static std::string TrimQuoted(const std::string& s)
{
    size_t start = s.find_first_not_of(" \t\"");
    if (start == std::string::npos) return {};
    size_t end = s.find_last_not_of(" \t\"");
    return s.substr(start, end - start + 1);
}

// Parse libraryfolders.vdf and return all Steam library paths.
static std::vector<std::wstring> GetSteamLibraryFolders(const std::wstring& steamDir)
{
    std::vector<std::wstring> folders;
    folders.push_back(steamDir);

    fs::path vdfPath = fs::path(steamDir) / "steamapps" / "libraryfolders.vdf";
    std::ifstream file(vdfPath);
    if (!file.is_open()) return folders;

    std::string line;
    while (std::getline(file, line))
    {
        // Look for "path" key: e.g.   "path"   "D:\\SteamLibrary"
        size_t keyPos = line.find("\"path\"");
        if (keyPos == std::string::npos) continue;

        size_t valStart = line.find('"', keyPos + 6);
        if (valStart == std::string::npos) continue;
        valStart++;

        size_t valEnd = line.find('"', valStart);
        if (valEnd == std::string::npos) continue;

        std::string rawPath = line.substr(valStart, valEnd - valStart);

        // VDF uses escaped backslashes
        std::string cleaned;
        for (size_t i = 0; i < rawPath.size(); ++i)
        {
            if (rawPath[i] == '\\' && i + 1 < rawPath.size() && rawPath[i + 1] == '\\')
            {
                cleaned += '\\';
                ++i;
            }
            else
            {
                cleaned += rawPath[i];
            }
        }

        int len = MultiByteToWideChar(CP_UTF8, 0, cleaned.c_str(),
                                      static_cast<int>(cleaned.size()), NULL, 0);
        std::wstring widePath(len, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, cleaned.c_str(),
                            static_cast<int>(cleaned.size()), widePath.data(), len);

        if (!widePath.empty())
            folders.push_back(widePath);
    }

    return folders;
}

// Read "installdir" from an appmanifest_NNNNN.acf file.
static std::string ReadInstallDir(const fs::path& acfPath)
{
    std::ifstream file(acfPath);
    if (!file.is_open()) return {};

    std::string line;
    while (std::getline(file, line))
    {
        size_t keyPos = line.find("\"installdir\"");
        if (keyPos == std::string::npos) continue;

        size_t valStart = line.find('"', keyPos + 12);
        if (valStart == std::string::npos) continue;
        valStart++;

        size_t valEnd = line.find('"', valStart);
        if (valEnd == std::string::npos) continue;

        return line.substr(valStart, valEnd - valStart);
    }
    return {};
}

std::wstring FindGameInstallPath()
{
    std::wstring steamDir = GetSteamInstallDir();
    if (steamDir.empty()) return {};

    auto libraries = GetSteamLibraryFolders(steamDir);

    std::wstring manifestName = L"appmanifest_" + std::to_wstring(SEVEN_DAYS_APP_ID) + L".acf";

    for (const auto& libDir : libraries)
    {
        fs::path steamapps = fs::path(libDir) / "steamapps";
        fs::path manifest  = steamapps / manifestName;

        if (!fs::exists(manifest)) continue;

        std::string installDir = ReadInstallDir(manifest);
        if (installDir.empty()) continue;

        fs::path gamePath = steamapps / "common" / installDir;
        if (fs::exists(gamePath))
            return gamePath.wstring();
    }

    return {};
}
