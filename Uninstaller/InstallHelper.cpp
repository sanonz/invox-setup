#include "StdAfx.h"
#include "InstallHelper.h"
#include <shlobj.h>
#include <shlwapi.h>
#include <atlbase.h>
#include <shobjidl.h>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "shell32.lib")

bool CInstallHelper::CreateDesktopShortcut(const std::wstring& targetPath, const std::wstring& shortcutName)
{
    HRESULT hr = CoInitialize(NULL);
    
    // 获取桌面路径
    WCHAR szDesktopPath[MAX_PATH] = { 0 };
    SHGetFolderPath(NULL, CSIDL_DESKTOP, NULL, 0, szDesktopPath);
    
    // 构建快捷方式路径
    std::wstring shortcutPath = szDesktopPath;
    shortcutPath += L"\\";
    shortcutPath += shortcutName;
    shortcutPath += L".lnk";
    
    // 创建快捷方式
    IShellLink* pShellLink = NULL;
    hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void**)&pShellLink);
    
    if (SUCCEEDED(hr))
    {
        pShellLink->SetPath(targetPath.c_str());
        
        // 设置工作目录
        std::wstring workDir = targetPath;
        size_t pos = workDir.find_last_of(L"\\");
        if (pos != std::wstring::npos)
        {
            workDir = workDir.substr(0, pos);
        }
        pShellLink->SetWorkingDirectory(workDir.c_str());
        
        // 保存快捷方式
        IPersistFile* pPersistFile = NULL;
        hr = pShellLink->QueryInterface(IID_IPersistFile, (void**)&pPersistFile);
        
        if (SUCCEEDED(hr))
        {
            hr = pPersistFile->Save(shortcutPath.c_str(), TRUE);
            pPersistFile->Release();
        }
        
        pShellLink->Release();
    }
    
    CoUninitialize();
    
    return SUCCEEDED(hr);
}

