// UninstallerCaller.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include <stdlib.h>
#include "Utility.h"
#include "PathMgr.h"

LPCTSTR ExeFileName = _T("RealUninstall.exe");

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	CString errMsg;

	PathMgr pathMgr;
	CString appDir = pathMgr.GetAppRootDir();

	const char* tempDirEnvVar = getenv("temp");
	if (tempDirEnvVar == NULL) {
		DisplayError(_T("The environment variable 'temp' does not exist."));
		return 0;
	}

	const CString tempDir = CString(tempDirEnvVar) + _T("\\");
	CString srcPath = MakePath(appDir, UninstallLogFileName);
	CString destPath = tempDir + UninstallLogFileName;

	if (!CopyFile(srcPath, destPath, FALSE)) {
		DWORD err = GetLastError();
		errMsg.Format(_T("The following error occurred copying file '%s':\n\n%s"), UninstallLogFileName, GetErrorMessage(err));
		DisplayError(errMsg);
		return err;
	}

	srcPath = MakePath(appDir, ExeFileName);
	destPath = tempDir + ExeFileName;

	if (!CopyFile(srcPath, destPath, FALSE)) {
		DWORD err = GetLastError();
		errMsg.Format(_T("The following error occurred copying file '%s':\n\n%s"), ExeFileName, GetErrorMessage(err));
		DisplayError(errMsg);
		return err;
	}

	// must change current folder away from any being uninstalled because ShellExecute
	// executes from the current folder which keeps that folder in use and would prevent
	// it from being able to be deleted
	VERIFY(SetCurrentDirectory(tempDir));

	// must use ShellExecute instead of CreateProcess to be able to run the
	// uninstaller as administrator
	// actually, maybe that wouldn't be necessary if this exe required admin also
	int err = (int)ShellExecute(NULL, _T("open"), destPath, NULL, NULL, SW_SHOWNORMAL);
	if (err <= 32) {
		errMsg.Format(_T("The following error occurred try to run file '%s':\n\n%s"), destPath, (LPCTSTR)GetErrorMessage(err));
		DisplayError(errMsg);
		return err;
	}

#if 0
	DWORD flags = 0;
	STARTUPINFO startUpInfo;
	PROCESS_INFORMATION procInfo;
	MemClear(startUpInfo);
	startUpInfo.cb = sizeof(startUpInfo);

	BOOL succ = CreateProcess(NULL, Quote(path), NULL, NULL, FALSE, flags, NULL, tempDir, &startUpInfo, &procInfo);
	assert(succ);
#endif

	return 0;
}