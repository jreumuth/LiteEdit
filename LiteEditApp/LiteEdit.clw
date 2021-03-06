; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=ToolsDlg
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "liteedit.h"
LastPage=0

ClassCount=14
Class1=CGotoDlg
Class2=CLiteEditApp
Class3=CLiteEditDlg

ResourceCount=12
Resource1=IDD_TOOLS_DLG
Resource2=IDD_ABOUT_DLG
Resource3=IDD_FINDREPLACE_DLG
Resource4=IDD_HOTKEYS_DLG
Resource5=IDD_LANGUAGES_PAGE
Class4=SyntaxColoringDlg
Class5=LanguagesPage
Class6=ColorsPage
Class7=EditorOptions
Resource6=IDD_EDITOR_OPTIONS_DLG
Class8=EditorOptionsDlg
Resource7=IDD_PROMPT_DLG
Class9=HotKeysDlg
Resource8=IDD_GOTO_DLG
Class10=AboutDlg
Resource9=IDD_TIMEDATE_FORMAT_DLG
Class11=ToolsDlg
Resource10=IDD_COLORS_PAGE
Class12=FindReplaceDlg
Class13=PromptDlg
Resource11=IDD_MAIN_DLG
Class14=TimeDateFormatDlg
Resource12=IDR_MENU

[CLS:CGotoDlg]
Type=0
BaseClass=CDialog
HeaderFile=GotoDlg.h
ImplementationFile=GotoDlg.cpp
LastObject=CGotoDlg
Filter=D
VirtualFilter=dWC

[CLS:CLiteEditApp]
Type=0
BaseClass=CWinApp
HeaderFile=LiteEdit.h
ImplementationFile=LiteEdit.cpp

[CLS:CLiteEditDlg]
Type=0
BaseClass=CDialog
HeaderFile=LiteEditDlg.h
ImplementationFile=LiteEditDlg.cpp
Filter=D
VirtualFilter=dWC
LastObject=CLiteEditDlg

[DLG:IDD_GOTO_DLG]
Type=1
Class=CGotoDlg
ControlCount=4
Control1=IDC_STATIC_LINE_NUMBER,static,1342308352
Control2=IDC_LINE_EDIT,edit,1350639744
Control3=IDOK,button,1342242817
Control4=IDCANCEL,button,1342242816

[DLG:IDD_MAIN_DLG]
Type=1
Class=CLiteEditDlg
ControlCount=0

[MNU:IDR_MENU]
Type=1
Class=?
Command1=ID_FILE_NEW
Command2=ID_FILE_OPEN
Command3=ID_FILE_SAVE
Command4=ID_FILE_SAVEAS
Command5=ID_FILE_PRINT
Command6=ID_FILE_RECENT_DUMMY
Command7=ID_FILE_EXIT
Command8=ID_EDIT_UNDO
Command9=ID_EDIT_REDO
Command10=ID_EDIT_CUT
Command11=ID_EDIT_COPY
Command12=ID_EDIT_PASTE
Command13=ID_EDIT_DELETE
Command14=ID_EDIT_SELECTALL
Command15=ID_EDIT_ADVANCED_LASTCLIPBOARD
Command16=ID_EDIT_ADVANCED_SWITCHCLIPBOARD
Command17=ID_EDIT_ADVANCED_ENCODING_ASCII
Command18=ID_EDIT_ADVANCED_ENCODING_UTF16
Command19=ID_EDIT_ADVANCED_ENCODING_UTF8
Command20=ID_EDIT_ADVANCED_ENCODING_UNKNOWN
Command21=ID_EDIT_ADVANCED_NEW_LINE_TYPE_CRLF
Command22=ID_EDIT_ADVANCED_NEW_LINE_TYPE_LF
Command23=ID_EDIT_ADVANCED_NEW_LINE_TYPE_CR
Command24=ID_EDIT_ADVANCED_INSERTDATETIME
Command25=ID_EDIT_ADVANCED_EDIT_TIMEDATE_FORMAT
Command26=ID_EDIT_ADVANCED_LOWERCASE
Command27=ID_EDIT_ADVANCED_UPPERCASE
Command28=ID_EDIT_ADVANCED_TITLECASE
Command29=ID_EDIT_ADVANCED_TOGGLECASE
Command30=ID_SEARCH_FIND
Command31=ID_SEARCH_FINDNEXT
Command32=ID_SEARCH_FINDPREVIOUS
Command33=ID_SEARCH_REPLACE
Command34=ID_SEARCH_GOTO
Command35=ID_BOOKMARKS_TOGGLE
Command36=ID_BOOKMARKS_NEXT
Command37=ID_BOOKMARKS_PREVIOUS
Command38=ID_BOOKMARKS_CLEARALL
Command39=ID_TOOLS_EDIT
Command40=ID_OPTIONS_FONT
Command41=ID_OPTIONS_EDITOR
Command42=ID_OPTIONS_HOTKEYS
Command43=ID_OPTIONS_SYNTAXCOLORING
Command44=ID_OPTIONS_WORDWRAP
Command45=ID_HELP_CONTENTS
Command46=ID_HELP_ABOUT
CommandCount=46

