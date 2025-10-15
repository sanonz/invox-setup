#include "StdAfx.h"
#include "InstallerWnd.h"
#include "resource.h"
#include "..\Common\Config.h"
#include "..\Common\InstallHelper.h"
#include "..\Common\Analytics.h"
#include "..\Common\Logger.h"
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
    , m_currentStep(STEP_NONE)
    , m_startMenuCreated(false)
    , m_desktopShortcutCreated(false)
    , m_registryWritten(false)
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
    return _T("InvoxInstallerWindow");
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

std::wstring CInstallerWnd::GetLocalizedText(const std::wstring& textId)
{
    CDuiString strText = CResourceManager::GetInstance()->GetText(textId.c_str());
    if (strText.IsEmpty())
    {
        // 如果找不到翻译，返回原始ID作为备用
        return textId;
    }
    return std::wstring(strText.GetData());
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
                std::wstring confirmMsg = GetLocalizedText(L"msgbox_confirm_exit_message");
                std::wstring confirmTitle = GetLocalizedText(L"msgbox_confirm_exit_title");
                if (MessageBox(m_hWnd, confirmMsg.c_str(), 
                    confirmTitle.c_str(), MB_YESNO | MB_ICONWARNING) != IDYES)
                {
                    return;
                }
                
                // 用户确认退出，执行回滚
                CLogger::GetInstance()->LogWarning(L"User clicked close button during installation, starting rollback...");
                
                // 如果安装线程还在运行，等待其结束（最多等待3秒）
                if (m_hInstallThread)
                {
                    // 设置标志让安装线程知道需要停止
                    m_bInstalling = false;
                    
                    // 等待线程结束
                    DWORD dwWaitResult = WaitForSingleObject(m_hInstallThread, 3000);
                    if (dwWaitResult == WAIT_TIMEOUT)
                    {
                        CLogger::GetInstance()->LogWarning(L"Installation thread did not respond in time, force terminating");
                        TerminateThread(m_hInstallThread, 0);
                    }
                    CloseHandle(m_hInstallThread);
                    m_hInstallThread = NULL;
                }
                
                // 执行回滚操作
                RollbackInstallation();
                
                CLogger::GetInstance()->LogInfo(L"Rollback completed, program will exit");
            }
            Close();
        }
        else if (strName == _T("minbtn"))
        {
            SendMessage(WM_SYSCOMMAND, SC_MINIMIZE, 0);
        }
        else if (strName == _T("privacy_link") || strName == _T("privacy_link_custom"))
        {
            // 打开协议链接
            ShellExecute(NULL, _T("open"), PRIVACY_URL, NULL, NULL, SW_SHOW);
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
            std::wstring failedMsg = GetLocalizedText(L"msgbox_install_failed_message");
            std::wstring failedTitle = GetLocalizedText(L"msgbox_install_failed_title");
            MessageBox(m_hWnd, failedMsg.c_str(), failedTitle.c_str(), MB_OK | MB_ICONERROR);
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
    std::wstring browseFolderTitle = GetLocalizedText(L"msgbox_browse_folder_title");
    BROWSEINFO bi = { 0 };
    bi.hwndOwner = m_hWnd;
    bi.lpszTitle = browseFolderTitle.c_str();
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
        // 初始化日志
        std::wstring logPath = m_strInstallPath + L"\\install.log";
        CLogger::GetInstance()->SetLogFile(logPath);
        CLogger::GetInstance()->LogInfo(L"========== Installation Started ==========");
        CLogger::GetInstance()->LogFormat(LOG_INFO, L"Install Path: %s", m_strInstallPath.c_str());
        
        // 更新进度：开始安装
        UpdateProgress(0, L"progress_preparing");
        Sleep(500);
        
        // 检查目标程序是否正在运行
        if (CInstallHelper::IsProcessRunning(APP_EXE_NAME))
        {
            CLogger::GetInstance()->LogWarning(L"Target application is already running");
            UpdateProgress(0, L"progress_app_running");
            Sleep(3000);
            ::PostMessage(m_hWnd, WM_INSTALL_COMPLETE, FALSE, 0);
            return;
        }
        
        // 安装前检查
        CInstallHelper::InstallErrorCode checkResult = CInstallHelper::PreInstallCheck(m_strInstallPath);
        if (checkResult != CInstallHelper::INSTALL_ERR_SUCCESS)
        {
            std::wstring errorMsg;
            switch (checkResult)
            {
            case CInstallHelper::INSTALL_ERR_INSUFFICIENT_PRIVILEGE:
                errorMsg = L"progress_insufficient_privilege";
                CLogger::GetInstance()->LogError(L"Insufficient privilege");
                break;
            case CInstallHelper::INSTALL_ERR_PATH_TOO_LONG:
                errorMsg = L"progress_path_too_long";
                CLogger::GetInstance()->LogError(L"Path too long");
                break;
            case CInstallHelper::INSTALL_ERR_INVALID_PATH:
                errorMsg = L"progress_invalid_path";
                CLogger::GetInstance()->LogError(L"Invalid path");
                break;
            default:
                errorMsg = L"progress_unknown_error";
                CLogger::GetInstance()->LogError(L"Unknown error");
                break;
            }
            
            UpdateProgress(0, errorMsg);
            Sleep(3000);
            ::PostMessage(m_hWnd, WM_INSTALL_COMPLETE, FALSE, 0);
            return;
        }
        
        WCHAR szTempPath[MAX_PATH] = { 0 };
        std::wstring str7zDllPath;

#if defined(_DEBUG)
        // 获取当前程序所在目录
        GetModuleFileName(NULL, szTempPath, MAX_PATH);
        PathRemoveFileSpec(szTempPath);
        
        // 构建压缩包路径
        m_tempDir = szTempPath;
        m_tempDir += L"\\";
        m_tempDir += APP_ARCHIVE;

        // 检查压缩包是否存在
        if (!PathFileExists(m_tempDir.c_str()))
        {
            UpdateProgress(0, L"progress_archive_not_found");
            Sleep(2000);
            ::PostMessage(m_hWnd, WM_INSTALL_COMPLETE, FALSE, 0);
            return;
        }
        
        m_archivePath = m_tempDir;

        // 构建 7zxa.dll 路径
        m_tempDir = szTempPath;
        m_tempDir = m_tempDir.substr(0, m_tempDir.length() - 3);
        m_tempDir += L"3rd\\7zxa.dll";

        // 检查 7zxa.dll 是否存在
        if (!PathFileExists(m_tempDir.c_str()))
        {
            UpdateProgress(0, L"progress_7zdll_not_found");
            Sleep(2000);
            ::PostMessage(m_hWnd, WM_INSTALL_COMPLETE, FALSE, 0);
            return;
        }

        str7zDllPath = m_tempDir;
#else
        // 创建临时目录
        GetTempPath(MAX_PATH, szTempPath);
        m_tempDir = szTempPath;
        m_tempDir += L"Installer_Temp\\";
        
        HRESULT hrDir = SHCreateDirectoryEx(NULL, m_tempDir.c_str(), NULL);
        if (FAILED(hrDir) && hrDir != HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS))
        {
            CLogger::GetInstance()->LogFormat(LOG_ERROR, L"Failed to create temp directory: 0x%08X", hrDir);
            UpdateProgress(0, L"progress_create_temp_failed");
            Sleep(2000);
            ::PostMessage(m_hWnd, WM_INSTALL_COMPLETE, FALSE, 0);
            return;
        }
        
        m_currentStep = STEP_TEMP_DIR_CREATED;
        CLogger::GetInstance()->LogInfo(L"Temp directory created successfully");
        
        // 检查是否需要中止
        if (!m_bInstalling)
        {
            CLogger::GetInstance()->LogWarning(L"Installation aborted by user");
            RollbackInstallation();
            ::PostMessage(m_hWnd, WM_INSTALL_COMPLETE, FALSE, 0);
            return;
        }
        
        // 从资源提取压缩包
        UpdateProgress(2, L"progress_extracting_package");
        m_archivePath = m_tempDir + APP_ARCHIVE;
        HINSTANCE hInstance = CPaintManagerUI::GetInstance();
        
        // 验证资源是否存在
        HRSRC h7zRes = ::FindResource(hInstance, MAKEINTRESOURCE(IDR_7ZXA_DLL), RT_RCDATA);
        if (h7zRes == NULL)
        {
            UpdateProgress(0, L"progress_resource_7zdll_not_found");
            Sleep(5000);
            ::PostMessage(m_hWnd, WM_INSTALL_COMPLETE, FALSE, 0);
            return;
        }

        str7zDllPath = m_tempDir;
        str7zDllPath += L"7zxa.dll";
        
        HRSRC hAppRes = ::FindResource(hInstance, MAKEINTRESOURCE(IDR_APP_7Z), RT_RCDATA);
        if (hAppRes == NULL)
        {
            UpdateProgress(0, L"progress_resource_app_not_found");
            Sleep(5000);
            ::PostMessage(m_hWnd, WM_INSTALL_COMPLETE, FALSE, 0);
            return;
        }
        
        // 记录需要的空间大小
        UINT64 requiredSize = ::SizeofResource(hInstance, h7zRes);
        requiredSize += ::SizeofResource(hInstance, hAppRes);

        // 检查磁盘空间
        std::wstring driveTempPath = CInstallHelper::GetDrivePath(m_tempDir);
        if (!CInstallHelper::CheckDiskSpace(driveTempPath, requiredSize))
        {
            UpdateProgress(0, L"progress_disk_full");
            CLogger::GetInstance()->LogError(L"Disk full");
            Sleep(3000);
            ::PostMessage(m_hWnd, WM_INSTALL_COMPLETE, FALSE, 0);
            return;
        }
        
        if (
            !CInstallHelper::ExtractBinaryResource(hInstance, IDR_7ZXA_DLL, str7zDllPath) ||
            !CInstallHelper::ExtractBinaryResource(hInstance, IDR_APP_7Z, m_archivePath)
        )
        {
            CLogger::GetInstance()->LogError(L"Failed to extract installation package");
            UpdateProgress(0, L"progress_extract_package_failed");
            Sleep(2000);
            RollbackInstallation();
            ::PostMessage(m_hWnd, WM_INSTALL_COMPLETE, FALSE, 0);
            return;
        }
        
        m_currentStep = STEP_ARCHIVE_EXTRACTED;
        CLogger::GetInstance()->LogInfo(L"Installation package extracted successfully");
        Sleep(300);
