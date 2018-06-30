@echo off

set DEFINES=/D "WIN32" /D "UNICODE" /D "_UNICODE" 
set COMPILER_SWITCHES=/std:c++17 /EHsc- /EHa- /W4 /wd4201 /wd4100 /wd4706 /wd4189 /GR- /FC /nologo
set LINKER_SWITCHES=/OPT:REF /INCREMENTAL:NO

set ARG_DEBUG=%2
IF "%ARG_DEBUG%" == "DEBUG" (
    set DEFINES=%DEFINES% /D "_DEBUG" /D "LIMIT_INTERNAL"
    set COMPILER_SWITCHES=%COMPILER_SWITCHES% /Z7 /Od /Oi /MTd
    set LINKER_SWITCHES=%LINKER_SWITCHES% /DEBUG 
)
:: #TODO: Optimized build when DEBUG is not specified

echo ^>--------- Build started ----------^<

IF NOT EXIST bin\ mkdir bin\
pushd bin\
del *.pdb >NUL 2> NUL

:: GAME, ENGINE, BOTH
set ARG_MODE=%1
IF "%ARG_MODE%" == "GAME" (
	CALL :build_game
	GOTO build_finished
)
IF "%ARG_MODE%" == "ENGINE" (
	CALL :build_engine
	GOTO build_finished
)
IF "%ARG_MODE%" == "BOTH" (
	CALL :build_game
	CALL :build_engine
	GOTO build_finished
)

:build_finished
popd
echo ^>--------- Build finished ---------^<
EXIT /B %ERRORLEVEL% 

:: Build functions
:build_game
cl %DEFINES% %COMPILER_SWITCHES% ..\src\test_game\TestGame.cpp /LD /link %LINKER_SWITCHES% /PDB:TestGame_%RANDOM%.pdb /EXPORT:GameUpdateAndRender /EXPORT:GameGetSoundSamples
EXIT /B 0

:build_engine
cl %DEFINES% %COMPILER_SWITCHES% ..\src\Win32_Limit.cpp /link %LINKER_SWITCHES%
EXIT /B 0
