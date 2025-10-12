#include "StdAfx.h"
#include "UninstallerWnd.h"
#include "..\Common\Config.h"
#include "..\Common\InstallHelper.h"
#include "..\Common\Analytics.h"
#include <shlobj.h>

#define WM_UNINSTALL_PROGRESS (WM_USER + 100)
#define WM_UNINSTALL_COMPLETE (WM_USER + 101)

CUninstallerWnd::CUninstallerWnd()
    : m_pReasonOption1(NULL)
    , m_pReasonOption2(NULL)
    , m_pReasonOption3(NULL)
    , m_pFeedbackEdit(NULL)
    , m_pUninstallBtn(NULL)
    , m_pProgress(NULL)
    , m_pProgressText(NULL)
    , m_pCompleteText(NULL)
    , m_pGoodbyeBtn(NULL)
    , m_pPage1(NULL)
    , m_pPage2(NULL)
    , m_hUninstallThread(NULL)
    , m_bUninstalling(false)
{
    // 从注册表读取安装路径
    std::wstring regPath = std::wstring(REG_UNINSTALL_PATH) + APP_NAME;
    
    HKEY hKey = NULL;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, regPath.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS ||
        RegOpenKeyEx(HKEY_CURRENT_USER, regPath.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        WCHAR szPath[MAX_PATH] = { 0 };
        DWORD dwSize = sizeof(szPath);
        if (RegQueryValueEx(hKey, L"InstallLocation", NULL, NULL, (LPBYTE)szPath, &dwSize) == ERROR_SUCCESS)
        {
            m_strInstallPath = szPath;
        }
        RegCloseKey(hKey);
    }
    
    if (m_strInstallPath.empty())
    {
        // 使用默认路径
        m_strInstallPath = CInstallHelper::GetProgramFilesPath();
        m_strInstallPath = CInstallHelper::EnsureAppNameInPath(m_strInstallPath, APP_NAME);
    }
}

CUninstallerWnd::~CUninstallerWnd()
{
    if (m_hUninstallThread)
    {
        CloseHandle(m_hUninstallThread);
        m_hUninstallThread = NULL;
    }
}

CDuiString CUninstallerWnd::GetSkinFile()
{
    return _T("uninstaller.xml");
}

LPCTSTR CUninstallerWnd::GetWindowClassName() const
{
    return _T("UninstallerWindow");
}

LRESULT CUninstallerWnd::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // 在创建 UI 之前加载语言文件
    std::wstring lang = CInstallHelper::GetSystemLanguage();
    std::wstring langFile = L"resources\\lan_" + lang + L".xml";
    
    CResourceManager::GetInstance()->SetTextQueryInterface(this);
    CResourceManager::GetInstance()->LoadLanguage(langFile.c_str());
    
    // 调用基类的 OnCreate，它会创建 UI 并调用 InitWindow
    return WindowImplBase::OnCreate(uMsg, wParam, lParam, bHandled);
}

void CUninstallerWnd::InitWindow()
{
    // 获取控件
    m_pPage1 = static_cast<CContainerUI*>(m_pm.FindControl(_T("page1")));
    m_pPage2 = static_cast<CContainerUI*>(m_pm.FindControl(_T("page2")));
    
    m_pReasonOption1 = static_cast<COptionUI*>(m_pm.FindControl(_T("reason1")));
    m_pReasonOption2 = static_cast<COptionUI*>(m_pm.FindControl(_T("reason2")));
    m_pReasonOption3 = static_cast<COptionUI*>(m_pm.FindControl(_T("reason3")));
    m_pFeedbackEdit = static_cast<CRichEditUI*>(m_pm.FindControl(_T("feedback_edit")));
    m_pUninstallBtn = static_cast<CButtonUI*>(m_pm.FindControl(_T("uninstall_btn")));
    
    m_pProgress = static_cast<CProgressUI*>(m_pm.FindControl(_T("uninstall_progress")));
    m_pProgressText = static_cast<CLabelUI*>(m_pm.FindControl(_T("progress_text")));
    m_pCompleteText = static_cast<CLabelUI*>(m_pm.FindControl(_T("complete_text")));
    m_pGoodbyeBtn = static_cast<CButtonUI*>(m_pm.FindControl(_T("goodbye_btn")));
    
    // 初始化控件状态
    if (m_pReasonOption1)
    {
        m_pReasonOption1->Selected(true);
    }
    
    if (m_pCompleteText)
    {
        m_pCompleteText->SetVisible(false);
    }
    
    if (m_pGoodbyeBtn)
    {
        m_pGoodbyeBtn->SetVisible(false);
    }

    // 显示第一页
    SwitchToPage(1);
    
    // 设置分析端点
    CAnalytics::GetInstance()->SetEndpoint(ANALYTICS_ENDPOINT);
}

