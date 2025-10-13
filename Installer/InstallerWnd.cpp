#include "StdAfx.h"
#include "InstallerWnd.h"
#include "resource.h"
#include "..\Common\Config.h"
#include "..\Common\InstallHelper.h"
#include "..\Common\Analytics.h"
#include <shlobj.h>
#include <shellapi.h>

#define WM_INSTALL_PROGRESS (WM_USER + 100)
#define WM_INSTALL_COMPLETE (WM_USER + 101)

CInstallerWnd::CInstallerWnd()
    : m_pAgreeCheck(NULL)
    , m_pAgreeCheckCustom(NULL)
    , m_pDesktopCheck(NULL)
    , m_pInstallBtn(NULL)
    , m_pCustomInstallBtn(NULL)
    , m_pBrowseBtn(NULL)
    , m_pPathEdit(NULL)
    , m_pSwitchCustomBtn(NULL)
    , m_pSwitchQuickBtn(NULL)
    , m_pProgress(NULL)
    , m_pProgressText(NULL)
    , m_pLaunchBtn(NULL)
    , m_pPage1(NULL)
    , m_pPage2(NULL)
    , m_pPage3(NULL)
    , m_pPage4(NULL)
    , m_hInstallThread(NULL)
    , m_bInstalling(false)
    , m_totalBytes(0)
    , m_processedBytes(0)
{
    // 默认安装路径
    m_strInstallPath = CInstallHelper::GetProgramFilesPath();
    m_strInstallPath = CInstallHelper::EnsureAppNameInPath(m_strInstallPath, APP_NAME);
}

CInstallerWnd::~CInstallerWnd()
{
    if (m_hInstallThread)
    {
        CloseHandle(m_hInstallThread);
        m_hInstallThread = NULL;
    }
}

CDuiString CInstallerWnd::GetSkinFile()
{
    return _T("installer.xml");
}

LPCTSTR CInstallerWnd::GetWindowClassName() const
{
    return _T("InstallerWindow");
}

LRESULT CInstallerWnd::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    SetIcon(IDR_MAINFRAME);

    // 在创建 UI 之前加载语言文件
    std::wstring lang = CInstallHelper::GetSystemLanguage();
    std::wstring langFile = L"resources\\lan_" + lang + L".xml";
    
    CResourceManager::GetInstance()->SetTextQueryInterface(this);
    CResourceManager::GetInstance()->LoadLanguage(langFile.c_str());
    
    // 调用基类的 OnCreate，它会创建 UI 并调用 InitWindow
    return WindowImplBase::OnCreate(uMsg, wParam, lParam, bHandled);
}

void CInstallerWnd::InitWindow()
{
    // 获取系统 DPI
    int sysDPI =  GetDeviceCaps(m_pm.GetPaintDC(), LOGPIXELSX);
    m_pm.SetAllDPI(sysDPI);
    
    // 获取标题栏
    m_pTitleBar = static_cast<CContainerUI*>(m_pm.FindControl(_T("titlebar")));
    
    // 获取页面控件
    m_pPage1 = static_cast<CContainerUI*>(m_pm.FindControl(_T("page1")));
    m_pPage2 = static_cast<CContainerUI*>(m_pm.FindControl(_T("page2")));
    m_pPage3 = static_cast<CContainerUI*>(m_pm.FindControl(_T("page3")));
    m_pPage4 = static_cast<CContainerUI*>(m_pm.FindControl(_T("page4")));
    
    // 获取第一页（快速安装）控件
    m_pAgreeCheck = static_cast<CCheckBoxUI*>(m_pm.FindControl(_T("agree_check")));
    m_pInstallBtn = static_cast<CButtonUI*>(m_pm.FindControl(_T("quick_install_btn")));
    m_pSwitchCustomBtn = static_cast<CButtonUI*>(m_pm.FindControl(_T("switch_custom_btn")));
    
    // 获取第二页（自定义安装）控件
    m_pAgreeCheckCustom = static_cast<CCheckBoxUI*>(m_pm.FindControl(_T("agree_check_custom")));
    m_pDesktopCheck = static_cast<CCheckBoxUI*>(m_pm.FindControl(_T("desktop_check")));
    m_pCustomInstallBtn = static_cast<CButtonUI*>(m_pm.FindControl(_T("custom_install_btn")));
    m_pBrowseBtn = static_cast<CButtonUI*>(m_pm.FindControl(_T("browse_btn")));
    m_pPathEdit = static_cast<CEditUI*>(m_pm.FindControl(_T("path_edit")));
    m_pSwitchQuickBtn = static_cast<CButtonUI*>(m_pm.FindControl(_T("switch_quick_btn")));
    
    // 获取第三页（安装进度）控件
    m_pProgress = static_cast<CProgressUI*>(m_pm.FindControl(_T("install_progress")));
    m_pProgressText = static_cast<CLabelUI*>(m_pm.FindControl(_T("progress_text")));
    
    // 获取第四页（安装完成）控件
    m_pLaunchBtn = static_cast<CButtonUI*>(m_pm.FindControl(_T("launch_btn")));
    
    // 初始化控件状态
    if (m_pPathEdit)
    {
        m_pPathEdit->SetText(m_strInstallPath.c_str());
    }
    
    if (m_pInstallBtn)
    {
        m_pInstallBtn->SetEnabled(false);
    }
    
    if (m_pCustomInstallBtn)
    {
        m_pCustomInstallBtn->SetEnabled(false);
    }
    
    if (m_pDesktopCheck)
    {
        m_pDesktopCheck->Selected(true);
    }

    // 显示第一页（快速安装页面）
    SwitchToPage(1);
    
    // 设置分析端点
    CAnalytics::GetInstance()->SetEndpoint(ANALYTICS_ENDPOINT);
}

