#include "StdAfx.h"
#include "InstallerWnd.h"
#include "Config.h"

void InitResource()
{
    // 资源类型
#ifdef _DEBUG
    CPaintManagerUI::SetResourceType(UILIB_FILE);
#else
    CPaintManagerUI::SetResourceType(UILIB_ZIPRESOURCE);
#endif
    // 加载资源
    switch (CPaintManagerUI::GetResourceType())
    {
    case UILIB_FILE:
    {
        CDuiString strResourcePath = CPaintManagerUI::GetInstancePath();
        strResourcePath = strResourcePath.Left(strResourcePath.GetLength() - 4);
        strResourcePath += _T("Installer\\Res\\");
        CPaintManagerUI::SetResourcePath(strResourcePath.GetData());
        break;
    }
    case UILIB_RESOURCE:
    {
        CDuiString strResourcePath = CPaintManagerUI::GetInstancePath();
        strResourcePath = strResourcePath.Left(strResourcePath.GetLength() - 4);
        strResourcePath += _T("Installer\\Res\\");
        CPaintManagerUI::SetResourcePath(strResourcePath.GetData());
        break;
    }
    case UILIB_ZIP:
    {
        CDuiString strResourcePath = CPaintManagerUI::GetInstancePath();
        strResourcePath = strResourcePath.Left(strResourcePath.GetLength() - 4);
        strResourcePath += _T("Installer\\Res\\");
        CPaintManagerUI::SetResourcePath(strResourcePath.GetData());
        CPaintManagerUI::SetResourceZip(_T("resources.zip"), true);
        break;
    }
    case UILIB_ZIPRESOURCE:
    {
        HRSRC hResource = ::FindResource(CPaintManagerUI::GetResourceDll(), _T("IDR_ZIPRES"), _T("ZIPRES"));
        if (hResource != NULL)
        {
            DWORD dwSize = 0;
            HGLOBAL hGlobal = ::LoadResource(CPaintManagerUI::GetResourceDll(), hResource);
            if (hGlobal != NULL)
            {
                dwSize = ::SizeofResource(CPaintManagerUI::GetResourceDll(), hResource);
                if (dwSize > 0)
                {
                    CPaintManagerUI::SetResourceZip((LPBYTE)::LockResource(hGlobal), dwSize, _T("323232"));
                }
            }
            ::FreeResource(hGlobal);
        }
    }
    break;
    }
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int nCmdShow)
{
    HRESULT Hr = ::CoInitialize(NULL);
    if (FAILED(Hr))
        return 0;

    CPaintManagerUI::SetInstance(hInstance);

    InitResource();

    CInstallerWnd *pFrame = new CInstallerWnd();
    if (pFrame == NULL)
        return 0;

    pFrame->Create(NULL, _T("安装程序"), UI_WNDSTYLE_DIALOG, 0L, 0, 0, 600, 450);
    pFrame->CenterWindow();
    pFrame->ShowModal();

    ::CoUninitialize();
    return 0;
}