LPCTSTR CUninstallerWnd::QueryControlText(LPCTSTR lpstrId, LPCTSTR lpstrType)
{
    return NULL;
}

void CUninstallerWnd::Notify(TNotifyUI& msg)
{
    if (msg.sType == _T("windowinit"))
    {
        // 窗口初始化
    }
    else if (msg.sType == _T("click"))
    {
        CDuiString strName = msg.pSender->GetName();
        
        if (strName == _T("closebtn"))
        {
            if (m_bUninstalling)
            {
                if (MessageBox(m_hWnd, _T("卸载正在进行中，确定要退出吗？"), 
                    _T("提示"), MB_YESNO | MB_ICONQUESTION) != IDYES)
                {
                    return;
                }
            }
            Close();
        }
        else if (strName == _T("minbtn"))
        {
            SendMessage(WM_SYSCOMMAND, SC_MINIMIZE, 0);
        }
        else if (strName == _T("uninstall_btn"))
        {
            m_strReason = GetSelectedReason();
            m_strFeedback = GetFeedback();
            StartUninstall();
        }
        else if (strName == _T("goodbye_btn"))
        {
            SelfDelete();
            Close();
        }
    }
}

LRESULT CUninstallerWnd::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_UNINSTALL_PROGRESS)
    {
        int percent = (int)wParam;
        std::wstring* pText = (std::wstring*)lParam;
        
        if (m_pProgress)
        {
            m_pProgress->SetValue(percent);
        }
        
        if (m_pProgressText && pText)
        {
            m_pProgressText->SetText(pText->c_str());
            delete pText;
        }
        
        return 0;
    }
    else if (uMsg == WM_UNINSTALL_COMPLETE)
    {
        bool success = (bool)wParam;
        
        if (success)
        {
            if (m_pProgress)
            {
                m_pProgress->SetVisible(false);
            }
            
            if (m_pProgressText)
            {
                m_pProgressText->SetVisible(false);
            }
            
            if (m_pCompleteText)
            {
                m_pCompleteText->SetVisible(true);
            }
            
            if (m_pGoodbyeBtn)
            {
                m_pGoodbyeBtn->SetVisible(true);
            }
        }
        else
        {
            MessageBox(m_hWnd, _T("卸载失败！"), _T("错误"), MB_OK | MB_ICONERROR);
            Close();
        }
        
        m_bUninstalling = false;
        return 0;
    }
    
    return WindowImplBase::HandleMessage(uMsg, wParam, lParam);
}

void CUninstallerWnd::SwitchToPage(int pageIndex)
{
    if (m_pPage1)
    {
        m_pPage1->SetVisible(pageIndex == 1);
    }
    
    if (m_pPage2)
    {
        m_pPage2->SetVisible(pageIndex == 2);
    }
}

void CUninstallerWnd::StartUninstall()
{
    // 切换到卸载页面
    SwitchToPage(2);
    
    // 启动卸载线程
    m_bUninstalling = true;
    m_hUninstallThread = CreateThread(NULL, 0, UninstallThreadProc, this, 0, NULL);
}

DWORD WINAPI CUninstallerWnd::UninstallThreadProc(LPVOID lpParam)
{
    CUninstallerWnd* pThis = (CUninstallerWnd*)lpParam;
    pThis->DoUninstall();
    return 0;
}