LPCTSTR CInstallerWnd::QueryControlText(LPCTSTR lpstrId, LPCTSTR lpstrType)
{
    return NULL;
}

void CInstallerWnd::Notify(TNotifyUI& msg)
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
            if (m_bInstalling)
            {
                if (MessageBox(m_hWnd, _T("安装正在进行中，确定要退出吗？"), 
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
        else if (strName == _T("agreement_link") || strName == _T("agreement_link_custom"))
        {
            // 打开协议链接
            ShellExecute(NULL, _T("open"), AGREEMENT_URL, NULL, NULL, SW_SHOW);
        }
        else if (strName == _T("switch_custom_btn"))
        {
            // 切换到自定义安装页面
            SwitchToPage(2);
        }
        else if (strName == _T("switch_quick_btn"))
        {
            // 切换到快速安装页面
            SwitchToPage(1);
        }
        else if (strName == _T("browse_btn"))
        {
            BrowseInstallPath();
        }
        else if (strName == _T("quick_install_btn") || strName == _T("custom_install_btn"))
        {
            StartInstall();
        }
        else if (strName == _T("launch_btn"))
        {
            LaunchApplication();
            Close();
        }
        else
        {
            // 通用处理：检查是否有关联控件
            HandleRelatedControlClick(msg.pSender);
        }
    }
    else if (msg.sType == _T("selectchanged"))
    {
        CDuiString strName = msg.pSender->GetName();
        
        if (strName == _T("agree_check") || strName == _T("agree_check_custom"))
        {
            CCheckBoxUI* pCheckBox = dynamic_cast<CCheckBoxUI*>(msg.pSender);
            
            // 协议勾选状态改变（快速安装页面）
            if (pCheckBox && m_pInstallBtn)
            {
                m_pInstallBtn->SetEnabled(pCheckBox->IsSelected());
            }
            if (m_pAgreeCheck)
            {
                m_pAgreeCheck->SetCheck(pCheckBox->IsSelected());
            }
            
            // 协议勾选状态改变（自定义安装页面）
            if (pCheckBox && m_pCustomInstallBtn)
            {
                m_pCustomInstallBtn->SetEnabled(pCheckBox->IsSelected());
            }
            if (m_pAgreeCheckCustom)
            {
                m_pAgreeCheckCustom->SetCheck(pCheckBox->IsSelected());
            }
        }
    }
    else if (msg.sType == _T("textchanged"))
    {
        CDuiString strName = msg.pSender->GetName();
        
        if (strName == _T("path_edit"))
        {
            if (m_pPathEdit)
            {
                m_strInstallPath = m_pPathEdit->GetText().GetData();
            }
        }
    }
}

LRESULT CInstallerWnd::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_INSTALL_PROGRESS)
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
    else if (uMsg == WM_INSTALL_COMPLETE)
    {
        bool success = (bool)wParam;
        
        if (success)
        {
            // 切换到安装完成页面（第4页）
            SwitchToPage(4);
        }
        else
        {
            MessageBox(m_hWnd, _T("安装失败！"), _T("错误"), MB_OK | MB_ICONERROR);
            Close();
        }
        
        m_bInstalling = false;
        return 0;
    }
    
    return WindowImplBase::HandleMessage(uMsg, wParam, lParam);
}

void CInstallerWnd::SwitchToPage(int pageIndex)
{
    if (m_pPage1)
    {
        m_pPage1->SetVisible(pageIndex == 1);
    }
    
    if (m_pPage2)
    {
        m_pPage2->SetVisible(pageIndex == 2);
    }
    
    if (m_pPage3)
    {
        m_pPage3->SetVisible(pageIndex == 3);
    }
    
    if (m_pPage4)
    {
        m_pPage4->SetVisible(pageIndex == 4);
    }

    if (m_pTitleBar) {
        m_pTitleBar->SetBkColor(pageIndex == 2 ? 0xFFFB2C36 : 0x00000000);
    }
}

