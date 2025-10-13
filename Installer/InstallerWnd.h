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
    // 标题栏
    CContainerUI* m_pTitleBar;
    
    // 页面容器
    CContainerUI* m_pPage1;
    CContainerUI* m_pPage2;
    CContainerUI* m_pPage3;
    CContainerUI* m_pPage4;
    
    // 界面控件 - 第一页（快速安装）
    CCheckBoxUI* m_pAgreeCheck;
    CButtonUI* m_pInstallBtn;
    CButtonUI* m_pSwitchCustomBtn;
    
    // 界面控件 - 第二页（自定义安装）
    CCheckBoxUI* m_pAgreeCheckCustom;
    CCheckBoxUI* m_pDesktopCheck;
    CButtonUI* m_pCustomInstallBtn;
    CButtonUI* m_pBrowseBtn;
    CEditUI* m_pPathEdit;
    CButtonUI* m_pSwitchQuickBtn;
    
    // 界面控件 - 第三页（安装进度）
    CProgressUI* m_pProgress;
    CLabelUI* m_pProgressText;
    
    // 界面控件 - 第四页（安装完成）
    CButtonUI* m_pLaunchBtn;

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

    // 通用方法：处理关联控件的点击事件
    void HandleRelatedControlClick(CControlUI* pControl);

private:
    std::wstring m_strInstallPath;
    HANDLE m_hInstallThread;
    bool m_bInstalling;
    UINT64 m_totalBytes;
    UINT64 m_processedBytes;
};