bool CInstallHelper::WriteUninstallRegistry(const std::wstring& appName,
    const std::wstring& version,
    const std::wstring& publisher,
    const std::wstring& installPath,
    const std::wstring& uninstallPath,
    UINT64 estimatedSize)
{
    std::wstring regPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\";
    regPath += appName;
    
    HKEY hKey = NULL;
    LONG lResult = RegCreateKeyEx(HKEY_LOCAL_MACHINE, regPath.c_str(), 0, NULL,
        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
    
    if (lResult != ERROR_SUCCESS)
    {
        // 如果无法写入 HKLM，尝试写入 HKCU
        lResult = RegCreateKeyEx(HKEY_CURRENT_USER, regPath.c_str(), 0, NULL,
            REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
    }
    
    if (lResult == ERROR_SUCCESS)
    {
        // 写入显示名称
        RegSetValueEx(hKey, L"DisplayName", 0, REG_SZ, 
            (BYTE*)appName.c_str(), (appName.length() + 1) * sizeof(WCHAR));
        
        // 写入版本
        RegSetValueEx(hKey, L"DisplayVersion", 0, REG_SZ,
            (BYTE*)version.c_str(), (version.length() + 1) * sizeof(WCHAR));
        
        // 写入发布者
        RegSetValueEx(hKey, L"Publisher", 0, REG_SZ,
            (BYTE*)publisher.c_str(), (publisher.length() + 1) * sizeof(WCHAR));
        
        // 写入安装路径
        RegSetValueEx(hKey, L"InstallLocation", 0, REG_SZ,
            (BYTE*)installPath.c_str(), (installPath.length() + 1) * sizeof(WCHAR));
        
        // 写入卸载命令
        std::wstring uninstallCmd = L"\"" + uninstallPath + L"\"";
        RegSetValueEx(hKey, L"UninstallString", 0, REG_SZ,
            (BYTE*)uninstallCmd.c_str(), (uninstallCmd.length() + 1) * sizeof(WCHAR));
        
        // 写入估计大小（KB）
        DWORD dwSize = (DWORD)(estimatedSize / 1024);
        RegSetValueEx(hKey, L"EstimatedSize", 0, REG_DWORD, (BYTE*)&dwSize, sizeof(DWORD));
        
        // 没有修改选项
        DWORD dwNoModify = 1;
        RegSetValueEx(hKey, L"NoModify", 0, REG_DWORD, (BYTE*)&dwNoModify, sizeof(DWORD));
        
        // 没有修复选项
        DWORD dwNoRepair = 1;
        RegSetValueEx(hKey, L"NoRepair", 0, REG_DWORD, (BYTE*)&dwNoRepair, sizeof(DWORD));
        
        RegCloseKey(hKey);
        return true;
    }
    
    return false;
}

bool CInstallHelper::RemoveUninstallRegistry(const std::wstring& appName)
{
    std::wstring regPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\";
    regPath += appName;
    
    LONG lResult = RegDeleteKey(HKEY_LOCAL_MACHINE, regPath.c_str());
    if (lResult != ERROR_SUCCESS)
    {
        lResult = RegDeleteKey(HKEY_CURRENT_USER, regPath.c_str());
    }
    
    return lResult == ERROR_SUCCESS;
}

std::wstring CInstallHelper::GetProgramFilesPath()
{
    WCHAR szPath[MAX_PATH] = { 0 };
    SHGetFolderPath(NULL, CSIDL_PROGRAM_FILES, NULL, 0, szPath);
    return szPath;
}

std::wstring CInstallHelper::EnsureAppNameInPath(const std::wstring& path, const std::wstring& appName)
{
    std::wstring result = path;
    
    // 移除末尾的反斜杠
    while (!result.empty() && result[result.length() - 1] == L'\\')
    {
        result = result.substr(0, result.length() - 1);
    }
    
    // 检查路径是否已经以应用名称结尾
    size_t pos = result.find_last_of(L"\\");
    std::wstring lastName;
    if (pos != std::wstring::npos)
    {
        lastName = result.substr(pos + 1);
    }
    else
    {
        lastName = result;
    }
    
    if (_wcsicmp(lastName.c_str(), appName.c_str()) != 0)
    {
        result += L"\\";
        result += appName;
    }
    
    return result;
}

UINT64 CInstallHelper::GetDirectorySize(const std::wstring& path)
{
    UINT64 totalSize = 0;
    
    std::wstring searchPath = path + L"\\*.*";
    WIN32_FIND_DATA findData;
    HANDLE hFind = FindFirstFile(searchPath.c_str(), &findData);
    
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (wcscmp(findData.cFileName, L".") == 0 || wcscmp(findData.cFileName, L"..") == 0)
                continue;
            
            std::wstring fullPath = path + L"\\" + findData.cFileName;
            
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                totalSize += GetDirectorySize(fullPath);
            }
            else
            {
                LARGE_INTEGER fileSize;
                fileSize.LowPart = findData.nFileSizeLow;
                fileSize.HighPart = findData.nFileSizeHigh;
                totalSize += fileSize.QuadPart;
            }
        } while (FindNextFile(hFind, &findData));
        
        FindClose(hFind);
    }
    
    return totalSize;
}

bool CInstallHelper::RemoveDirectory(const std::wstring& path)
{
    std::wstring searchPath = path + L"\\*.*";
    WIN32_FIND_DATA findData;
    HANDLE hFind = FindFirstFile(searchPath.c_str(), &findData);
    
    if (hFind == INVALID_HANDLE_VALUE)
        return false;
    
    do
    {
        if (wcscmp(findData.cFileName, L".") == 0 || wcscmp(findData.cFileName, L"..") == 0)
            continue;
        
        std::wstring fullPath = path + L"\\" + findData.cFileName;
        
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            RemoveDirectory(fullPath);
        }
        else
        {
            SetFileAttributes(fullPath.c_str(), FILE_ATTRIBUTE_NORMAL);
            DeleteFile(fullPath.c_str());
        }
    } while (FindNextFile(hFind, &findData));
    
    FindClose(hFind);
    
    return ::RemoveDirectory(path.c_str()) == TRUE;
}

std::wstring CInstallHelper::GetSystemLanguage()
{
    LANGID langId = GetUserDefaultUILanguage();
    WORD primaryLang = PRIMARYLANGID(langId);
    
    if (primaryLang == LANG_CHINESE)
    {
        return L"cn";
    }
    else
    {
        return L"en";
    }
}
