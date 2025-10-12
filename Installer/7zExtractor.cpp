#include "StdAfx.h"
#include "7zExtractor.h"
#include "7zStreamHelpers.h"
#include "InstallHelper.h"
#include "resource.h"
#include <shlwapi.h>
#include <ShlObj.h>

#pragma comment(lib, "shlwapi.lib")

C7zExtractor::C7zExtractor()
    : m_progressCallback(NULL)
    , m_pUserData(NULL)
    , m_h7zDll(NULL)
    , m_pfnCreateObject(NULL)
{
    // 初始化 COM
    CoInitialize(NULL);
    
    // 初始化 7z.dll
    Init7zDll();
}

C7zExtractor::~C7zExtractor()
{
    Free7zDll();
    CoUninitialize();
}

void C7zExtractor::SetProgressCallback(ProgressCallback callback, void* userData)
{
    m_progressCallback = callback;
    m_pUserData = userData;
}

bool C7zExtractor::Init7zDll()
{
    // 获取临时目录
    WCHAR szTempPath[MAX_PATH] = { 0 };
    GetTempPath(MAX_PATH, szTempPath);

#if defined(_DEBUG)
    // 获取当前程序所在目录
    GetModuleFileName(NULL, szTempPath, MAX_PATH);
    PathRemoveFileSpec(szTempPath);
    
    // 构建 7z.dll 路径
    std::wstring str7zDllPath = szTempPath;
    str7zDllPath = str7zDllPath.substr(0, str7zDllPath.length() - 3);
    str7zDllPath += L"3rd\\7z.dll";
#else
    // 构建 7z.dll 临时路径
    std::wstring str7zDllPath = szTempPath;
    str7zDllPath += L"Installer_Temp\\7z.dll";
    
    // 从资源提取 7z.dll（使用 CInstallHelper 统一的提取函数）
    HINSTANCE hInstance = GetModuleHandle(NULL);
    if (!CInstallHelper::ExtractBinaryResource(hInstance, IDR_7Z_DLL, str7zDllPath))
    {
        return false;
    }
#endif    // _DEBUG
    
    // 加载 7z.dll
    m_h7zDll = LoadLibrary(str7zDllPath.c_str());
    if (!m_h7zDll)
    {
        return false;
    }
    
    // 获取 CreateObject 函数指针
    m_pfnCreateObject = (CreateObjectFunc)GetProcAddress(m_h7zDll, "CreateObject");
    if (!m_pfnCreateObject)
    {
        Free7zDll();
        return false;
    }
    
    return true;
}

void C7zExtractor::Free7zDll()
{
    if (m_h7zDll)
    {
        FreeLibrary(m_h7zDll);
        m_h7zDll = NULL;
    }
    
    m_pfnCreateObject = NULL;
}

UINT64 C7zExtractor::GetArchiveSize(const std::wstring& archivePath)
{
    WIN32_FILE_ATTRIBUTE_DATA fileInfo;
    if (GetFileAttributesEx(archivePath.c_str(), GetFileExInfoStandard, &fileInfo))
    {
        LARGE_INTEGER size;
        size.HighPart = fileInfo.nFileSizeHigh;
        size.LowPart = fileInfo.nFileSizeLow;
        return size.QuadPart;
    }
    return 0;
}

bool C7zExtractor::ExtractWith7zDll(const std::wstring& archivePath, const std::wstring& destPath)
{
    if (!m_h7zDll || !m_pfnCreateObject)
    {
        return false;
    }
    
    // 创建归档处理器
    IInArchive* archive = NULL;
    HRESULT hr = m_pfnCreateObject(&CLSID_CFormat7z, &IID_IInArchive, (void**)&archive);
    if (FAILED(hr) || !archive)
    {
        return false;
    }
    
    // 创建输入流
    CInFileStream* fileStream = new CInFileStream();
    if (!fileStream->Open(archivePath.c_str()))
    {
        fileStream->Release();
        archive->Release();
        return false;
    }
    
    // 创建打开回调
    CArchiveOpenCallback* openCallback = new CArchiveOpenCallback();
    
    // 打开归档
    hr = archive->Open(fileStream, NULL, openCallback);
    openCallback->Release();
    
    if (FAILED(hr))
    {
        fileStream->Release();
        archive->Release();
        return false;
    }
    
    // 获取文件数量
    UInt32 numItems = 0;
    archive->GetNumberOfItems(&numItems);
    
    // 创建目标目录
    SHCreateDirectoryExW(NULL, destPath.c_str(), NULL);
    
    // 创建解压回调
    CArchiveExtractCallback* extractCallback = new CArchiveExtractCallback();
    extractCallback->Init(archive, destPath);
    extractCallback->SetProgressCallback(m_progressCallback, m_pUserData);
    
    // 解压所有文件
    hr = archive->Extract(NULL, (UInt32)-1, 0, extractCallback);
    
    // 清理
    extractCallback->Release();
    fileStream->Release();
    archive->Close();
    archive->Release();
    
    return SUCCEEDED(hr);
}

bool C7zExtractor::Extract(const std::wstring& archivePath, const std::wstring& destPath)
{
    // 检查压缩包是否存在
    if (!PathFileExists(archivePath.c_str()))
    {
        return false;
    }
    
    return ExtractWith7zDll(archivePath, destPath);
}
