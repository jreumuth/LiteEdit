# Microsoft Developer Studio Project File - Name="LiteEdit" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=LiteEdit - Win32 Unicode Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "LiteEdit.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "LiteEdit.mak" CFG="LiteEdit - Win32 Unicode Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "LiteEdit - Win32 Ascii Debug" (based on "Win32 (x86) Application")
!MESSAGE "LiteEdit - Win32 Ascii Release" (based on "Win32 (x86) Application")
!MESSAGE "LiteEdit - Win32 Unicode Debug" (based on "Win32 (x86) Application")
!MESSAGE "LiteEdit - Win32 Unicode Release" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "LiteEdit - Win32 Ascii Debug"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\Bin"
# PROP Intermediate_Dir "Ascii_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\Framework" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"Ascii_Debug/LiteEdit.bsc"
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 Framework.lib shlwapi.lib /nologo /subsystem:windows /map:"..\Bin\LiteEdit.map" /debug /machine:I386 /pdbtype:sept /libpath:"..\Framework\Ascii_Debug"

!ELSEIF  "$(CFG)" == "LiteEdit - Win32 Ascii Release"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\Bin"
# PROP Intermediate_Dir "Ascii_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\Framework" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"Ascii_Release/LiteEdit.bsc"
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 Framework.lib shlwapi.lib /nologo /subsystem:windows /profile /map:"..\Bin\LiteEdit.map" /machine:I386 /libpath:"..\Framework\Ascii_Release"
# SUBTRACT LINK32 /nodefaultlib

!ELSEIF  "$(CFG)" == "LiteEdit - Win32 Unicode Debug"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "LiteEdit___Win32_Unicode_Debug"
# PROP BASE Intermediate_Dir "LiteEdit___Win32_Unicode_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Unicode_Debug"
# PROP Intermediate_Dir "Unicode_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\Framework" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_UNICODE" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\Framework" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_UNICODE" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo /o"Debug/LiteEdit.bsc"
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 Framework.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept /libpath:"..\Framework\Debug"
# ADD LINK32 shlwapi.lib Framework.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /debug /machine:I386 /out:"..\Bin\LiteEdit.exe" /pdbtype:sept /libpath:"..\Framework\Unicode_Debug"

!ELSEIF  "$(CFG)" == "LiteEdit - Win32 Unicode Release"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "LiteEdit___Win32_Unicode_Release"
# PROP BASE Intermediate_Dir "LiteEdit___Win32_Unicode_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Unicode_Release"
# PROP Intermediate_Dir "Unicode_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /I "..\Framework" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_UNICODE" /FR /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\Framework" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_UNICODE" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 Framework.lib /nologo /subsystem:windows /profile /map:"Bin\LiteEdit.map" /machine:I386 /libpath:"..\Framework\Release"
# SUBTRACT BASE LINK32 /nodefaultlib
# ADD LINK32 shlwapi.lib Framework.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /profile /map:"..\Bin\LiteEdit.map" /machine:I386 /out:"..\Bin\LiteEdit.exe" /libpath:"..\Framework\Unicode_Release"
# SUBTRACT LINK32 /nodefaultlib

!ENDIF 

# Begin Target

# Name "LiteEdit - Win32 Ascii Debug"
# Name "LiteEdit - Win32 Ascii Release"
# Name "LiteEdit - Win32 Unicode Debug"
# Name "LiteEdit - Win32 Unicode Release"
# Begin Group "EditCtrl"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\CodeColoring.cpp
# End Source File
# Begin Source File

SOURCE=.\CodeColoring.h
# End Source File
# Begin Source File

SOURCE=.\EditCtrl.cpp
# End Source File
# Begin Source File

SOURCE=.\EditCtrl.h
# End Source File
# End Group
# Begin Group "Dialogs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\AboutDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\AboutDlg.h
# End Source File
# Begin Source File

SOURCE=.\ColorsPage.cpp
# End Source File
# Begin Source File

SOURCE=.\ColorsPage.h
# End Source File
# Begin Source File

SOURCE=.\ConfigData.cpp
# End Source File
# Begin Source File

SOURCE=.\ConfigData.h
# End Source File
# Begin Source File

SOURCE=.\EditorOptionsDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\EditorOptionsDlg.h
# End Source File
# Begin Source File

SOURCE=.\FindReplaceDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\FindReplaceDlg.h
# End Source File
# Begin Source File

SOURCE=.\GotoDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\GotoDlg.h
# End Source File
# Begin Source File

SOURCE=.\HotKeysDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\HotKeysDlg.h
# End Source File
# Begin Source File

SOURCE=.\LanguagesPage.cpp
# End Source File
# Begin Source File

SOURCE=.\LanguagesPage.h
# End Source File
# Begin Source File

SOURCE=.\LiteEditDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\LiteEditDlg.h
# End Source File
# Begin Source File

SOURCE=.\PromptDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\PromptDlg.h
# End Source File
# Begin Source File

SOURCE=.\SyntaxColoringDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\SyntaxColoringDlg.h
# End Source File
# Begin Source File

SOURCE=.\TimeDateFormatDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\TimeDateFormatDlg.h
# End Source File
# Begin Source File

SOURCE=.\ToolsDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\ToolsDlg.h
# End Source File
# End Group
# Begin Group "AppFramework"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\LiteEdit.cpp
# End Source File
# Begin Source File

SOURCE=.\LiteEdit.h
# End Source File
# Begin Source File

SOURCE=.\res\LiteEdit.ico
# End Source File
# Begin Source File

SOURCE=.\LiteEdit.rc
# End Source File
# Begin Source File

SOURCE=.\res\LiteEdit.rc2
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\res\macro.ico
# End Source File
# End Target
# End Project
# Section LiteEdit : {0050004F-0045-0052-5400-59005F005200}
# 	1:12:IDR_LITEEDIT:103
# End Section
