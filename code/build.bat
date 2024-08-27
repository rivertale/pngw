@echo off

if not exist ..\build mkdir ..\build
pushd ..\build
cl /nologo /Od /Zi /Fe:pngw.exe ..\code\main.c /link /incremental:no user32.lib comdlg32.lib
del /q *.obj vc140.pdb
popd

if not exist ..\release mkdir ..\release
if not exist ..\release\pngw mkdir ..\release\pngw
pushd ..\release\pngw
cl /nologo /O2 /Fe:pngw.exe ..\..\code\main.c /link /incremental:no user32.lib comdlg32.lib
del /q *.obj vc140.pdb
popd