void CUninstallerWnd::DoUninstall()
{
    bool success = false;
    
    try
    {
        // 更新进度：开始卸载
        UpdateProgress(0, L"正在准备卸载...");
        Sleep(500);
        
        // 上报卸载信息
        UpdateProgress(5, L"正在上报卸载信息...");
        CAnalytics::GetInstance()->ReportUninstall(APP_NAME, m_strReason, m_strFeedback);
        Sleep(300);
        
        // 计算文件总大小
        UpdateProgress(10, L"正在计算文件大小...");
        UINT64 totalSize = CInstallHelper::GetDirectorySize(m_strInstallPath);
        Sleep(300);
        
        // 删除桌面快捷方式
        UpdateProgress(20, L"正在删除快捷方式...");
        WCHAR szDesktopPath[MAX_PATH] = { 0 };
        SHGetFolderPath(NULL, CSIDL_DESKTOP, NULL, 0, szDesktopPath);
        std::wstring shortcutPath = szDesktopPath;
        shortcutPath += L"\\";
        shortcutPath += APP_NAME;
        shortcutPath += L".lnk";
        DeleteFile(shortcutPath.c_str());
        Sleep(300);
        
        // 删除安装文件
        UpdateProgress(30, L"正在删除文件...");
        
        // 模拟删除进度
        for (int i = 30; i <= 80; i += 5)
        {
            UpdateProgress(i, L"正在删除文件...");
            Sleep(200);
        }
        
        // 删除除卸载程序外的所有文件
        std::wstring searchPath = m_strInstallPath + L"\\*.*";
        WIN32_FIND_DATA findData;
        HANDLE hFind = FindFirstFile(searchPath.c_str(), &findData);
        
        if (hFind != INVALID_HANDLE_VALUE)
        {
            do
            {
                if (wcscmp(findData.cFileName, L".") == 0 || 
                    wcscmp(findData.cFileName, L"..") == 0 ||
                    _wcsicmp(findData.cFileName, APP_UNINSTALL_NAME) == 0)
                    continue;
                
                std::wstring fullPath = m_strInstallPath + L"\\" + findData.cFileName;
                
                if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    CInstallHelper::RemoveDirectory(fullPath);
                }
                else
                {
                    SetFileAttributes(fullPath.c_str(), FILE_ATTRIBUTE_NORMAL);
                    DeleteFile(fullPath.c_str());
                }
            } while (FindNextFile(hFind, &findData));
            
            FindClose(hFind);
        }
        
        UpdateProgress(85, L"文件删除完成");
        Sleep(300);
        
        // 删除开始菜单快捷方式
        UpdateProgress(87, L"正在清理开始菜单快捷方式...");
        CInstallHelper::RemoveStartMenuShortcut(APP_NAME);
        Sleep(300);
        
        // 删除注册表
        UpdateProgress(90, L"正在清理注册表...");
        CInstallHelper::RemoveUninstallRegistry(APP_NAME);
        Sleep(300);
        
        // 卸载完成
        UpdateProgress(100, L"卸载完成！");
        Sleep(500);
        
        success = true;
    }
    catch (...)
    {
        success = false;
    }
    
    ::PostMessage(m_hWnd, WM_UNINSTALL_COMPLETE, success, 0);
}

void CUninstallerWnd::UpdateProgress(int percent, const std::wstring& text)
{
    std::wstring* pText = new std::wstring(text);
    ::PostMessage(m_hWnd, WM_UNINSTALL_PROGRESS, percent, (LPARAM)pText);
}

std::wstring CUninstallerWnd::GetSelectedReason()
{
    if (m_pReasonOption1 && m_pReasonOption1->IsSelected())
    {
        return L"不需要了";
    }
    else if (m_pReasonOption2 && m_pReasonOption2->IsSelected())
    {
        return L"安装失败";
    }
    else if (m_pReasonOption3 && m_pReasonOption3->IsSelected())
    {
        return L"其他";
    }
    return L"未知";
}

std::wstring CUninstallerWnd::GetFeedback()
{
    if (m_pFeedbackEdit)
    {
        return m_pFeedbackEdit->GetText().GetData();
    }
    return L"";
}

void CUninstallerWnd::SelfDelete()
{
    // 获取当前程序路径
    WCHAR szModulePath[MAX_PATH] = { 0 };
    GetModuleFileName(NULL, szModulePath, MAX_PATH);
    
    // 创建批处理文件来删除自身
    WCHAR szTempPath[MAX_PATH] = { 0 };
    GetTempPath(MAX_PATH, szTempPath);
    
    std::wstring batPath = szTempPath;
    batPath += L"uninstall_cleanup.bat";
    
    // 创建批处理文件
    FILE* fp = NULL;
    _wfopen_s(&fp, batPath.c_str(), L"w");
    if (fp)
    {
        fwprintf(fp, L"@echo off\n");
        fwprintf(fp, L"timeout /t 2 /nobreak > nul\n");
        fwprintf(fp, L"del /f /q \"%s\"\n", szModulePath);
        fwprintf(fp, L"rd /s /q \"%s\"\n", m_strInstallPath.c_str());
        fwprintf(fp, L"del /f /q \"%s\"\n", batPath.c_str());
        fclose(fp);
        
        // 执行批处理
        STARTUPINFO si = { 0 };
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
        
        PROCESS_INFORMATION pi = { 0 };
        
        std::wstring cmdLine = L"cmd.exe /c \"" + batPath + L"\"";
        WCHAR szCmdLine[1024] = { 0 };
        wcscpy_s(szCmdLine, cmdLine.c_str());
        
        CreateProcess(NULL, szCmdLine, NULL, NULL, FALSE, 
            CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
        
        if (pi.hProcess)
        {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
    }
}
