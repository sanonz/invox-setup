#pragma once
#include <windows.h>
#include <string>
#include "7zTypes.h"

// 7z 解压辅助类
class C7zExtractor
{
public:
    C7zExtractor();
    ~C7zExtractor();

    // 设置回调函数用于更新进度
    typedef void (*ProgressCallback)(UINT64 bytesProcessed, UINT64 totalBytes, void* userData);
    void SetProgressCallback(ProgressCallback callback, void* userData);

    // 解压文件
    // archivePath: 7z 压缩包路径
    // destPath: 目标解压路径
    // 返回: 成功返回 true，失败返回 false
    bool Extract(const std::wstring& archivePath, const std::wstring& destPath);

    // 获取压缩包总大小（用于进度计算）
    UINT64 GetArchiveSize(const std::wstring& archivePath);

private:
    ProgressCallback m_progressCallback;
    void* m_pUserData;
    
    // 7zxa.dll 动态库句柄
    HMODULE m_h7zDll;
    
    // CreateObject 函数指针
    CreateObjectFunc m_pfnCreateObject;
    
    // 初始化 7zxa.dll
    bool Init7zDll();
    
    // 释放 7zxa.dll
    void Free7zDll();
    
    // 使用 7zxa.dll 解压
    bool ExtractWith7zDll(const std::wstring& archivePath, const std::wstring& destPath);
};
