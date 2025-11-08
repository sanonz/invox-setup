#ifndef __LAYOUT_HELPER_H__
#define __LAYOUT_HELPER_H__

#pragma once

namespace DuiLib {

class CLayoutHelper
{
public:
    // 计算容器内所有子控件的总宽度
    static int GetChildrenTotalWidth(CControlUI* pControl)
    {
        if (!pControl) return 0;
        
        CContainerUI* pContainer = static_cast<CContainerUI*>(pControl->GetInterface(DUI_CTR_CONTAINER));
        if (!pContainer) return 0;

        int nTotalWidth = 0;
        for (int i = 0; i < pContainer->GetCount(); i++)
        {
            CControlUI* pChild = pContainer->GetItemAt(i);
            if (pChild && pChild->IsVisible())
            {
                // 累加子控件的宽度和padding
                nTotalWidth += pChild->GetFixedWidth();
                RECT rcPadding = pChild->GetPadding();
                nTotalWidth += rcPadding.left + rcPadding.right;
            }
        }
        return nTotalWidth;
    }

    // 计算容器内所有子控件的总宽度（包括自动计算宽度的控件）
    static int GetChildrenTotalWidthWithAuto(CControlUI* pControl)
    {
        if (!pControl) return 0;
        
        CContainerUI* pContainer = static_cast<CContainerUI*>(pControl->GetInterface(DUI_CTR_CONTAINER));
        if (!pContainer) return 0;

        int nTotalWidth = 0;
        for (int i = 0; i < pContainer->GetCount(); i++)
        {
            CControlUI* pChild = pContainer->GetItemAt(i);
            if (pChild && pChild->IsVisible())
            {
                // 获取实际宽度（包括自动计算的宽度）
                nTotalWidth += pChild->GetWidth();
                RECT rcPadding = pChild->GetPadding();
                nTotalWidth += rcPadding.left + rcPadding.right;
            }
        }
        return nTotalWidth;
    }
};

} // namespace DuiLib

#endif // __LAYOUT_HELPER_H__