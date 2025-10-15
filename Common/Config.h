#pragma once

// 将数字转换为字符串
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

// 版本号定义
#define APP_VERSION_MAJOR   1
#define APP_VERSION_MINOR   0
#define APP_VERSION_BUILD   0
#define APP_VERSION_REVISION 0

// 应用程序配置
#define APP_NAME           L"InvoxApp"
#define APP_VERSION        L"" TOSTRING(APP_VERSION_MAJOR) "." TOSTRING(APP_VERSION_MINOR) "." TOSTRING(APP_VERSION_BUILD) "." TOSTRING(APP_VERSION_REVISION)
#define APP_PUBLISHER      L"YourCompany"
#define APP_EXE_NAME       L"InvoxApp.exe"
#define APP_UNINSTALL_NAME L"Uninstaller.exe"

// 版权信息
#define APP_COPYRIGHT      L"Copyright © 2025 YourCompany"
#define APP_DESCRIPTION    L"InvoxApp Installer & Uninstaller"

// 安装包文件
#define APP_ARCHIVE        L"app.7z"

// 注册表路径
#define REG_UNINSTALL_PATH L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\"

// 隐私政策 URL
#define PRIVACY_URL       L"https://yourcompany.com/privacy.html"

// 分析上报 API
#define ANALYTICS_ENDPOINT L"https://api.yourcompany.com/analytics"
