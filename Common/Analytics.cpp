#include "StdAfx.h"
#include "Analytics.h"
#include <sstream>
#include <iomanip>

CAnalytics* CAnalytics::m_pInstance = NULL;

CAnalytics::CAnalytics()
{
    m_strEndpoint = L"";
}

CAnalytics::~CAnalytics()
{
}

CAnalytics* CAnalytics::GetInstance()
{
    if (m_pInstance == NULL)
    {
        m_pInstance = new CAnalytics();
    }
    return m_pInstance;
}

void CAnalytics::SetEndpoint(const std::wstring& url)
{
    m_strEndpoint = url;
}

std::string CAnalytics::WStringToString(const std::wstring& wstr)
{
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

std::wstring CAnalytics::StringToWString(const std::string& str)
{
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

std::string CAnalytics::EscapeJsonString(const std::string& str)
{
    std::ostringstream oss;
    for (char ch : str)
    {
        switch (ch)
        {
        case '\\': oss << "\\\\"; break;
        case '\"': oss << "\\\""; break;
        case '\b': oss << "\\b"; break;
        case '\f': oss << "\\f"; break;
        case '\n': oss << "\\n"; break;
        case '\r': oss << "\\r"; break;
        case '\t': oss << "\\t"; break;
        default:
            if ('\x00' <= ch && ch <= '\x1f')
            {
                oss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)ch;
            }
            else
            {
                oss << ch;
            }
        }
    }
    return oss.str();
}

std::string CAnalytics::MapToJson(const std::map<std::wstring, std::wstring>& params)
{
    std::ostringstream oss;
    oss << "{";
    bool first = true;
    for (auto& pair : params)
    {
        if (!first) oss << ",";
        first = false;
        oss << "\"" << EscapeJsonString(WStringToString(pair.first)) << "\":\"" << EscapeJsonString(WStringToString(pair.second)) << "\"";
    }
    oss << "}";
    return oss.str();
}

bool CAnalytics::PostData(const std::wstring& url, const std::string& postData)
{
    if (url.empty()) return false;

    HINTERNET hInternet = NULL;
    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;
    bool bResult = false;

    try
    {
        // 初始化 WinINet
        hInternet = InternetOpen(L"InstallerAnalytics/1.0", 
            INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
        if (!hInternet) return false;

        // 解析 URL
        URL_COMPONENTS urlComp = { 0 };
        urlComp.dwStructSize = sizeof(urlComp);
        WCHAR szHostName[256] = { 0 };
        WCHAR szUrlPath[1024] = { 0 };
        urlComp.lpszHostName = szHostName;
        urlComp.dwHostNameLength = 256;
        urlComp.lpszUrlPath = szUrlPath;
        urlComp.dwUrlPathLength = 1024;

        if (!InternetCrackUrl(url.c_str(), 0, 0, &urlComp))
        {
            InternetCloseHandle(hInternet);
            return false;
        }

        // 连接服务器
        hConnect = InternetConnect(hInternet, szHostName, urlComp.nPort,
            NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
        if (!hConnect)
        {
            InternetCloseHandle(hInternet);
            return false;
        }

        // 打开请求
        LPCTSTR rgpszAcceptTypes[] = { L"*/*", NULL };
        DWORD dwFlags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE;
        if (urlComp.nScheme == INTERNET_SCHEME_HTTPS)
            dwFlags |= INTERNET_FLAG_SECURE;

        hRequest = HttpOpenRequest(hConnect, L"POST", szUrlPath, NULL, NULL,
            rgpszAcceptTypes, dwFlags, 0);
        if (!hRequest)
        {
            InternetCloseHandle(hConnect);
            InternetCloseHandle(hInternet);
            return false;
        }

        // 设置请求头
        std::wstring headers = L"Content-Type: application/json\r\n";
        
        // 发送请求
        bResult = HttpSendRequest(hRequest, headers.c_str(), -1,
            (LPVOID)postData.c_str(), static_cast<DWORD>(postData.length())) == TRUE;

        // 清理
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
    }
    catch (...)
    {
        if (hRequest) InternetCloseHandle(hRequest);
        if (hConnect) InternetCloseHandle(hConnect);
        if (hInternet) InternetCloseHandle(hInternet);
        return false;
    }

    return bResult;
}

bool CAnalytics::ReportEvent(const std::wstring& eventType, const std::map<std::wstring, std::wstring>& params)
{
    if (m_strEndpoint.empty()) return false;

    std::map<std::wstring, std::wstring> fullParams = params;
    fullParams[L"event_type"] = eventType;
    
    // 添加时间戳
    WCHAR szTime[64] = { 0 };
    SYSTEMTIME st;
    GetLocalTime(&st);
    wsprintf(szTime, L"%04d-%02d-%02d %02d:%02d:%02d", 
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    fullParams[L"timestamp"] = szTime;

    std::string jsonData = MapToJson(fullParams);
    return PostData(m_strEndpoint, jsonData);
}

bool CAnalytics::ReportInstall(const std::wstring& appName, const std::wstring& version, const std::wstring& installPath)
{
    std::map<std::wstring, std::wstring> params;
    params[L"app_name"] = appName;
    params[L"version"] = version;
    params[L"install_path"] = installPath;
    
    // 获取系统信息
    OSVERSIONINFOEX osvi = { 0 };
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    
    // 使用 RtlGetVersion 替代已弃用的 GetVersionEx
    typedef LONG(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
    HMODULE hMod = GetModuleHandleW(L"ntdll.dll");
    if (hMod)
    {
        RtlGetVersionPtr pRtlGetVersion = (RtlGetVersionPtr)GetProcAddress(hMod, "RtlGetVersion");
        if (pRtlGetVersion)
        {
            pRtlGetVersion((PRTL_OSVERSIONINFOW)&osvi);
        }
    }
    
    WCHAR szOSVersion[64] = { 0 };
    wsprintf(szOSVersion, L"Windows %d.%d", osvi.dwMajorVersion, osvi.dwMinorVersion);
    params[L"os_version"] = szOSVersion;
    
    return ReportEvent(L"install", params);
}

bool CAnalytics::ReportUninstall(const std::wstring& appName, const std::wstring& reason, const std::wstring& feedback)
{
    std::map<std::wstring, std::wstring> params;
    params[L"app_name"] = appName;
    params[L"reason"] = reason;
    params[L"feedback"] = feedback;
    
    return ReportEvent(L"uninstall", params);
}
