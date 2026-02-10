#include "Console.h"
#include <cstdio>
#include <io.h>
#include <fcntl.h>

static HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

HANDLE GetConsoleHandle()
{
    return hConsole;
}

void SetColor(LogColor color)
{
    SetConsoleTextAttribute(hConsole, static_cast<WORD>(color));
}

void ResetColor()
{
    SetColor(LogColor::White);
}

void PrintColoredW(LogColor color, const std::wstring& text)
{
    SetColor(color);
    DWORD written = 0;
    WriteConsoleW(hConsole, text.c_str(), static_cast<DWORD>(text.size()), &written, NULL);
    ResetColor();
}

void PrintLineW(LogColor labelColor, const std::wstring& label,
                LogColor valueColor, const std::wstring& value)
{
    DWORD written = 0;
    SetColor(labelColor);
    WriteConsoleW(hConsole, label.c_str(), static_cast<DWORD>(label.size()), &written, NULL);
    SetColor(valueColor);
    WriteConsoleW(hConsole, value.c_str(), static_cast<DWORD>(value.size()), &written, NULL);
    WriteConsoleW(hConsole, L"\n", 1, &written, NULL);
    ResetColor();
}

bool PromptYesNo(const std::wstring& question)
{
    SetColor(LogColor::Yellow);
    DWORD written = 0;
    WriteConsoleW(hConsole, question.c_str(), static_cast<DWORD>(question.size()), &written, NULL);

    std::wstring suffix = L" (Y/N): ";
    WriteConsoleW(hConsole, suffix.c_str(), static_cast<DWORD>(suffix.size()), &written, NULL);
    ResetColor();

    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    wchar_t buf[8]{};
    DWORD read = 0;
    ReadConsoleW(hIn, buf, 4, &read, NULL);

    wchar_t ch = buf[0];
    return (ch == L'Y' || ch == L'y' ||
            ch == L'\x0414' || ch == L'\x0434');  // ? / ?
}

void PrintProgressBar(DWORD received, DWORD total)
{
    constexpr int BAR_WIDTH = 40;

    int percent = 0;
    int filled = 0;
    if (total > 0)
    {
        percent = static_cast<int>((static_cast<ULONGLONG>(received) * 100) / total);
        filled = static_cast<int>((static_cast<ULONGLONG>(received) * BAR_WIDTH) / total);
    }
    else
    {
        percent = 0;
        filled = 0;
    }

    if (filled > BAR_WIDTH) filled = BAR_WIDTH;
    if (percent > 100) percent = 100;

    DWORD receivedMB = received / (1024 * 1024);
    DWORD totalMB = total / (1024 * 1024);

    wchar_t line[128]{};
    int pos = 0;
    line[pos++] = L'\r';
    line[pos++] = L'[';
    for (int i = 0; i < BAR_WIDTH; ++i)
        line[pos++] = (i < filled) ? L'\x2588' : L'\x2591';
    line[pos++] = L']';
    line[pos++] = L' ';

    wchar_t tail[48]{};
    if (total > 0)
        wsprintfW(tail, L"%d%% (%lu / %lu MB)", percent, receivedMB, totalMB);
    else
        wsprintfW(tail, L"%lu MB", receivedMB);

    for (int i = 0; tail[i]; ++i)
        line[pos++] = tail[i];

    // Pad with spaces to overwrite previous line remnants
    while (pos < 100)
        line[pos++] = L' ';

    DWORD written = 0;
    SetColor(LogColor::Green);
    WriteConsoleW(hConsole, line, pos, &written, NULL);
    ResetColor();
}