void CInstallerWnd::BrowseInstallPath()
{
    BROWSEINFO bi = { 0 };
    bi.hwndOwner = m_hWnd;
    bi.lpszTitle = _T("请选择安装目录");
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    
    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    if (pidl != NULL)
    {
        WCHAR szPath[MAX_PATH] = { 0 };
        if (SHGetPathFromIDList(pidl, szPath))
        {
            m_strInstallPath = CInstallHelper::EnsureAppNameInPath(szPath, APP_NAME);
            
            if (m_pPathEdit)
            {
                m_pPathEdit->SetText(m_strInstallPath.c_str());
            }
        }
        
        CoTaskMemFree(pidl);
    }
}

void CInstallerWnd::StartInstall()
{
    // 确保路径以应用名称结尾
    m_strInstallPath = CInstallHelper::EnsureAppNameInPath(m_strInstallPath, APP_NAME);
    
    // 切换到安装进度页面（第3页）
    SwitchToPage(3);
    
    // 启动安装线程
    m_bInstalling = true;
    m_hInstallThread = CreateThread(NULL, 0, InstallThreadProc, this, 0, NULL);
}

DWORD WINAPI CInstallerWnd::InstallThreadProc(LPVOID lpParam)
{
    CInstallerWnd* pThis = (CInstallerWnd*)lpParam;
    pThis->DoInstall();
    return 0;
}

void CInstallerWnd::DoInstall()
{
    bool success = false;
    
    try
    {
        // 更新进度：开始安装
        UpdateProgress(0, L"正在准备安装...");
        Sleep(500);
        
        WCHAR szTempPath[MAX_PATH] = { 0 };

#if defined(_DEBUG)
        // 获取当前程序所在目录
        GetModuleFileName(NULL, szTempPath, MAX_PATH);
        PathRemoveFileSpec(szTempPath);
        
        // 构建压缩包路径
        std::wstring archivePath = szTempPath;
        archivePath += L"\\";
        archivePath += APP_ARCHIVE;
        
        // 检查压缩包是否存在
        if (!PathFileExists(archivePath.c_str()))
        {
            UpdateProgress(0, L"找不到安装包文件！");
            Sleep(2000);
            ::PostMessage(m_hWnd, WM_INSTALL_COMPLETE, FALSE, 0);
            return;
        }
#else
        // 创建临时目录
        GetTempPath(MAX_PATH, szTempPath);
        std::wstring tempDir = szTempPath;
        tempDir += L"Installer_Temp\\";
        SHCreateDirectoryEx(NULL, tempDir.c_str(), NULL);
        
        // 从资源提取压缩包
        UpdateProgress(2, L"正在提取安装包...");
        std::wstring archivePath = tempDir + APP_ARCHIVE;
        HINSTANCE hInstance = CPaintManagerUI::GetInstance();
        
        // 验证资源是否存在（调试信息）
        HRSRC hResCheck = ::FindResource(hInstance, MAKEINTRESOURCE(IDR_APP_7Z), RT_RCDATA);
        if (hResCheck == NULL)
        {
            DWORD err = GetLastError();
            WCHAR errMsg[512];
            wsprintf(errMsg, L"找不到资源 IDR_APP_7Z (ID=%d)，错误代码: %d\n检查：\n1. 是否重新编译？\n2. bin\\app.7z 是否存在？\n3. installer.rc 配置是否正确？", IDR_APP_7Z, err);
            UpdateProgress(0, errMsg);
            OutputDebugString(errMsg);
            Sleep(5000);
            ::PostMessage(m_hWnd, WM_INSTALL_COMPLETE, FALSE, 0);
            return;
        }
        else
        {
            DWORD resSize = ::SizeofResource(hInstance, hResCheck);
            WCHAR sizeMsg[256];
            wsprintf(sizeMsg, L"[调试] 找到资源 IDR_APP_7Z，大小: %d 字节", resSize);
            OutputDebugString(sizeMsg);
        }

        if (!CInstallHelper::ExtractBinaryResource(hInstance, IDR_APP_7Z, archivePath))
        {
            UpdateProgress(0, L"提取安装包失败！请检查磁盘空间和权限。");
            Sleep(2000);
            ::PostMessage(m_hWnd, WM_INSTALL_COMPLETE, FALSE, 0);
            return;
        }
        
        Sleep(300);
#endif    // _DEBUG
        
        // 创建安装目录
        UpdateProgress(5, L"正在创建安装目录...");
        SHCreateDirectoryEx(NULL, m_strInstallPath.c_str(), NULL);
        Sleep(300);
        
        // 解压文件
        UpdateProgress(10, L"正在解压文件...");
        C7zExtractor extractor;
        extractor.SetProgressCallback(OnExtractProgress, this);
        
        m_totalBytes = extractor.GetArchiveSize(archivePath);
        m_processedBytes = 0;
        
        if (!extractor.Extract(archivePath, m_strInstallPath))
        {
            UpdateProgress(0, L"解压文件失败！");
            Sleep(2000);
            ::PostMessage(m_hWnd, WM_INSTALL_COMPLETE, FALSE, 0);
            return;
        }
        
        UpdateProgress(80, L"文件解压完成");
        Sleep(300);
        
        std::wstring exePath = m_strInstallPath + L"\\" + APP_EXE_NAME;
        
        // 创建开始菜单快捷方式（始终创建，用于 Windows 搜索）
        UpdateProgress(83, L"正在创建开始菜单快捷方式...");
        CInstallHelper::CreateStartMenuShortcut(exePath, APP_NAME);
        Sleep(300);
        
        // 创建桌面快捷方式（可选）
        if (m_pDesktopCheck && m_pDesktopCheck->IsSelected())
        {
            UpdateProgress(86, L"正在创建桌面快捷方式...");
            CInstallHelper::CreateDesktopShortcut(exePath, APP_NAME);
            Sleep(300);
        }
        
        // 写入注册表
        UpdateProgress(90, L"正在写入注册表...");
        std::wstring uninstallPath = m_strInstallPath + L"\\" + APP_UNINSTALL_NAME;
        UINT64 installSize = CInstallHelper::GetDirectorySize(m_strInstallPath);
        CInstallHelper::WriteUninstallRegistry(APP_NAME, APP_VERSION, APP_PUBLISHER,
            m_strInstallPath, uninstallPath, exePath, installSize);
        Sleep(300);
        
        // 上报安装信息
        UpdateProgress(95, L"正在上报安装信息...");
        CAnalytics::GetInstance()->ReportInstall(APP_NAME, APP_VERSION, m_strInstallPath);
        Sleep(300);
        
        // 安装完成
        UpdateProgress(100, L"安装完成！");
        Sleep(500);
        
#if ！defined(_DEBUG)
        // 清理临时文件
        DeleteFile(archivePath.c_str());
        RemoveDirectory(tempDir.c_str());
#endif    // _DEBUG
        
        success = true;
    }
    catch (...)
    {
        success = false;
    }
    
    ::PostMessage(m_hWnd, WM_INSTALL_COMPLETE, success, 0);
}

