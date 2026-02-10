#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include <functional>

using ProgressCallback = std::function<void(DWORD bytesReceived, DWORD totalBytes)>;

std::wstring Utf8ToWide(const std::string& utf8);
std::string DownloadText(const std::wstring& host, const std::wstring& path);
std::vector<char> DownloadBinary(const std::wstring& host, const std::wstring& path);
std::vector<char> DownloadBinaryFromUrl(const std::wstring& url);
std::vector<char> DownloadBinaryWithProgress(const std::wstring& url,
                                             ProgressCallback onProgress);
