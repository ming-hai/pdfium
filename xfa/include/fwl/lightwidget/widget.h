// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef _FWL_WIDGET_LIGHT_H
#define _FWL_WIDGET_LIGHT_H
class CFWL_WidgetMgr;
class CFWL_Message;
class CFWL_Event;
class IFWL_WidgetDelegate;
class IFWL_Widget;
class CFWL_Widget;
class CFWL_WidgetDelegate;
class CFWL_WidgetProperties
{
public:
    CFWL_WidgetProperties()
    {
        m_ctmOnParent.Reset();
        m_rtWidget.Set(0, 0, 0, 0);
        m_dwStyles		= FWL_WGTSTYLE_Child;
        m_dwStyleExes	= 0;
        m_dwStates		= 0;
        m_pParent		= NULL;
        m_pOwner		= NULL;
    }
    CFX_WideString		m_wsWindowclass;
    CFX_Matrix			m_ctmOnParent;
    CFX_RectF			m_rtWidget;
    FX_DWORD			m_dwStyles;
    FX_DWORD			m_dwStyleExes;
    FX_DWORD			m_dwStates;
    CFWL_Widget			*m_pParent;
    CFWL_Widget			*m_pOwner;
};
class CFWL_Widget
{
public:
    IFWL_Widget*	GetWidget();
    FX_DWORD		Release();
    CFWL_Widget*	Retain();
    FX_DWORD		GetRefCount() const;
    FWL_ERR			GetClassName(CFX_WideString &wsClass) const;
    FX_DWORD		GetClassID() const;
    virtual FX_BOOL	IsInstance(FX_WSTR wsClass) const;
protected:
    FWL_ERR			Initialize(const CFWL_WidgetProperties *pProperties = NULL);
public:
    FWL_ERR			GetWidgetRect(CFX_RectF &rect, FX_BOOL bAutoSize = FALSE);
    FWL_ERR			GetGlobalRect(CFX_RectF &rect);
    FWL_ERR			SetWidgetRect(const CFX_RectF &rect);
    FWL_ERR			GetClientRect(CFX_RectF &rtClient);
    CFWL_Widget*	GetParent();
    FWL_ERR			SetParent(CFWL_Widget *pParent);
    CFWL_Widget*	GetOwner();
    FWL_ERR			SetOwner(CFWL_Widget *pOwner);
    FX_DWORD		GetStyles();
    FWL_ERR			ModifyStyles(FX_DWORD dwStylesAdded, FX_DWORD dwStylesRemoved);
    FX_DWORD		GetStylesEx();
    FWL_ERR			ModifyStylesEx(FX_DWORD dwStylesExAdded, FX_DWORD dwStylesExRemoved);
    FX_DWORD		GetStates();
    FWL_ERR			SetStates(FX_DWORD dwStates, FX_BOOL bSet = TRUE);
    FWL_ERR			SetPrivateData(FX_LPVOID module_id, FX_LPVOID pData, PD_CALLBACK_FREEDATA callback);
    FX_LPVOID		GetPrivateData(FX_LPVOID module_id);
    FWL_ERR			Update();
    FWL_ERR			LockUpdate();
    FWL_ERR			UnlockUpdate();
    FX_DWORD		HitTest(FX_FLOAT fx, FX_FLOAT fy);
    FWL_ERR			TransformTo(CFWL_Widget *pWidget, FX_FLOAT &fx, FX_FLOAT &fy);
    FWL_ERR			TransformTo(CFWL_Widget *pWidget, CFX_RectF &rt);
    FWL_ERR			GetMatrix(CFX_Matrix &matrix, FX_BOOL bGlobal = FALSE);
    FWL_ERR			SetMatrix(const CFX_Matrix &matrix);
    FWL_ERR			DrawWidget(CFX_Graphics *pGraphics, const CFX_Matrix *pMatrix = NULL);
    FWL_ERR			GetProperties(CFWL_WidgetProperties &properties);
    FWL_ERR			SetProperties(const CFWL_WidgetProperties &properties);
    IFWL_WidgetDelegate* SetDelegate(IFWL_WidgetDelegate* pDelegate);
    FWL_ERR			Repaint(const CFX_RectF *pRect = NULL);
    FWL_ERR			SetFocus(FX_BOOL bFocus);
    FWL_ERR			SetGrab(FX_BOOL bSet);
    CFWL_Widget();
    virtual ~CFWL_Widget();
    void		RegisterEventTarget(CFWL_Widget *pEventSource = NULL, FX_DWORD dwFilter = FWL_EVENT_ALL_MASK);
    void		DispatchEvent(CFWL_Event *pEvent);
    CFX_SizeF	CalcTextSize(const CFX_WideString &wsText, FX_BOOL bMultiLine = FALSE, FX_INT32 iLineWidth = -1);
    IFWL_Widget			*m_pImp;
    IFWL_WidgetDelegate *m_pDelegate;
    CFWL_WidgetMgr		*m_pWidgetMgr;
    CFWL_WidgetProperties *m_pProperties;
};
class CFWL_WidgetDelegate
{
public:
    CFWL_WidgetDelegate();
    virtual ~CFWL_WidgetDelegate();
    virtual FX_INT32	OnProcessMessage(CFWL_Message *pMessage);
    virtual FWL_ERR		OnProcessEvent(CFWL_Event *pEvent);
    virtual FWL_ERR		OnDrawWidget(CFX_Graphics *pGraphics, const CFX_Matrix *pMatrix = NULL);
};
#endif
