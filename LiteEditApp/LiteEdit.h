// LiteEdit.h : main header file for the LITEEDIT application
//

#if !defined(AFX_LITEEDIT_H__51143EC9_3D15_4B2D_AB4B_04B190DE8EA8__INCLUDED_)
#define AFX_LITEEDIT_H__51143EC9_3D15_4B2D_AB4B_04B190DE8EA8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CLiteEditApp:
// See LiteEdit.cpp for the implementation of this class
//

class CLiteEditApp : public CWinApp
{
public:
	CLiteEditApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLiteEditApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CLiteEditApp)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	BOOL m_bATLInited;
private:
	BOOL InitATL();
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LITEEDIT_H__51143EC9_3D15_4B2D_AB4B_04B190DE8EA8__INCLUDED_)
