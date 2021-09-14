@echo off
@setlocal enableextensions enabledelayedexpansion

set CC=clang
set TYPE=release
set PLATFORM_ID=Windows
set CLEAN=

set "CONFDIR=config"
set cfg=
if exist %CONFDIR%\_config set /p cfg=<%CONFDIR%\_config

:: set msys2-shell
set SH=call msys2_shell -no-start -here -use-full-path -defterm

for %%i in (%cfg% %*) do (
	if "%%i"=="clang" set CC=clang
	if "%%i"=="debug" set TYPE=debug
	if "%%i"=="release" set TYPE=release
	if "%%i"=="clean" set CLEAN=clean
)

set "PATH=%ALLUSERSPROFILE%\chocolatey\bin;!PATH!
set "PATH=%ProgramFiles%\CMake\bin;!PATH!
set "PATH=%SystemDrive%\tools\msys64;!PATH!"
set "PATH=%SystemDrive%\tools\msys64\mingw64\bin;!PATH!"

set "cfg=%CC% %TYPE%"
:: save config
if not exist %CONFDIR% mkdir %CONFDIR%
echo %cfg%>%CONFDIR%\_config

if "%CLEAN%"=="clean" (
	echo Cleaning...
	%SH% -c "make clean"
	echo "call build.bat again to build..."
)

if "%CLEAN%"=="clean" goto :eof

if "%CC%"=="clang" (
	where /q clang
	if errorlevel 1 (
		echo echo Requires the Clang compiler be present in PATH
		goto :eof
	)
)

set CXX=%CC%
if "%CC%"=="clang" set "CXX=clang++"

set LD=
if "%CC%"=="clang" set LD=lld-link

set ENV=CC='%CC%' CXX='%CXX%' LD='%LD%' PLATFORM_ID='%PLATFORM_ID%'

echo Building %cfg%...

:: enable debug
if "%TYPE%"=="debug" set CONFIG_OPTS=--enable-debug

:: call ./configure
%SH% -c "%ENV% ./configure !CONFIG_OPTS!"
:: call make
%SH% -c "make"
