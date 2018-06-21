@echo off

mkdir bin\
pushd bin\
cl /std:c++17 /DLIMIT_INTERNAL /DWIN32 /D_DEBUG /DUNICODE /D_UNICODE /Zi /Gm /Od /W4 ..\src\Win32_Limit.cpp /DEBUG /INCREMENTAL
popd