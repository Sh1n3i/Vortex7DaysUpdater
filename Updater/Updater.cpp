#include "Console/Console.h"
#include "Http/Http.h"
#include "Parser/Parser.h"
#include "Steam/Steam.h"
#include "ModInstaller/ModInstaller.h"

int main()
{
    HANDLE hCon = GetConsoleHandle();

    // -- Header --
    PrintColoredW(LogColor::Yellow,
        L"============================================\n"
        L"   Vortex 7 Days to Die \x2014 \x041F\x0440\x043E\x0432\x0435\x0440\x043A\x0430 \x043E\x0431\x043D\x043E\x0432\x043B\x0435\x043D\x0438\x0439\n"
        L"============================================\n\n");

    // -- Detect game installation --
    PrintColoredW(LogColor::Cyan,
        L"[INFO] \x041F\x043E\x0438\x0441\x043A \x0443\x0441\x0442\x0430\x043D\x043E\x0432\x043B\x0435\x043D\x043D\x043E\x0439 \x0438\x0433\x0440\x044B 7 Days to Die...\n");
    //   = "????? ????????????? ???? 7 Days to Die..."

    std::wstring gamePath = FindGameInstallPath();
    if (gamePath.empty())
    {
        PrintColoredW(LogColor::Red,
            L"[\x041E\x0428\x0418\x0411\x041A\x0410] \x0418\x0433\x0440\x0430 7 Days to Die \x043D\x0435 \x043D\x0430\x0439\x0434\x0435\x043D\x0430 \x0432 Steam!\n\n");
        //   = "???? 7 Days to Die ?? ??????? ? Steam!"
    }
    else
    {
        // "???? ???????: "
        PrintLineW(LogColor::Green,
            L"[OK] \x0418\x0433\x0440\x0430 \x043D\x0430\x0439\x0434\x0435\x043D\x0430: ",
            LogColor::White, gamePath);

        DWORD written = 0;
        WriteConsoleW(hCon, L"\n", 1, &written, NULL);
    }

    // -- Fetch update info --
    PrintColoredW(LogColor::Cyan,
        L"[INFO] \x0417\x0430\x0433\x0440\x0443\x0437\x043A\x0430 \x0434\x0430\x043D\x043D\x044B\x0445 \x043E\x0431\x043D\x043E\x0432\x043B\x0435\x043D\x0438\x044F \x0441 GitHub...\n");
    //   = "???????? ?????? ?????????? ? GitHub..."

    std::string body = DownloadText(
        L"raw.githubusercontent.com",
        L"/PavelDeep/Building_Mods_Vortex/refs/heads/main/Update_Vortex");

    if (body.empty())
    {
        PrintColoredW(LogColor::Red,
            L"[\x041E\x0428\x0418\x0411\x041A\x0410] \x041D\x0435 \x0443\x0434\x0430\x043B\x043E\x0441\x044C \x0437\x0430\x0433\x0440\x0443\x0437\x0438\x0442\x044C \x0444\x0430\x0439\x043B \x043E\x0431\x043D\x043E\x0432\x043B\x0435\x043D\x0438\x044F!\n");
        //   = "?? ??????? ????????? ???? ??????????!"
        return 1;
    }

    PrintColoredW(LogColor::Green,
        L"[OK] \x0414\x0430\x043D\x043D\x044B\x0435 \x0443\x0441\x043F\x0435\x0448\x043D\x043E \x043F\x043E\x043B\x0443\x0447\x0435\x043D\x044B.\n\n");
    //   = "?????? ??????? ????????."

    // -- Parse & display --
    auto entries = ParseUpdateInfo(body);
    if (entries.empty())
    {
        PrintColoredW(LogColor::Red,
            L"[\x041E\x0428\x0418\x0411\x041A\x0410] \x0424\x0430\x0439\x043B \x043E\x0431\x043D\x043E\x0432\x043B\x0435\x043D\x0438\x044F \x043F\x0443\x0441\x0442 \x0438\x043B\x0438 \x0438\x043C\x0435\x0435\x0442 \x043D\x0435\x0432\x0435\x0440\x043D\x044B\x0439 \x0444\x043E\x0440\x043C\x0430\x0442.\n");
        //   = "???? ?????????? ???? ??? ????? ???????? ??????."
        return 1;
    }

    // "?????????? ?? ??????????"
    PrintColoredW(LogColor::Yellow,
        L"\x2500\x2500 \x0418\x043D\x0444\x043E\x0440\x043C\x0430\x0446\x0438\x044F \x043E\x0431 \x043E\x0431\x043D\x043E\x0432\x043B\x0435\x043D\x0438\x0438 "
        L"\x2500\x2500\x2500\x2500\x2500\x2500\x2500\x2500\x2500\x2500\x2500\x2500\x2500\x2500\x2500\x2500\x2500\x2500\x2500\x2500\x2500\x2500\x2500\x2500\x2500\x2500\n");

    for (const auto& [key, value] : entries)
    {
        LogColor valColor = ColorForKey(key);
        std::wstring wKey = Utf8ToWide(key);
        std::wstring wVal = Utf8ToWide(value);
        PrintLineW(LogColor::White, L"  " + wKey + L": ", valColor, wVal);
    }

    DWORD written = 0;
    WriteConsoleW(hCon, L"\n", 1, &written, NULL);

    // -- Install mods --
    if (!gamePath.empty())
    {
        if (InstallLatestMods(gamePath))
        {
            PrintColoredW(LogColor::Green,
                L"\n[\x0413\x041E\x0422\x041E\x0412\x041E] \x041E\x0431\x043D\x043E\x0432\x043B\x0435\x043D\x0438\x0435 \x0437\x0430\x0432\x0435\x0440\x0448\x0435\x043D\x043E.\n");
        }
        else
        {
            PrintColoredW(LogColor::Red,
                L"\n[\x041E\x0428\x0418\x0411\x041A\x0410] \x041E\x0431\x043D\x043E\x0432\x043B\x0435\x043D\x0438\x0435 \x043D\x0435 \x0443\x0434\x0430\x043B\x043E\x0441\x044C.\n");
        }
    }

    PrintColoredW(LogColor::White,
        L"\n\x041D\x0430\x0436\x043C\x0438\x0442\x0435 Enter \x0434\x043B\x044F \x0437\x0430\x043A\x0440\x044B\x0442\x0438\x044F...\n");
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    FlushConsoleInputBuffer(hIn);
    wchar_t buf[4]{};
    DWORD read = 0;
    ReadConsoleW(hIn, buf, 2, &read, NULL);

    return 0;
}