#endif    // _DEBUG

        C7zExtractor *extractor = new C7zExtractor(str7zDllPath);
        extractor->SetProgressCallback(OnExtractProgress, this);

        // 打开压缩包
        if (!extractor->Open(m_archivePath))
        {
            CLogger::GetInstance()->LogError(L"Failed to open archive");
            UpdateProgress(0, L"progress_open_archive_failed");
            Sleep(2000);
            RollbackInstallation();
            ::PostMessage(m_hWnd, WM_INSTALL_COMPLETE, FALSE, 0);
            return;
        }
        
        // 获取解压后的总大小用于进度计算
        m_totalBytes = extractor->GetUncompressedSize();
        
        // 检查磁盘空间
        std::wstring drivePath = CInstallHelper::GetDrivePath(m_strInstallPath);
        if (!CInstallHelper::CheckDiskSpace(drivePath, m_totalBytes))
        {
            UpdateProgress(0, L"progress_disk_full");
            CLogger::GetInstance()->LogError(L"Disk full");
            Sleep(3000);
            extractor->Close();
            ::PostMessage(m_hWnd, WM_INSTALL_COMPLETE, FALSE, 0);
            return;
        }
        
        // 创建安装目录
        UpdateProgress(5, L"progress_creating_install_dir");
        HRESULT hrInstallDir = SHCreateDirectoryEx(NULL, m_strInstallPath.c_str(), NULL);
        if (FAILED(hrInstallDir) && hrInstallDir != HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS))
        {
            CLogger::GetInstance()->LogFormat(LOG_ERROR, L"Failed to create install directory: 0x%08X", hrInstallDir);
            UpdateProgress(0, L"progress_create_install_dir_failed");
            Sleep(2000);
            delete extractor;
            RollbackInstallation();
            ::PostMessage(m_hWnd, WM_INSTALL_COMPLETE, FALSE, 0);
            return;
        }
        
        m_currentStep = STEP_INSTALL_DIR_CREATED;
        CLogger::GetInstance()->LogInfo(L"Install directory created successfully");
        Sleep(300);
        
        // 检查是否需要中止
        if (!m_bInstalling)
        {
            CLogger::GetInstance()->LogWarning(L"Installation aborted by user");
            delete extractor;
            RollbackInstallation();
            ::PostMessage(m_hWnd, WM_INSTALL_COMPLETE, FALSE, 0);
            return;
        }
        
        // 解压文件
        UpdateProgress(10, L"progress_extracting_files");
        if (!extractor->Extract(m_strInstallPath))
        {
            CLogger::GetInstance()->LogError(L"Failed to extract files");
            UpdateProgress(0, L"progress_extract_files_failed");
            Sleep(2000);
            delete extractor;
            RollbackInstallation();
            ::PostMessage(m_hWnd, WM_INSTALL_COMPLETE, FALSE, 0);
            return;
        }
        
        // 关闭压缩包
        delete extractor;
        
        m_currentStep = STEP_FILES_EXTRACTED;
        CLogger::GetInstance()->LogInfo(L"Files extracted successfully");
        UpdateProgress(80, L"progress_files_extracted");
        Sleep(300);
        
        // 检查是否需要中止
        if (!m_bInstalling)
        {
            CLogger::GetInstance()->LogWarning(L"Installation aborted by user");
            RollbackInstallation();
            ::PostMessage(m_hWnd, WM_INSTALL_COMPLETE, FALSE, 0);
            return;
        }
        
        std::wstring exePath = m_strInstallPath + L"\\" + APP_EXE_NAME;
        
        // 创建开始菜单快捷方式（始终创建，用于 Windows 搜索）
        UpdateProgress(83, L"progress_creating_start_menu");
        if (CInstallHelper::CreateStartMenuShortcut(exePath, APP_NAME))
        {
            m_currentStep = STEP_START_MENU_CREATED;
            m_startMenuCreated = true;
            CLogger::GetInstance()->LogInfo(L"Start menu shortcut created successfully");
        }
        else
        {
            CLogger::GetInstance()->LogWarning(L"Failed to create start menu shortcut");
        }
        Sleep(300);
        
        // 创建桌面快捷方式（可选）
        if (m_pDesktopCheck && m_pDesktopCheck->IsSelected())
        {
            UpdateProgress(86, L"progress_creating_desktop_shortcut");
            if (CInstallHelper::CreateDesktopShortcut(exePath, APP_NAME))
            {
                m_currentStep = STEP_DESKTOP_SHORTCUT_CREATED;
                m_desktopShortcutCreated = true;
                CLogger::GetInstance()->LogInfo(L"Desktop shortcut created successfully");
            }
            else
            {
                CLogger::GetInstance()->LogWarning(L"Failed to create desktop shortcut");
            }
            Sleep(300);
        }
        
        // 写入注册表
        UpdateProgress(90, L"progress_writing_registry");
        std::wstring uninstallPath = m_strInstallPath + L"\\" + APP_UNINSTALL_NAME;
        if (CInstallHelper::WriteUninstallRegistry(APP_NAME, APP_VERSION, APP_PUBLISHER,
            m_strInstallPath, uninstallPath, exePath, m_totalBytes))
        {
            m_currentStep = STEP_REGISTRY_WRITTEN;
            m_registryWritten = true;
            CLogger::GetInstance()->LogInfo(L"Registry written successfully");
        }
        else
        {
            CLogger::GetInstance()->LogWarning(L"Failed to write registry");
        }
        Sleep(300);
        
        // 上报安装信息
        UpdateProgress(95, L"progress_reporting_analytics");
        CAnalytics::GetInstance()->ReportInstall(APP_NAME, APP_VERSION, m_strInstallPath);
        Sleep(300);
        
        // 安装完成
        UpdateProgress(100, L"progress_installation_complete");
        m_currentStep = STEP_COMPLETED;
        CLogger::GetInstance()->LogInfo(L"Installation completed successfully");
        Sleep(500);
        