[DLG:IDD_LANGUAGES_PAGE]
Type=1
Class=LanguagesPage
ControlCount=31
Control1=IDC_STATIC_LANG,button,1342177287
Control2=IDC_LIST_LANG,listbox,1352728833
Control3=IDC_BUTTON_ADD_LANG,button,1342242816
Control4=IDC_BUTTON_REMOVE_LANG,button,1342242816
Control5=IDC_STATIC_LANG_OPTIONS,button,1342177287
Control6=IDC_STATIC_LANG_NAME,static,1342308352
Control7=IDC_EDIT_LANG_NAME,edit,1350631552
Control8=IDC_CHECK_CASE_SENSITIVE,button,1342242819
Control9=IDC_STATIC_FILE_EXTENSIONS,static,1342308352
Control10=IDC_EDIT_EXTENSIONS,edit,1350631552
Control11=IDC_STATIC_ELEM,button,1342177287
Control12=IDC_LIST_ELEM,listbox,1352728833
Control13=IDC_BUTTON_ADD_ELEM,button,1342242816
Control14=IDC_BUTTON_REMOVE_ELEM,button,1342242816
Control15=IDC_BUTTON_UP,button,1342242816
Control16=IDC_BUTTON_DOWN,button,1342242816
Control17=IDC_STATIC_KIND,static,1342308352
Control18=IDC_COMBO_KIND,combobox,1344339971
Control19=IDC_STATIC_ELEM_DEF,static,1342308352
Control20=IDC_COMBO_ELEM_DEF,combobox,1344339971
Control21=IDC_CHECK_WHOLE_WORD,button,1342242819
Control22=IDC_STATIC_START,static,1342308352
Control23=IDC_EDIT_START,edit,1350631552
Control24=IDC_STATIC_END,static,1342308352
Control25=IDC_EDIT_END,edit,1350631552
Control26=IDC_STATIC_ESCAPE_CHAR,static,1342308352
Control27=IDC_EDIT_ESCAPE_CHAR,edit,1350631552
Control28=IDC_CHECK_SPAN_LINES,button,1342242819
Control29=IDC_CHECK_ALLOW_NESTING,button,1342242819
Control30=IDC_STATIC_WORD_LIST,static,1342308352
Control31=IDC_EDIT_WORD_LIST,edit,1352732868

[DLG:IDD_COLORS_PAGE]
Type=1
Class=ColorsPage
ControlCount=18
Control1=IDC_STATIC,button,1342177287
Control2=IDC_LIST_SCHEMAS,listbox,1352728833
Control3=IDC_BUTTON_NEW,button,1342242816
Control4=IDC_BUTTON_DELETE,button,1342242816
Control5=IDC_BUTTON_MOVE_UP,button,1342242816
Control6=IDC_BUTTON_MOVE_DOWN,button,1342242816
Control7=IDC_STATIC_SCHEME_NAME,static,1342308352
Control8=IDC_EDIT_NAME,edit,1350631552
Control9=IDC_STATIC_ITEM,static,1342308352
Control10=IDC_COMBO_ITEMS,combobox,1344339971
Control11=IDC_STATIC_CURRENT_COLOR,static,1342308352
Control12=IDC_STATIC_COLOR,static,1342308352
Control13=IDC_BUTTON_PICK_COLOR,button,1342242816
Control14=IDC_STATIC_SELECTION_COLORS,button,1342177287
Control15=IDC_RADIO_USE_SYSTEM_SELECTION_COLORS,button,1342308361
Control16=IDC_RADIO_INVERT_COLORS,button,1342186505
Control17=IDC_STATIC_PREVIEW,button,1342308359
Control18=IDC_EDIT_PREVIEW,edit,1345388740

[CLS:SyntaxColoringDlg]
Type=0
HeaderFile=SyntaxColoringDlg.h
ImplementationFile=SyntaxColoringDlg.cpp
BaseClass=CDialog
Filter=D
LastObject=IDC_TAB
VirtualFilter=dWC

