#include "ModInstaller.h"
#include "../Http/Http.h"
#include "../Console/Console.h"
#include <Windows.h>
#include <shlobj.h>
#include <shldisp.h>
#include <atlbase.h>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")

// ?? tiny JSON helpers (no third-party lib) ??????????????????????????

static std::string FindJsonString(const std::string& json, const std::string& key)
{
    std::string needle = "\"" + key + "\"";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) return {};

    pos = json.find(':', pos + needle.size());
    if (pos == std::string::npos) return {};

    pos = json.find('"', pos + 1);
    if (pos == std::string::npos) return {};
    pos++;

    std::string value;
    for (; pos < json.size() && json[pos] != '"'; ++pos)
    {
        if (json[pos] == '\\' && pos + 1 < json.size())
        {
            ++pos;
            value += json[pos];
        }
        else
        {
            value += json[pos];
        }
    }
    return value;
}

std::wstring ParseReleaseDownloadUrl(const std::string& json)
{
    // Find the "assets" array and grab the first "browser_download_url"
    size_t assetsPos = json.find("\"assets\"");
    if (assetsPos == std::string::npos) return {};

    std::string assetsSub = json.substr(assetsPos);
    std::string url = FindJsonString(assetsSub, "browser_download_url");
    if (url.empty()) return {};

    return Utf8ToWide(url);
}

std::string ParseReleaseVersion(const std::string& json)
{
    return FindJsonString(json, "tag_name");
}

// ?? Version file ????????????????????????????????????????????????????

static fs::path VersionFilePath(const std::wstring& gamePath)
{
    return fs::path(gamePath) / "Mods" / ".vortex_version";
}

std::string ReadInstalledVersion(const std::wstring& gamePath)
{
    auto p = VersionFilePath(gamePath);
    std::ifstream f(p);
    if (!f.is_open()) return {};
    std::string ver;
    std::getline(f, ver);
    return ver;
}

void WriteInstalledVersion(const std::wstring& gamePath, const std::string& version)
{
    auto p = VersionFilePath(gamePath);
    fs::create_directories(p.parent_path());
    std::ofstream f(p, std::ios::trunc);
    f << version;
}

// ?? Backup ??????????????????????????????????????????????????????????

static std::wstring BackupDir(const std::wstring& gamePath)
{
    return (fs::path(gamePath) / "Mods_Backups").wstring();
}

void RotateBackups(const std::wstring& gamePath)
{
    fs::path dir = BackupDir(gamePath);
    if (!fs::exists(dir)) return;

    std::vector<fs::path> backups;
    for (const auto& e : fs::directory_iterator(dir))
    {
        if (e.is_directory())
            backups.push_back(e.path());
    }

    std::sort(backups.begin(), backups.end());

    while (static_cast<int>(backups.size()) > MAX_BACKUPS)
    {
        std::error_code ec;
        fs::remove_all(backups.front(), ec);
        backups.erase(backups.begin());
    }
}

bool BackupModsFolder(const std::wstring& gamePath)
{
    fs::path modsDir = fs::path(gamePath) / "Mods";
    if (!fs::exists(modsDir)) return true;

    fs::path backupDir = BackupDir(gamePath);
    fs::create_directories(backupDir);

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_s(&tm, &time);

    std::wostringstream wss;
    wss << L"Mods_Backup_"
        << std::setfill(L'0')
        << (tm.tm_year + 1900)
        << std::setw(2) << (tm.tm_mon + 1)
        << std::setw(2) << tm.tm_mday << L"_"
        << std::setw(2) << tm.tm_hour
        << std::setw(2) << tm.tm_min
        << std::setw(2) << tm.tm_sec;

    fs::path destCopy = backupDir / wss.str();
    std::error_code ec;
    fs::create_directories(destCopy, ec);
    if (ec) return false;

    fs::copy(modsDir, destCopy,
             fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
    if (ec)
    {
        PrintColoredW(LogColor::Red,
            L"[\x041E\x0428\x0418\x0411\x041A\x0410] \x041D\x0435 \x0443\x0434\x0430\x043B\x043E\x0441\x044C \x0441\x043A\x043E\x043F\x0438\x0440\x043E\x0432\x0430\x0442\x044C Mods!\n");
        fs::remove_all(destCopy, ec);
        return false;
    }

    RotateBackups(gamePath);
    return true;
}

// ?? Mods folder cleanup ?????????????????????????????????????????????

static std::wstring ToLower(const std::wstring& s)
{
    std::wstring out = s;
    std::transform(out.begin(), out.end(), out.begin(), ::towlower);
    return out;
}

bool CleanModsFolder(const std::wstring& modsPath,
                     const std::vector<std::wstring>& protect)
{
    if (!fs::exists(modsPath)) return true;

    std::vector<std::wstring> protectLower;
    for (const auto& p : protect)
        protectLower.push_back(ToLower(p));

    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(modsPath, ec))
    {
        std::wstring name = ToLower(entry.path().filename().wstring());
        bool skip = false;
        for (const auto& p : protectLower)
        {
            if (name == p) { skip = true; break; }
        }
        if (skip) continue;

        fs::remove_all(entry.path(), ec);
        if (ec)
        {
            PrintColoredW(LogColor::Red,
                L"[\x041E\x0428\x0418\x0411\x041A\x0410] \x041D\x0435 \x0443\x0434\x0430\x043B\x043E\x0441\x044C \x0443\x0434\x0430\x043B\x0438\x0442\x044C: "
                + entry.path().wstring() + L"\n");
        }
    }
    return true;
}

