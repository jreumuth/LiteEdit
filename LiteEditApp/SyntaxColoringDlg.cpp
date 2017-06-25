#include "stdafx.h"
#include "liteedit.h"
#include "SyntaxColoringDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BEGIN_MESSAGE_MAP(SyntaxColoringDlg, Base)
	//{{AFX_MSG_MAP(ColorsPage)
	ON_MESSAGE(WM_HELP, OnHelp) // to capture F1, even when no sheet is active
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()