[CLS:LanguagesPage]
Type=0
HeaderFile=LanguagesPage.h
ImplementationFile=LanguagesPage.cpp
BaseClass=CDialog
Filter=D
LastObject=IDC_STATIC_ELEM
VirtualFilter=dWC

[CLS:ColorsPage]
Type=0
HeaderFile=colorspage.h
ImplementationFile=colorspage.cpp
BaseClass=CDialog
LastObject=ColorsPage
Filter=W
VirtualFilter=dWC

[CLS:EditorOptions]
Type=0
HeaderFile=EditorOptions.h
ImplementationFile=EditorOptions.cpp
BaseClass=CDialog
Filter=D
LastObject=IDCANCEL
VirtualFilter=dWC

[CLS:EditorOptionsDlg]
Type=0
HeaderFile=EditorOptionsDlg.h
ImplementationFile=EditorOptionsDlg.cpp
BaseClass=CDialog
Filter=D
LastObject=IDC_COMBO_NEW_LINE_TYPE
VirtualFilter=dWC

[DLG:IDD_EDITOR_OPTIONS_DLG]
Type=1
Class=EditorOptionsDlg
ControlCount=19
Control1=IDC_STATIC,button,1342177287
Control2=IDC_CHECK_REMOVE_WHIETSPACE,button,1342242819
Control3=IDC_STATIC,static,1342308352
Control4=IDC_EDIT_DEFAULT_EXT,edit,1350631552
Control5=IDC_STATIC,static,1342308352
Control6=IDC_EDIT_MAX_RECENT,edit,1350639744
Control7=IDC_STATIC,static,1342308352
Control8=IDC_COMBO_ENCODING,combobox,1344339971
Control9=IDC_STATIC,static,1342308352
Control10=IDC_COMBO_NEW_LINE_TYPE,combobox,1344339971
Control11=IDC_STATIC,button,1342308359
Control12=IDC_CHECK_SHOW_LINE_NUMBERS,button,1342242819
Control13=IDC_CHECK_SHOW_WHITESPACE,button,1342242819
Control14=IDC_CHECK_AUTOINDENT,button,1342242819
Control15=IDC_CHECK_TAB_INSERTS_SPACES,button,1342242819
Control16=IDC_STATIC,static,1342308352
Control17=IDC_EDIT_TAB_WIDTH,edit,1350639744
Control18=IDOK,button,1342242817
Control19=IDCANCEL,button,1342242816

[DLG:IDD_HOTKEYS_DLG]
Type=1
Class=HotKeysDlg
ControlCount=6
Control1=IDC_STATIC,static,1342308352
Control2=IDC_LIST_MENU_ITEMS,listbox,1352728833
Control3=IDC_STATIC,static,1342308352
Control4=IDC_HOTKEY,msctls_hotkey32,1350631424
Control5=IDOK,button,1342242817
Control6=IDCANCEL,button,1342242816

[CLS:HotKeysDlg]
Type=0
HeaderFile=HotKeysDlg.h
ImplementationFile=HotKeysDlg.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC
LastObject=IDC_LIST_MENU_ITEMS

[DLG:IDD_ABOUT_DLG]
Type=1
Class=AboutDlg
ControlCount=2
Control1=IDOK,button,1342242817
Control2=IDC_RICHEDIT,RICHEDIT,1350633668

[CLS:AboutDlg]
Type=0
HeaderFile=AboutDlg.h
ImplementationFile=AboutDlg.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC
LastObject=IDC_EDIT