// ?? ZIP extraction via Shell COM ????????????????????????????????????

bool ExtractZipToFolder(const std::wstring& zipPath, const std::wstring& destPath)
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    CComPtr<IShellDispatch> pShell;
    HRESULT hr = CoCreateInstance(CLSID_Shell, NULL, CLSCTX_INPROC_SERVER,
                                  IID_IShellDispatch,
                                  reinterpret_cast<void**>(&pShell));
    if (FAILED(hr)) { CoUninitialize(); return false; }

    CComVariant vZip(zipPath.c_str());
    CComVariant vDest(destPath.c_str());

    CComPtr<Folder> pZipFolder;
    hr = pShell->NameSpace(vZip, &pZipFolder);
    if (FAILED(hr) || !pZipFolder) { CoUninitialize(); return false; }

    CComPtr<Folder> pDestFolder;
    hr = pShell->NameSpace(vDest, &pDestFolder);
    if (FAILED(hr) || !pDestFolder) { CoUninitialize(); return false; }

    CComPtr<FolderItems> pItems;
    hr = pZipFolder->Items(&pItems);
    if (FAILED(hr) || !pItems) { CoUninitialize(); return false; }

    long itemCount = 0;
    pItems->get_Count(&itemCount);

    CComVariant vOpt(FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT);
    hr = pDestFolder->CopyHere(CComVariant(static_cast<IDispatch*>(pItems)), vOpt);

    if (SUCCEEDED(hr))
    {
        // CopyHere is async — wait until all items appear in destination
        namespace fs = std::filesystem;
        for (int attempt = 0; attempt < 600; ++attempt)  // up to 60 seconds
        {
            Sleep(100);
            long count = 0;
            CComPtr<FolderItems> pDestItems;
            if (SUCCEEDED(pDestFolder->Items(&pDestItems)) && pDestItems)
                pDestItems->get_Count(&count);
            if (count >= itemCount)
                break;
        }
    }

    CoUninitialize();
    return SUCCEEDED(hr);
}

// ?? Full pipeline ???????????????????????????????????????????????????

