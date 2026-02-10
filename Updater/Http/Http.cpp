#include "Http.h"
#include <winhttp.h>
#include <vector>
#include <functional>

#pragma comment(lib, "winhttp.lib")

std::wstring Utf8ToWide(const std::string& utf8)
{
    if (utf8.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()), NULL, 0);
    std::wstring wide(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()), wide.data(), len);
    return wide;
}

std::string DownloadText(const std::wstring& host, const std::wstring& path)
{
    std::string result;

    HINTERNET hSession = WinHttpOpen(L"VortexUpdater/1.0",
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return result;

    HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(),
                                        INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return result; }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path.c_str(),
                                            NULL, WINHTTP_NO_REFERER,
                                            WINHTTP_DEFAULT_ACCEPT_TYPES,
                                            WINHTTP_FLAG_SECURE);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return result;
    }

    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
        !WinHttpReceiveResponse(hRequest, NULL))
    {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return result;
    }

    DWORD bytesAvailable = 0;
    while (WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0)
    {
        std::vector<char> buf(bytesAvailable);
        DWORD bytesRead = 0;
        if (WinHttpReadData(hRequest, buf.data(), bytesAvailable, &bytesRead))
            result.append(buf.data(), bytesRead);
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return result;
}

std::vector<char> DownloadBinary(const std::wstring& host, const std::wstring& path)
{
    std::vector<char> result;

    HINTERNET hSession = WinHttpOpen(L"VortexUpdater/1.0",
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return result;

    HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(),
                                        INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return result; }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path.c_str(),
                                            NULL, WINHTTP_NO_REFERER,
                                            WINHTTP_DEFAULT_ACCEPT_TYPES,
                                            WINHTTP_FLAG_SECURE);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return result;
    }

    DWORD dwOption = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
    WinHttpSetOption(hRequest, WINHTTP_OPTION_REDIRECT_POLICY, &dwOption, sizeof(dwOption));

    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
        !WinHttpReceiveResponse(hRequest, NULL))
    {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return result;
    }

    DWORD bytesAvail = 0;
    while (WinHttpQueryDataAvailable(hRequest, &bytesAvail) && bytesAvail > 0)
    {
        size_t offset = result.size();
        result.resize(offset + bytesAvail);
        DWORD bytesRead = 0;
        if (WinHttpReadData(hRequest, result.data() + offset, bytesAvail, &bytesRead))
            result.resize(offset + bytesRead);
        else
            result.resize(offset);
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return result;
}

std::vector<char> DownloadBinaryFromUrl(const std::wstring& url)
{
    URL_COMPONENTS uc{};
    uc.dwStructSize = sizeof(uc);

    wchar_t hostBuf[256]{};
    wchar_t pathBuf[2048]{};
    uc.lpszHostName = hostBuf;
    uc.dwHostNameLength = _countof(hostBuf);
    uc.lpszUrlPath = pathBuf;
    uc.dwUrlPathLength = _countof(pathBuf);

    if (!WinHttpCrackUrl(url.c_str(), static_cast<DWORD>(url.size()), 0, &uc))
        return {};

    return DownloadBinary(std::wstring(hostBuf), std::wstring(pathBuf));
}

std::vector<char> DownloadBinaryWithProgress(const std::wstring& url,
                                             ProgressCallback onProgress)
{
    URL_COMPONENTS uc{};
    uc.dwStructSize = sizeof(uc);

    wchar_t hostBuf[256]{};
    wchar_t pathBuf[2048]{};
    uc.lpszHostName = hostBuf;
    uc.dwHostNameLength = _countof(hostBuf);
    uc.lpszUrlPath = pathBuf;
    uc.dwUrlPathLength = _countof(pathBuf);

    if (!WinHttpCrackUrl(url.c_str(), static_cast<DWORD>(url.size()), 0, &uc))
        return {};

    std::wstring host(hostBuf);
    std::wstring path(pathBuf);
    std::vector<char> result;

    HINTERNET hSession = WinHttpOpen(L"VortexUpdater/1.0",
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return result;

    HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(),
                                        INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return result; }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path.c_str(),
                                            NULL, WINHTTP_NO_REFERER,
                                            WINHTTP_DEFAULT_ACCEPT_TYPES,
                                            WINHTTP_FLAG_SECURE);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return result;
    }

    DWORD dwOption = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
    WinHttpSetOption(hRequest, WINHTTP_OPTION_REDIRECT_POLICY, &dwOption, sizeof(dwOption));

    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
        !WinHttpReceiveResponse(hRequest, NULL))
    {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return result;
    }

    DWORD totalSize = 0;
    DWORD headerSize = sizeof(totalSize);
    WinHttpQueryHeaders(hRequest,
                        WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX,
                        &totalSize, &headerSize, WINHTTP_NO_HEADER_INDEX);

    if (totalSize > 0)
        result.reserve(totalSize);

    DWORD received = 0;
    DWORD bytesAvail = 0;
    while (WinHttpQueryDataAvailable(hRequest, &bytesAvail) && bytesAvail > 0)
    {
        size_t offset = result.size();
        result.resize(offset + bytesAvail);
        DWORD bytesRead = 0;
        if (WinHttpReadData(hRequest, result.data() + offset, bytesAvail, &bytesRead))
        {
            result.resize(offset + bytesRead);
            received += bytesRead;
            if (onProgress)
                onProgress(received, totalSize);
        }
        else
        {
            result.resize(offset);
        }
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return result;
}
