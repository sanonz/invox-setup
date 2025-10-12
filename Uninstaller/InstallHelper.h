#pragma once
#include <windows.h>
#include <string>

// 安装辅助函数
class CInstallHelper
{
public:
    // 创建桌面快捷方式
    static bool CreateDesktopShortcut(const std::wstring& targetPath, const std::wstring& shortcutName);
    
    // 创建开始菜单快捷方式
    static bool CreateStartMenuShortcut(const std::wstring& targetPath, const std::wstring& shortcutName, const std::wstring& folderName = L"");
    
    // 写入注册表卸载信息
    static bool WriteUninstallRegistry(const std::wstring& appName, 
        const std::wstring& version,
        const std::wstring& publisher,
        const std::wstring& installPath,
        const std::wstring& uninstallPath,
        const std::wstring& iconPath,
        UINT64 estimatedSize);
    
    // 删除注册表卸载信息
    static bool RemoveUninstallRegistry(const std::wstring& appName);
    
    // 删除开始菜单快捷方式
    static bool RemoveStartMenuShortcut(const std::wstring& shortcutName, const std::wstring& folderName = L"");
    
    // 获取 Program Files 目录
    static std::wstring GetProgramFilesPath();
    
    // 确保路径以应用名称结尾
    static std::wstring EnsureAppNameInPath(const std::wstring& path, const std::wstring& appName);
    
    // 计算目录大小（字节）
    static UINT64 GetDirectorySize(const std::wstring& path);
    
    // 删除目录及所有内容
    static bool RemoveDirectory(const std::wstring& path);
    
    // 获取系统语言
    static std::wstring GetSystemLanguage();
};
