#ifndef PathMgrH
#define PathMgrH

#include "Utility.h"

// note that these value are duplicated in the version resource of every project
const Version ProgramVersion(2, 1, 1);

const CString ProgramName = TEXT("Lite Edit");
const CString ProgramLongName = ProgramName + TEXT(" ") + ProgramVersion.AsString();
const CString RegKeyName = ProgramName;
const CString Publisher = TEXT("Jimmy Reumuth");
const CString PublisherEMail = TEXT("reumuth@rocketmail.com");
const CString AppUrl = TEXT("http://www.reumuth.com/programming");

const CString UninstallLogFileName = TEXT("Uninstall.log");
const CString InstallRegPrefix = TEXT(":Reg:");
const CString LiteEditDirName = TEXT("Lite Edit");
const CString ConfigFileName = TEXT("LiteEdit.ini");
const CString HelpFileDirName = TEXT("Help");
const CString LangDirName = TEXT("Languages");
const CString LangExt = TEXT(".lang");

enum HelpFile {
	hfContents,
	hfSyntaxColoring,
	hfTools,
	hfMisc
};

class PathMgr {
private:
	// this is separate and static so I can cheaply create PathMgr as a local
	struct PathData {
		CString mAppPath;
		CString mAppRootDir;

		CString mProgramFilesDir;
		CString mAllUsersData;
		CString mPerUserData;
		CString mTempDir;
		CString mRunUninstalledLanguageDir;

		bool	mForceRunUninstalled;

		PathData();
	};

	static PathData mPathData;

	CString GetStdIOTempFilePathImpl(LPCTSTR baseName) const {
		// include the process ID in case two Lite Edits are both running
		// filters at the same time
		CString fileName;
		fileName.Format(_T("LiteEditStd%s%d.txt"), baseName, GetCurrentProcessId());

		// create in temp folder since it's writeable on Vista/7 and guaranteed to exist
		return MakePath(mPathData.mTempDir, fileName);
	}

public:
	// the App dir and path are always the current .exe, which could be Lite Edit,
	// the installer, or the uninstaller
	CString GetAppPath() const {return mPathData.mAppPath;}
	CString GetAppRootDir() const {return mPathData.mAppRootDir;}

	CString GetProgramFilesDir() const {return mPathData.mProgramFilesDir;}

	CString GetAllUsersDataDir() const {return mPathData.mAllUsersData;}
	CString GetPerUserDataDir() const {return mPathData.mPerUserData;}

	bool GetForceRunUninstalled() const {return mPathData.mForceRunUninstalled;}

	// The RunUninstalled method is to support running Lite Edit when configuration files
	// are with the application instead of their installed folders.
	// The RunUninstalled folders are also the folders that were used before Lite Edit
	// 2.0.1 and Vista/7 support.
	// RunUninstalled folders are used for several things:
	// 1. Putting Lite Edit on a portable drive so you can use it on any computer without
	//    installing it to that computer.
	// 2. Installing Lite Edit as part of some other application, especially if someone
	//    originally did this before version 2.0.1 and is installing over an older version.
	// Note that the InstalledConfigFilePath is actually created on start up rather than
	// installed (see GetActualConfigFilePath).
	CString GetInstalledConfigFileDir() const {return mPathData.mPerUserData;}
	CString GetInstalledConfigFilePath() const {return MakePath(GetInstalledConfigFileDir(), ConfigFileName);}
	CString GetRunUninstalledConfigFilePath() const {return MakePath(mPathData.mAppRootDir, ConfigFileName);}
	CString GetActualConfigFilePath() const;

	CString GetHelpFileDir() const {return MakePath(mPathData.mAppRootDir, HelpFileDirName);}
	CString GetHelpFilePath(HelpFile helpFile) const;
	CString GetInternetExplorerPath() const {return MakePath(mPathData.mProgramFilesDir, TEXT("Internet Explorer\\iexplore.exe"));}

	// Update 'Porting Syntax Coloring Configurations' in the help file if the
	// language folder changes.
	CString GetInstalledLanguageDir() const {return MakePath(mPathData.mAllUsersData, LangDirName);}
	CString GetInstalledLanguagePath(const CString& languageName) {return MakePath(GetInstalledLanguageDir(), languageName + LangExt);}
	// see GetRunUninstalledConfigFilePath for comments
	CString GetRunUninstalledLanguageDir() const {return mPathData.mRunUninstalledLanguageDir;}
	CString GetRunUninstalledLanguagePath(const CString& languageName) {return MakePath(GetRunUninstalledLanguageDir(), languageName + LangExt);}
	CString GetNewLanguageFileDir() const;

	CString GetStdInTempFilePath() const {return GetStdIOTempFilePathImpl(TEXT("In"));}
	CString GetStdOutTempFilePath() const {return GetStdIOTempFilePathImpl(TEXT("Out"));}
	CString GetStdErrTempFilePath() const {return GetStdIOTempFilePathImpl(TEXT("Err"));}
};

#endif