bool InstallLatestMods(const std::wstring& gamePath)
{
    // 1. Fetch latest release metadata from GitHub API
    PrintColoredW(LogColor::Cyan,
        L"[INFO] \x0417\x0430\x043F\x0440\x043E\x0441 \x043F\x043E\x0441\x043B\x0435\x0434\x043D\x0435\x0433\x043E \x0440\x0435\x043B\x0438\x0437\x0430 \x0441 GitHub...\n");

    std::string releaseJson = DownloadText(
        L"api.github.com",
        L"/repos/PavelDeep/Building_Mods_Vortex/releases/latest");

    if (releaseJson.empty())
    {
        PrintColoredW(LogColor::Red,
            L"[\x041E\x0428\x0418\x0411\x041A\x0410] \x041D\x0435 \x0443\x0434\x0430\x043B\x043E\x0441\x044C \x043F\x043E\x043B\x0443\x0447\x0438\x0442\x044C \x0434\x0430\x043D\x043D\x044B\x0435 \x0440\x0435\x043B\x0438\x0437\x0430!\n");
        return false;
    }

    // 2. Version check
    std::string latestVersion = ParseReleaseVersion(releaseJson);
    std::string installedVersion = ReadInstalledVersion(gamePath);

    if (!latestVersion.empty() && latestVersion == installedVersion)
    {
        PrintColoredW(LogColor::Green,
            L"[OK] \x0423\x0436\x0435 \x0443\x0441\x0442\x0430\x043D\x043E\x0432\x043B\x0435\x043D\x0430 \x043F\x043E\x0441\x043B\x0435\x0434\x043D\x044F\x044F \x0432\x0435\x0440\x0441\x0438\x044F: "
            + Utf8ToWide(latestVersion) + L"\n");
        return true;
    }

    if (!installedVersion.empty())
    {
        PrintLineW(LogColor::Cyan,
            L"[INFO] \x0422\x0435\x043A\x0443\x0449\x0430\x044F \x0432\x0435\x0440\x0441\x0438\x044F: ",
            LogColor::White, Utf8ToWide(installedVersion));
    }
    if (!latestVersion.empty())
    {
        PrintLineW(LogColor::Cyan,
            L"[INFO] \x041D\x043E\x0432\x0430\x044F \x0432\x0435\x0440\x0441\x0438\x044F:    ",
            LogColor::Yellow, Utf8ToWide(latestVersion));
    }

    std::wstring downloadUrl = ParseReleaseDownloadUrl(releaseJson);
    if (downloadUrl.empty())
    {
        PrintColoredW(LogColor::Red,
            L"[\x041E\x0428\x0418\x0411\x041A\x0410] \x041D\x0435 \x043D\x0430\x0439\x0434\x0435\x043D\x0430 \x0441\x0441\x044B\x043B\x043A\x0430 \x0434\x043B\x044F \x0441\x043A\x0430\x0447\x0438\x0432\x0430\x043D\x0438\x044F!\n");
        return false;
    }

    PrintColoredW(LogColor::Green,
        L"[OK] \x0421\x0441\x044B\x043B\x043A\x0430 \x043D\x0430\x0439\x0434\x0435\x043D\x0430.\n");

    // 3. Ask user about backup
    fs::path modsDir = fs::path(gamePath) / "Mods";
    if (fs::exists(modsDir) && !fs::is_empty(modsDir))
    {
        // \"\x0421\x043E\x0437\x0434\x0430\x0442\x044C \x0431\x044D\x043A\x0430\x043F \x0442\x0435\x043A\x0443\x0449\x0438\x0445 \x043C\x043E\x0434\x043E\x0432?\" = \"Create backup of current mods?\"
        if (PromptYesNo(L"\x0421\x043E\x0437\x0434\x0430\x0442\x044C \x0431\x044D\x043A\x0430\x043F \x0442\x0435\x043A\x0443\x0449\x0438\x0445 \x043C\x043E\x0434\x043E\x0432?"))
        {
            PrintColoredW(LogColor::Cyan,
                L"[INFO] \x0421\x043E\x0437\x0434\x0430\x043D\x0438\x0435 \x0431\x044D\x043A\x0430\x043F\x0430...\n");

            if (BackupModsFolder(gamePath))
            {
                PrintColoredW(LogColor::Green,
                    L"[OK] \x0411\x044D\x043A\x0430\x043F \x0441\x043E\x0437\x0434\x0430\x043D (\x043C\x0430\x043A\x0441: "
                    + std::to_wstring(MAX_BACKUPS) + L").\n");
            }
            else
            {
                PrintColoredW(LogColor::Yellow,
                    L"[\x0412\x041D\x0418\x041C\x0410\x041D\x0418\x0415] \x041D\x0435 \x0443\x0434\x0430\x043B\x043E\x0441\x044C \x0441\x043E\x0437\x0434\x0430\x0442\x044C \x0431\x044D\x043A\x0430\x043F, \x043F\x0440\x043E\x0434\x043E\x043B\x0436\x0430\x0435\x043C...\n");
            }
        }
    }

    // 4. Download the ZIP with progress bar
    PrintColoredW(LogColor::Cyan,
        L"[INFO] \x0421\x043A\x0430\x0447\x0438\x0432\x0430\x043D\x0438\x0435 \x0430\x0440\x0445\x0438\x0432\x0430...\n");

    std::vector<char> zipData = DownloadBinaryWithProgress(downloadUrl,
        [](DWORD received, DWORD total) {
            PrintProgressBar(received, total);
        });

    // Newline after progress bar
    DWORD w = 0;
    HANDLE hCon = GetConsoleHandle();
    WriteConsoleW(hCon, L"\n", 1, &w, NULL);

    if (zipData.empty())
    {
        PrintColoredW(LogColor::Red,
            L"[\x041E\x0428\x0418\x0411\x041A\x0410] \x041D\x0435 \x0443\x0434\x0430\x043B\x043E\x0441\x044C \x0441\x043A\x0430\x0447\x0430\x0442\x044C \x0430\x0440\x0445\x0438\x0432!\n");
        return false;
    }

    PrintColoredW(LogColor::Green,
        L"[OK] \x0410\x0440\x0445\x0438\x0432 \x0437\x0430\x0433\x0440\x0443\x0436\x0435\x043D ("
        + std::to_wstring(zipData.size() / 1024 / 1024) + L" MB).\n");

    // 5. Save ZIP to temp file
    wchar_t tmpDir[MAX_PATH]{};
    GetTempPathW(MAX_PATH, tmpDir);
    std::wstring zipPath = std::wstring(tmpDir) + L"Update_Vortex.zip";

    {
        std::ofstream out(zipPath, std::ios::binary);
        if (!out.is_open())
        {
            PrintColoredW(LogColor::Red,
                L"[\x041E\x0428\x0418\x0411\x041A\x0410] \x041D\x0435 \x0443\x0434\x0430\x043B\x043E\x0441\x044C \x0441\x043E\x0445\x0440\x0430\x043D\x0438\x0442\x044C \x0432\x0440\x0435\x043C\x0435\x043D\x043D\x044B\x0439 \x0444\x0430\x0439\x043B!\n");
            return false;
        }
        out.write(zipData.data(), zipData.size());
    }

    // 6. Prepare Mods folder
    if (!fs::exists(modsDir))
        fs::create_directories(modsDir);

    // 7. Clean existing mods (keep protected folders)
    PrintColoredW(LogColor::Cyan,
        L"[INFO] \x041E\x0447\x0438\x0441\x0442\x043A\x0430 \x043F\x0430\x043F\x043A\x0438 Mods...\n");

    std::vector<std::wstring> protectedFolders = { L"0_TFP_Harmony", L"Jahy7Days", L".vortex_version" };
    CleanModsFolder(modsDir.wstring(), protectedFolders);

    PrintColoredW(LogColor::Green,
        L"[OK] \x041F\x0430\x043F\x043A\x0430 \x043E\x0447\x0438\x0449\x0435\x043D\x0430 (\x0441\x043E\x0445\x0440\x0430\x043D\x0435\x043D\x044B: 0_TFP_Harmony, Jahy7Days).\n");

    // 8. Extract ZIP to Mods
    PrintColoredW(LogColor::Cyan,
        L"[INFO] \x0420\x0430\x0441\x043F\x0430\x043A\x043E\x0432\x043A\x0430 \x043C\x043E\x0434\x043E\x0432...\n");

    bool ok = ExtractZipToFolder(zipPath, modsDir.wstring());

    // 9. Cleanup temp
    DeleteFileW(zipPath.c_str());

    if (!ok)
    {
        PrintColoredW(LogColor::Red,
            L"[\x041E\x0428\x0418\x0411\x041A\x0410] \x041D\x0435 \x0443\x0434\x0430\x043B\x043E\x0441\x044C \x0440\x0430\x0441\x043F\x0430\x043A\x043E\x0432\x0430\x0442\x044C \x0430\x0440\x0445\x0438\x0432!\n");
        return false;
    }

    // 10. Save installed version
    if (!latestVersion.empty())
        WriteInstalledVersion(gamePath, latestVersion);

    PrintColoredW(LogColor::Green,
        L"[OK] \x041C\x043E\x0434\x044B \x0443\x0441\x043F\x0435\x0448\x043D\x043E \x0443\x0441\x0442\x0430\x043D\x043E\x0432\x043B\x0435\x043D\x044B!\n");
    return true;
}