[DLG:IDD_TOOLS_DLG]
Type=1
Class=ToolsDlg
ControlCount=40
Control1=IDC_STATIC,button,1342177287
Control2=IDC_LIST_MENU_ITEMS,listbox,1352728961
Control3=IDC_BUTTON_IN_SUBMENU,button,1342242816
Control4=IDC_BUTTON_OUT_SUBMENU,button,1342242816
Control5=IDC_BUTTON_INSERT,button,1342242816
Control6=IDC_BUTTON_MOVE_UP,button,1342242816
Control7=IDC_BUTTON_DELETE,button,1342242816
Control8=IDC_BUTTON_MOVE_DOWN,button,1342242816
Control9=IDC_BUTTON_CHECK_MNEMONICS,button,1342242816
Control10=IDC_CHECK_IS_SUBMENU,button,1342242819
Control11=IDC_CHECK_IS_SEPARATOR,button,1342242819
Control12=IDC_STATIC_NAME,static,1342308352
Control13=IDC_EDIT_MENU_ITEM_NAME,edit,1350631552
Control14=IDC_STATIC_ENABLED_EXTS,static,1342308352
Control15=IDC_EDIT_EXTS,edit,1350631552
Control16=IDC_STATIC_VISIBLE_EXTS,static,1342308352
Control17=IDC_EDIT_VISIBLE_EXTS,edit,1350631552
Control18=IDC_STATIC_MACRO_HELP,static,1342308352
Control19=IDC_STATIC_COMMAND,static,1342308352
Control20=IDC_EDIT_COMMAND,edit,1350631552
Control21=IDC_BUTTON_COMMAND_MACRO,button,1342242880
Control22=IDC_BUTTON_BROWSE,button,1342242816
Control23=IDC_EDIT_SHOW_COMMAND,edit,1350568064
Control24=IDC_STATIC_PARAMETERS,static,1342308352
Control25=IDC_EDIT_PARAMETERS,edit,1350631552
Control26=IDC_BUTTON_PARAMETERS_MACRO,button,1342242880
Control27=IDC_EDIT_SHOW_PARAMETERS,edit,1350568064
Control28=IDC_STATIC_INIT_DIR,static,1342308352
Control29=IDC_EDIT_INIT_DIR,edit,1350631552
Control30=IDC_BUTTON_INIT_DIR_MACRO,button,1342242880
Control31=IDC_EDIT_SHOW_INIT_DIR,edit,1350568064
Control32=IDC_CHECK_IS_FILTER,button,1342242819
Control33=IDC_CHECK_SAVE_FILE,button,1342242819
Control34=IDC_CHECK_HIDE_COMMAND_WINDOW,button,1342242819
Control35=IDC_STATIC_HOTKEY,static,1342308352
Control36=IDC_HOTKEY,msctls_hotkey32,1350631424
Control37=IDC_BUTTON_CHECK_HOTKEY,button,1342242816
Control38=IDOK,button,1342242817
Control39=IDCANCEL,button,1342242816
Control40=IDC_BUTTON_HELP,button,1342242816

[CLS:ToolsDlg]
Type=0
HeaderFile=ToolsDlg.h
ImplementationFile=ToolsDlg.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC
LastObject=IDC_CHECK_IS_SUBMENU

[DLG:IDD_FINDREPLACE_DLG]
Type=1
Class=FindReplaceDlg
ControlCount=14
Control1=IDC_STATIC,static,1342308352
Control2=IDC_COMBO_FIND,combobox,1344340034
Control3=IDC_STATIC_REPLACE_WITH,static,1342308352
Control4=IDC_COMBO_REPLACE,combobox,1344340034
Control5=IDC_CHECK_WHOLE_WORD,button,1342242819
Control6=IDC_CHECK_CASE,button,1342242819
Control7=IDC_CHECK_CLOSE,button,1342242819
Control8=IDC_CHECK_UP,button,1342242819
Control9=IDC_CHECK_SELECTION_ONLY,button,1342242819
Control10=IDC_CHECK_ESCAPED_CHARS,button,1342252035
Control11=IDC_BUTTON_FIND,button,1342242817
Control12=IDC_BUTTON_REPLACE,button,1342242816
Control13=IDC_BUTTON_REPLACE_ALL,button,1342242816
Control14=IDCANCEL,button,1342242816

[CLS:FindReplaceDlg]
Type=0
HeaderFile=FindReplaceDlg.h
ImplementationFile=FindReplaceDlg.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC
LastObject=IDC_COMBO_FIND

[DLG:IDD_PROMPT_DLG]
Type=1
Class=PromptDlg
ControlCount=3
Control1=IDC_STATIC_PROMPT,static,1342308352
Control2=IDC_EDIT,edit,1350631552
Control3=IDOK,button,1342242817

[CLS:PromptDlg]
Type=0
HeaderFile=PromptDlg.h
ImplementationFile=PromptDlg.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC
LastObject=IDOK

[DLG:IDD_TIMEDATE_FORMAT_DLG]
Type=1
Class=TimeDateFormatDlg
ControlCount=8
Control1=IDC_STATIC,static,1342308352
Control2=IDC_EDIT_TIME_DATE_FORMAT,edit,1352728900
Control3=IDC_MACRO_BUTTON,button,1342242880
Control4=IDC_STATIC,static,1342308352
Control5=IDC_EDIT_TIME_DATE,edit,1352665156
Control6=IDOK,button,1342242817
Control7=IDCANCEL,button,1342242816
Control8=IDC_STATIC,static,1342308480

[CLS:TimeDateFormatDlg]
Type=0
HeaderFile=TimeDateFormatDlg.h
ImplementationFile=TimeDateFormatDlg.cpp
BaseClass=CDialog
Filter=D
LastObject=IDC_MACRO_BUTTON
VirtualFilter=dWC

