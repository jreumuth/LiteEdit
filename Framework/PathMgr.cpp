#include "StdAfx.h"
#include "PathMgr.h"
#include "ShlObj.h"

const CString RunInstalledMarkerFileName = _T("RunUninstalled.txt");

// constants for SHGetSpecialFolderPath
const int CSIDL_PROGRAM_FILESX86	= 0x002a;
const int CSIDL_LOCAL_APPDATA		= 0x001c;
const int CSIDL_COMMON_APPDATA		= 0x0023;
const int CSIDL_COMMON_DOCUMENTS	= 0x002e;

enum FolderUsed {fuNone, fuCsidl, fuEnvVar, fuDefault};

//It looks like there's no good way to get the system folders for
//all systems. Many constants for SHGetSpecialFolderPath are ony defined
//for certain version of the OS and/or IE. Some macros, such as
//the ProgramFiles macro, is not defined on all windows systems.
CString GetSystemFolder(int csidl, LPCSTR envVarName, LPCTSTR defaultVal, FolderUsed* folderUsed = NULL) {
	CString path;
	FolderUsed _folderUsed = fuNone;

	if (GetSpecialFolderPath(csidl, path)) {
		_folderUsed = fuCsidl;
	} else {
		const char* envVarPath = getenv(envVarName);

		if (envVarPath != NULL) {
			path = envVarPath;
			_folderUsed = fuEnvVar;
		} else {
			path = defaultVal;
			_folderUsed = fuDefault;
		}
	}

	if (folderUsed != NULL)
		*folderUsed = _folderUsed;

	return path;
}

CString GetProgramFilesDir() {
	return GetSystemFolder(CSIDL_PROGRAM_FILESX86, "ProgramFiles", TEXT("C:\\Program Files"));
}

CString GetLocalAppDataDir() {
	FolderUsed folderUsed;
	CString result = GetSystemFolder(CSIDL_LOCAL_APPDATA, "UserProfile", PathMgr().GetAppRootDir(), &folderUsed);

	if (folderUsed == fuEnvVar)
		result = MakePath(result, _T("Local Settings\\Application Data"));

	if (folderUsed == fuCsidl || folderUsed == fuEnvVar)
		result = MakePath(result, LiteEditDirName);

	return result;
}

CString GetCommonAppDataDir() {
	FolderUsed folderUsed;
	CString result = GetSystemFolder(CSIDL_COMMON_APPDATA, "AllUsersProfile", PathMgr().GetAppRootDir(), &folderUsed);

	if (folderUsed == fuEnvVar)
		result = MakePath(result, _T("Application Data"));

	if (folderUsed == fuCsidl || folderUsed == fuEnvVar)
		result = MakePath(result, LiteEditDirName);

	return result;
}

CString GetPublicDocumentsDir() {
	FolderUsed folderUsed;
	CString result = GetSystemFolder(CSIDL_COMMON_DOCUMENTS, "AllUsersProfile", PathMgr().GetAppRootDir(), &folderUsed);

	if (folderUsed == fuEnvVar)
		result = MakePath(result, _T("Documents"));

	if (folderUsed == fuCsidl || folderUsed == fuEnvVar)
		result = MakePath(result, LiteEditDirName);

	return result;
}

CString GetAppPath() {
	CString modulePath;
	const int BuffSize = BIG_MAX_PATH;
	LPTSTR buff = modulePath.GetBuffer(BuffSize);
	// GetModuleFileName just returns the size of the buffer, instead of
	// the size needed, when the buffer is too small, so I can't dynamically
	// allocate a string of the correct size.
	DWORD len = GetModuleFileName(NULL, buff, BuffSize);
	ASSERT(len > 0);
	modulePath.ReleaseBuffer(len);
	return modulePath;
}

PathMgr::PathData PathMgr::mPathData;

PathMgr::PathData::PathData() {
	//mAppPath and mAppRootDir must be first because other methods called from here
	//reference them
	mAppPath = ::GetAppPath();
	mAppRootDir = GetDir(mAppPath);

	mProgramFilesDir = ::GetProgramFilesDir();
	mAllUsersData = ::GetPublicDocumentsDir();
	mPerUserData = ::GetLocalAppDataDir();
	mTempDir = getenv("Temp");

	mRunUninstalledLanguageDir = MakePath(mAppRootDir, LangDirName);
	if (!FileExists(mRunUninstalledLanguageDir)) // support old structure where the language were with the program; possibly necessary for Lite Edit OEM
		mRunUninstalledLanguageDir = mAppRootDir;

	mForceRunUninstalled = FileExists(MakePath(mAppRootDir, RunInstalledMarkerFileName));
}

CString PathMgr::GetActualConfigFilePath() const {
	CString runInstalledConfigFilePath = GetRunUninstalledConfigFilePath();
	CString installedConfigFilePath = GetInstalledConfigFilePath();

	// Always use the run uninstalled config file when mForceRunUninstalled is true. Otherwise...
	// Use the config file from the installed directory unless it doesn't exist and the
	// run uninstalled config file does, or if the config file install dir can't be created.
	// Note that the InstalledConfigFileDir must be created on demand instead of during
	// installation since we can't assume the current user is the one that installed
	// Lite Edit, espeically because of needing admin to install on Vista/7.
	if (GetForceRunUninstalled())
		return runInstalledConfigFilePath;
	else if (!FileExists(installedConfigFilePath) && FileExists(runInstalledConfigFilePath))
		return runInstalledConfigFilePath;
	else {
		CString installedConfigFileDir = GetInstalledConfigFileDir();
		if (!FileExists(installedConfigFileDir)) {
			// This file could be added to the uninstall log when created successfully
			// to avoid leaving the file around after uninstall, though I didn't go
			// through the trouble.
			if (!CreateDirPath(installedConfigFileDir))
				return runInstalledConfigFilePath;
		}

		return installedConfigFilePath;
	}
}

CString PathMgr::GetHelpFilePath(HelpFile helpFile) const {
	CString fileName;

	switch (helpFile) {
	case hfContents:
		fileName = _T("Contents.html");
		break;
	case hfSyntaxColoring:
		fileName = _T("SyntaxColoring.html");
		break;
	case hfTools:
		fileName = _T("Tools.html");
		break;
	case hfMisc:
		fileName = _T("Misc.html");
		break;
	default:
		ASSERT(false);
	}

	return MakePath(GetHelpFileDir(), fileName);
}

CString PathMgr::GetNewLanguageFileDir() const {
	if (GetForceRunUninstalled())
		return GetRunUninstalledLanguageDir();

	CString installedLanguageDir = GetInstalledLanguageDir();

	if (FileExists(installedLanguageDir))
		return installedLanguageDir;
	else
		return GetRunUninstalledLanguageDir();
}
