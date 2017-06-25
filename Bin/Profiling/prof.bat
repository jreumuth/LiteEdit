@echo off
pushd ..

prep /sf EditCtrl::Paste /om /ft LiteEdit.exe
rem prep /sf EditCtrl::Paste /om /ft /excall /inc EditCtrl.obj /inc Utility.obj /inc CodeColoring.obj /inc LiteEditDlg.obj /inc Persist.obj LiteEdit.exe
rem prep /om /ft /excall /inc EditCtrl.obj /inc Utility.obj /inc CodeColoring.obj /inc LiteEditDlg.obj /inc Persist.obj LiteEdit.exe
if errorlevel 1 goto done
profile LiteEdit
if errorlevel 1 goto done
prep /m LiteEdit
if errorlevel 1 goto done
plist LiteEdit > out.txt
notepad out.txt

:done
popd