void CInstallerWnd::UpdateProgress(int percent, const std::wstring& text)
{
    std::wstring* pText = new std::wstring(text);
    ::PostMessage(m_hWnd, WM_INSTALL_PROGRESS, percent, (LPARAM)pText);
}

void CInstallerWnd::OnExtractProgress(UINT64 bytesProcessed, UINT64 totalBytes, void* userData)
{
    CInstallerWnd* pThis = (CInstallerWnd*)userData;
    if (pThis && totalBytes > 0)
    {
        int percent = (int)(10 + (bytesProcessed * 70 / totalBytes));
        
        WCHAR szText[256] = { 0 };
        wsprintf(szText, L"正在解压文件... %d%%", (int)(bytesProcessed * 100 / totalBytes));
        
        pThis->UpdateProgress(percent, szText);
    }
}

void CInstallerWnd::LaunchApplication()
{
    std::wstring exePath = m_strInstallPath + L"\\" + APP_EXE_NAME;
    ShellExecute(NULL, _T("open"), exePath.c_str(), NULL, m_strInstallPath.c_str(), SW_SHOW);
}

void CInstallerWnd::HandleRelatedControlClick(CControlUI* pControl)
{
    if (!pControl)
        return;

    // 获取控件的用户自定义数据（userData）
    // DuiLib 支持通过 SetUserData/GetUserData 存储自定义字符串
    CDuiString relatedControlName = pControl->GetUserData();
    
    if (relatedControlName.IsEmpty())
        return;

    // 查找关联的控件
    CControlUI* pRelatedControl = m_pm.FindControl(relatedControlName);
    if (!pRelatedControl)
        return;

    // 尝试将关联控件转换为 CheckBox
    CCheckBoxUI* pCheckBox = dynamic_cast<CCheckBoxUI*>(pRelatedControl);
    if (pCheckBox)
    {
        // 切换 CheckBox 的选中状态
        pCheckBox->Selected(!pCheckBox->IsSelected());
        
        // 触发 selectchanged 事件，让原有的业务逻辑继续工作
        m_pm.SendNotify(pCheckBox, DUI_MSGTYPE_SELECTCHANGED);
    }
    else
    {
        // 如果是 Option，也可以类似处理
        COptionUI* pOption = dynamic_cast<COptionUI*>(pRelatedControl);
        if (pOption)
        {
            pOption->Selected(!pOption->IsSelected());
            m_pm.SendNotify(pOption, DUI_MSGTYPE_SELECTCHANGED);
        }
    }
}
