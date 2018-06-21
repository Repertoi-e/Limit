@echo off

IF NOT EXIST bin\ mkdir bin\
pushd bin\
cl /W4 /wd4706 /MT /nologo /std:c++17 /DLIMIT_INTERNAL /DWIN32 /D_DEBUG /DUNICODE /D_UNICODE /Zi /Gm- /GR- /EHsc- /EHa- /Od /Oi ..\src\Win32_Limit.cpp /link /opt:ref /subsystem:windows,5.1 /DEBUG
popd