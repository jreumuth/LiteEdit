#if !defined(AFX_SYNTAXCOLORINGDLG_H__D170DC5A_3AD4_40D2_9D15_32888156D2FB__INCLUDED_)
#define AFX_SYNTAXCOLORINGDLG_H__D170DC5A_3AD4_40D2_9D15_32888156D2FB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "LanguagesPage.h"
#include "ColorsPage.h"

class SyntaxColoringDlg : public CPropertySheet {
private:
	typedef CPropertySheet Base;

	LanguagesPage mLangPage;
	ColorsPage mColorsPage;

protected:
	DECLARE_MESSAGE_MAP()

	// to capture F1, even when no sheet is active
	LRESULT OnHelp(WPARAM wParam, LPARAM lParam) {
		gConfigData.ExecuteHelp(hfSyntaxColoring, m_hWnd);
		return 0;
	}

public:
	SyntaxColoringDlg(const CString& initLang) :
		CPropertySheet(TEXT("Syntax Coloring")),
		mLangPage(initLang)
	{
		AddPage(&mLangPage);
		AddPage(&mColorsPage);
	}
};

#endif // !defined(AFX_SYNTAXCOLORINGDLG_H__D170DC5A_3AD4_40D2_9D15_32888156D2FB__INCLUDED_)
