#pragma once

#include <Windows.h>
#include <string>

enum class LogColor : WORD
{
    White   = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
    Green   = FOREGROUND_GREEN | FOREGROUND_INTENSITY,
    Cyan    = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
    Yellow  = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
    Red     = FOREGROUND_RED | FOREGROUND_INTENSITY,
    Magenta = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
};

HANDLE GetConsoleHandle();

void SetColor(LogColor color);
void ResetColor();
void PrintColoredW(LogColor color, const std::wstring& text);
void PrintLineW(LogColor labelColor, const std::wstring& label,
                LogColor valueColor, const std::wstring& value);
bool PromptYesNo(const std::wstring& question);
void PrintProgressBar(DWORD received, DWORD total);
