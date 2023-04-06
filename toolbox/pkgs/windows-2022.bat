@echo off
@setlocal enableextensions enabledelayedexpansion

net session >nul: 2>&1
if errorlevel 1 (
	echo %0 must be run with Administrator privileges
	goto :eof
)

:: Use PowerShell to install Chocolatey
set "PATH=%ALLUSERSPROFILE%\chocolatey\bin;%PATH%"
echo [1/11] Install: Chocolatey Manager
powershell -NoProfile -InputFormat None -ExecutionPolicy Bypass -Command "[System.Net.ServicePointManager]::SecurityProtocol = 3072; iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))"
where /q choco
if errorlevel 1 (
	echo [1/11] Install: Chocolatey = FAIL
	goto :eof
)
echo [1/11] Install: Chocolatey = PASS

:: Use Chocolatey to install msys2
echo [2/11] Install: MSYS2
set "PATH=%SystemDrive%\tools\msys64;%PATH%"
choco install msys2 -y -r
where /q msys2
if errorlevel 1 (
	echo [2/11] Install: MSYS2 = FAIL
	goto :eof
)
echo [2/11] Install: MSYS2 = PASS

echo [3/11] Install: Git
choco install git -y -r
where /q git
echo [3/11] Install: Git = PASS

echo [4/11] Install: dos2unix
choco install dos2unix
echo [4/11] Install: dos2unix = PASS

:: Use MSYS2/pacman to install gcc and clang toolchain
set MSYS2=call msys2_shell -no-start -here -use-full-path -defterm

echo [5/11] Install: MinGW Toolchain via msys2/pacman
set "PATH=%SystemDrive%\tools\msys64\mingw64\bin;%PATH%"
%MSYS2% -c "pacman --noconfirm -Syy --needed base-devel mingw-w64-x86_64-toolchain"
where /q gcc
if errorlevel 1 (
	echo [5/11] Install: MinGW Toolchain via msys2/pacman = FAIL
	goto :eof
)
echo [5/11] Install: MinGW Toolchain via msys2/pacman = PASS

echo [6/11] Install: MinGW/clang via msys2/pacman
%MSYS2% -c "pacman --noconfirm -Syy mingw-w64-x86_64-clang"
where /q clang
if errorlevel 1 (
	echo [6/11] Install: MinGW Toolchain via msys2/pacman = FAIL
	goto :eof
)
echo [6/11] Install: MinGW Toolchain via msys2/pacman = PASS

echo [7/11] Install: MinGW/nsis via msys2/pacman
%MSYS2% -c "pacman --noconfirm -Syy mingw-w64-x86_64-nsis"
echo [7/11] Install: MinGW/nsis via msys2/pacman = OK

echo [8/11] Install: MinGW/meson via msys2/pacman
%MSYS2% -c "pacman --noconfirm -Syy mingw-w64-x86_64-meson"
where /q meson
if errorlevel 1 (
	echo [8/11] Install: MinGW/meson via msys2/pacman = FAIL
	goto :eof
)
echo [8/11] Install: MinGW/meson via msys2/pacman = OK

echo [9/11] Install: Python3 and dependencies via msys2/pacman
%MSYS2% -c "pacman --noconfirm -Syy mingw-w64-x86_64-python mingw-w64-x86_64-python-pip"
echo [9/11] Install: Python3 and dependencies via msys2/pacman = OK

echo [10/11] Install: Make and dependencies via msys2/pacman
%MSYS2% -c "pacman --noconfirm -Syy mingw-w64-x86_64-make"
echo [10/11] Install: Make and dependencies via msys2/pacman = OK

echo [11/11] Install: Autotools and dependencies via msys2/pacman
%MSYS2% -c "pacman --noconfirm -Syy mingw-w64-x86_64-autotools"
echo [11/11] Install: Autotools and dependencies via msys2/pacman = OK
