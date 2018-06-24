@echo off

set DEFINES=/D "WIN32" /D "UNICODE" /D "_UNICODE" 
set COMPILER_SWITCHES=/std:c++17 /EHsc- /EHa- /W4 /wd4100 /wd4706 /GR- /FC /nologo
set LINKER_SWITCHES=/OPT:REF /INCREMENTAL:NO

set DEBUG=%1
IF "%DEBUG%" == "DEBUG" (
    set DEFINES=%DEFINES% /D "_DEBUG" /D "LIMIT_INTERNAL"
    set COMPILER_SWITCHES=%COMPILER_SWITCHES% /Z7 /Od /Oi /MTd
    set LINKER_SWITCHES=%LINKER_SWITCHES% /DEBUG 
)
:: #TODO: Optimized build when DEBUG is not specified

echo ^>--------- Build started ---------^<

IF NOT EXIST bin\ mkdir bin\
pushd bin\
del *.pdb >NUL 2> NUL
cl %DEFINES% %COMPILER_SWITCHES% ..\src\test_game\TestGame.cpp /LD /link %LINKER_SWITCHES% /PDB:TestGame_%RANDOM%.pdb /EXPORT:GameUpdateAndRender /EXPORT:GameGetSoundSamples
cl %DEFINES% %COMPILER_SWITCHES% ..\src\Win32_Limit.cpp /link %LINKER_SWITCHES%
popd

echo ^>--------- Build finished ---------^<