#if !defined(_DEBUG)
        // 清理临时文件
        CInstallHelper::RemoveDirectory(m_tempDir.c_str());
        CLogger::GetInstance()->LogInfo(L"Temporary files cleaned up");
#endif    // _DEBUG
        
        success = true;
    }
    catch (...)
    {
        CLogger::GetInstance()->LogError(L"An exception occurred during the installation process");
        success = false;
        RollbackInstallation();
    }
    
    if (!success)
    {
        CLogger::GetInstance()->LogError(L"Installation failed");
    }
    
    ::PostMessage(m_hWnd, WM_INSTALL_COMPLETE, success, 0);
}

void CInstallerWnd::RollbackInstallation()
{
    CLogger::GetInstance()->LogWarning(L"========== Starting Installation Rollback ==========");
    UpdateProgress(0, L"progress_rollback_cleaning");
    
    // 根据当前步骤逆序清理
    
    // 删除注册表
    if (m_registryWritten)
    {
        CLogger::GetInstance()->LogInfo(L"Cleaning up registry...");
        CInstallHelper::RemoveUninstallRegistry(APP_NAME);
    }
    
    // 删除桌面快捷方式
    if (m_desktopShortcutCreated)
    {
        CLogger::GetInstance()->LogInfo(L"Cleaning up desktop shortcut...");
        WCHAR szDesktopPath[MAX_PATH] = { 0 };
        SHGetFolderPath(NULL, CSIDL_DESKTOP, NULL, 0, szDesktopPath);
        std::wstring shortcutPath = szDesktopPath;
        shortcutPath += L"\\";
        shortcutPath += APP_NAME;
        shortcutPath += L".lnk";
        DeleteFile(shortcutPath.c_str());
    }
    
    // 删除开始菜单快捷方式
    if (m_startMenuCreated)
    {
        CLogger::GetInstance()->LogInfo(L"Cleaning up start menu shortcut...");
        CInstallHelper::RemoveStartMenuShortcut(APP_NAME);
    }
    
    // 删除安装文件
    if (m_currentStep >= STEP_FILES_EXTRACTED)
    {
        CLogger::GetInstance()->LogInfo(L"Cleaning up installation files...");
        CInstallHelper::RemoveDirectory(m_strInstallPath);
    }
    
#if !defined(_DEBUG)
    // 删除临时文件
    if (m_currentStep >= STEP_ARCHIVE_EXTRACTED)
    {
        CLogger::GetInstance()->LogInfo(L"Cleaning up temp files...");
        if (!m_archivePath.empty())
        {
            DeleteFile(m_archivePath.c_str());
        }
    }
    
    // 删除临时目录
    if (m_currentStep >= STEP_TEMP_DIR_CREATED)
    {
        if (!m_tempDir.empty())
        {
            RemoveDirectory(m_tempDir.c_str());
        }
    }
#endif    // _DEBUG
    
    CLogger::GetInstance()->LogInfo(L"Rollback completed");
    
    // 重置状态
    m_currentStep = STEP_NONE;
    m_startMenuCreated = false;
    m_desktopShortcutCreated = false;
    m_registryWritten = false;
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
        std::wstring formatText = pThis->GetLocalizedText(L"progress_extracting_with_percent");
        wsprintf(szText, formatText.c_str(), (int)(bytesProcessed * 100 / totalBytes));
        
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
