@echo off

set Defines=/DWIN32 /D_DEBUG /DUNICODE /D_UNICODE /DLIMIT_INTERNAL
set CommonCompilerFlags=/W4 /wd4706 /MT /nologo /std:c++17 /Gm- /GR-
set CommonLinkerFlags=/opt:ref

:: -subsystem:windows,5.02
:: -subsystem:windows,5.1

IF NOT EXIST bin\ mkdir bin\
pushd bin\
cl %CommonCompilerFlags% /EHsc- /EHa- /Zi /Od /Oi %Defines% ..\src\Win32_Limit.cpp /link %CommonLinkerFlags% /DEBUG
popd