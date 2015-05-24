@echo off

set VCVARSALL=%VS120COMNTOOLS%..\..\VC\vcvarsall.bat
set MAKENSIS=%PROGRAMFILES%\NSIS\MakeNSIS.exe

set VERSION_MAJOR=1
set VERSION_MINOR=1
set VERSION_SUBMINOR=19

if not exist "%VCVARSALL%" echo ERROR: vcvarsall.bat not found & goto END
call "%VCVARSALL%" x86 > nul

if exist "%MAKENSIS%" goto NSISFOUND
set MAKENSIS=%MAKENSIS:Program Files\=Program Files (x86)\%
if not exist "%MAKENSIS%" echo ERROR: MakeNSIS.exe not found & goto END
:NSISFOUND

set MSBUILD="msbuild.exe" /nologo^
	/p:PlatformToolset=v120_xp;VisualStudioVersion=12.0^
	/p:Configuration=Release

if exist "Certificate.bat" call "Certificate.bat" > nul
set SIGNTOOL="signtool.exe" sign /t http://time.certum.pl /f "%CERTFILE%" /p "%CERTKEY%"

:: Update Version.h.
> "..\RainLexer\Version.h" echo #pragma once
>>"..\RainLexer\Version.h" echo #define RAINLEXER_VERSION_RC %VERSION_MAJOR%,%VERSION_MINOR%,%VERSION_SUBMINOR%,0
>>"..\RainLexer\Version.h" echo #define RAINLEXER_VERSION_STRING "%VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_SUBMINOR%.0"
>>"..\RainLexer\Version.h" echo #define RAINLEXER_TITLE L"RainLexer %VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_SUBMINOR%"

echo * Building RainLexer.
%MSBUILD% /t:rebuild /p:Platform=Win32 /v:q /m ..\RainLexer.sln
if not %ERRORLEVEL% == 0 echo   ERROR %ERRORLEVEL%: Build failed & goto END

:: Sign installer.
if not "%CERTFILE%" == "" (
	echo * Signing RainLexer.dll.
	%SIGNTOOL% ..\Release\RainLexer.dll > BuildLog.txt
	if not %ERRORLEVEL% == 0 echo   ERROR %ERRORLEVEL%: Signing RainLexer.dll failed. & goto END
)

set INSTALLER_NAME=RainLexer-%VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_SUBMINOR%.exe

set INSTALLER_DEFINES=^
	/DOUTFILE="%INSTALLER_NAME%"^
	/DVERSION="%VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_SUBMINOR%"

echo * Building installer.
"%MAKENSIS%" %INSTALLER_DEFINES% ..\Installer\Installer.nsi > "BuildLog.txt"
if not %ERRORLEVEL% == 0 echo   ERROR %ERRORLEVEL%: Building installer failed. & goto END

:: Sign installer.
if not "%CERTFILE%" == "" (
	echo * Signing installer.
	%SIGNTOOL% %INSTALLER_NAME% > BuildLog.txt
	if not %ERRORLEVEL% == 0 echo   ERROR %ERRORLEVEL%: Signing installer failed. & goto END
)

:: If we got here, build was successful so delete BuildLog.txt.
if exist "BuildLog.txt" del "BuildLog.txt"

echo * Build complete.

:END
echo.
pause
