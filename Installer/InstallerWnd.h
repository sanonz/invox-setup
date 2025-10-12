#pragma once
#include "..\DuiLib\UIlib.h"
#include "7zExtractor.h"
#include <string>

using namespace DuiLib;

// 安装主窗口
class CInstallerWnd : public WindowImplBase
{
public:
    CInstallerWnd();
    ~CInstallerWnd();

public:
    virtual CDuiString GetSkinFile();
    virtual LPCTSTR GetWindowClassName() const;
    virtual void InitWindow();
    virtual void Notify(TNotifyUI& msg);
    virtual LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
    virtual LPCTSTR QueryControlText(LPCTSTR lpstrId, LPCTSTR lpstrType);

protected:
    // 界面控件
    CCheckBoxUI* m_pAgreeCheck;
    CCheckBoxUI* m_pCustomCheck;
    CCheckBoxUI* m_pDesktopCheck;
    CButtonUI* m_pInstallBtn;
    CButtonUI* m_pBrowseBtn;
    CEditUI* m_pPathEdit;
    CVerticalLayoutUI* m_pCustomLayout;
    
    CProgressUI* m_pProgress;
    CLabelUI* m_pProgressText;
    CButtonUI* m_pLaunchBtn;
    
    CContainerUI* m_pPage1;
    CContainerUI* m_pPage2;

private:
    // 切换页面
    void SwitchToPage(int pageIndex);
    
    // 浏览安装路径
    void BrowseInstallPath();
    
    // 开始安装
    void StartInstall();
    
    // 安装线程函数
    static DWORD WINAPI InstallThreadProc(LPVOID lpParam);
    void DoInstall();
    
    // 更新进度
    void UpdateProgress(int percent, const std::wstring& text);
    
    // 7z 解压进度回调
    static void OnExtractProgress(UINT64 bytesProcessed, UINT64 totalBytes, void* userData);
    
    // 启动应用
    void LaunchApplication();

private:
    std::wstring m_strInstallPath;
    HANDLE m_hInstallThread;
    bool m_bInstalling;
    UINT64 m_totalBytes;
    UINT64 m_processedBytes;
};
