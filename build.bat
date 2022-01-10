@echo off
@setlocal enableextensions enabledelayedexpansion

set PROJECT=xnvme
set PROJECT_VER=0.0.27
set CC=clang
set MESON=meson
set BUILD_TYPE=release
set PLATFORM_ID=Windows
set BUILD_DIR=builddir
set NPROC=%NUMBER_OF_PROCESSORS%
set CTAGS=ctags
set GIT=git
set INFO=
set CLOBBER=
set CONFIG=
set CLEAN=
set INSTALL=
set BUILD=

:: set msys2-shell
set SH=call msys2_shell -no-start -here -use-full-path -defterm

for %%i in (%*) do (
	if "%%i"=="debug" set BUILD_TYPE=debug
	if "%%i"=="release" set BUILD_TYPE=release
	if "%%i"=="info" set INFO=info
	if "%%i"=="config" set CONFIG=config
	if "%%i"=="config-debug" set CONFIG=config-debug
	if "%%i"=="build" set BUILD=build
	if "%%i"=="clean" set CLEAN=clean
	if "%%i"=="clobber" set CLOBBER=clobber
	if "%%i"=="install" set INSTALL=install
	if "%%i"=="uninstall" set INSTALL=uninstall
)

set "PATH=%ALLUSERSPROFILE%\chocolatey\bin;!PATH!"
set "PATH=%SystemDrive%\tools\msys64;!PATH!"
set "PATH=%SystemDrive%\tools\msys64\mingw64\bin;!PATH!"


if "%INFO%"=="info" (
	@call :info
	goto :eof
)

if "%CONFIG%"=="config" (
	@call :config
	goto :eof
)

if "%CONFIG%"=="config-debug" (
	@call :config-debug
	goto :eof
)

if "%BUILD%"=="build" (
	@call :build
	goto :eof
)

if "%CLEAN%"=="clean" (
	@call :clean
	goto :eof
)

if "%INSTALL%"=="install" (
	@call :install
	@goto :eof
)

if "%INSTALL%"=="uninstall" (
	@call :uninstall
	@goto :eof
)

if "%CLOBBER%"=="clobber" (
	@call :clobber
	@goto :eof
)

if [%1]==[] (call :default) else (echo "Wrong arguments")

goto :eof

:default
	call :info
	call :tags
	call :git-setup
	@echo "## xNVMe: make default"
	@if not exist %BUILD_DIR% (
		call :config
		call :build
	)
	@goto :eof

:config
	@echo "## xNVMe: make config"
	@set CC=%CC%
	%SH% -c "%MESON% setup %BUILD_DIR%"
	@echo "## xNVMe: make config [DONE]"
	@goto :eof

:config-debug
	@echo "## xNVMe: make config-debug"
	@set CC=%CC%
	%SH% -c "%MESON% setup %BUILD_DIR% --buildtype=debug"
	@echo "## xNVMe: make config-debug [DONE]"
	@goto :eof

:build
	@call :info
	@call :_require_builddir
	@echo "## xNVMe: make build"
	%SH% -c "%MESON% compile -C %BUILD_DIR%"
	@echo "## xNVMe: make build [DONE]"
	@goto :eof

:install
	@call :_require_builddir
	@echo "## xNVMe: make install"
	%SH% -c "%MESON% install -C %BUILD_DIR%"
	@echo "## xNVMe: make install [DONE]"
	@goto :eof

:uninstall
	@echo "## xNVMe: make install"
	@if not exist %BUILD_DIR% (
		echo "Missing builddir('%BUILD_DIR%)"
	)
	@cd %BUILD_DIR%
	%SH% -c "%MESON% --internal uninstall"
	@echo "## xNVMe: make install [DONE]"
	@goto :eof

:clean
	@echo "## xNVMe: make clean"
	%SH% -c "rm -fr %BUILD_DIR% || true"
	@echo "## xNVMe: make clean [DONE]"
	@goto :eof

:info
	@echo "## xNVMe: make info"
	@echo "PLATFORM: %PLATFORM_ID%"
	@echo "CC: %CC%"
	@echo "CTAGS: %CTAGS%"
	@echo "NPROC: %NPROC%"
	@echo "PROJECT_VER: %PROJECT_VER%"
	@echo "## xNVMe: make info [DONE]"
	@goto :eof

:git-setup
	@echo "## xNVMe:: git-setup"
	%SH% -c "%GIT% config core.hooksPath .githooks || true"
	@echo "## xNVMe:: git-setup [DONE]"
	@goto :eof

:tags
	@echo "## xNVMe: make tags"
	%SH% -c "%CTAGS% * --languages=C -h="".c.h"" -R --exclude=builddir,include,src,tools,examples,tests,subprojects/* || true"
	@echo "## xNVMe: make tags [DONE]"
	@goto :eof

:_require_builddir
	@if not exist %BUILD_DIR% (
		echo ""
		echo "+========================================+"
		echo "                                          "
		echo " ERR: missing builddir: '%BUILD_DIR%'    "
		echo "                                          "
		echo " Configure it first by running:           "
		echo "                                          "
		echo " 'meson setup %BUILD_DIR%'               "
		echo "                                          "
		echo "+========================================+"
		echo ""
	)
	goto :eof

::
:: clobber: clean third-party builds, drop any third-party changes and clean any
:: untracked stuff lying around
::
:clobber
	@call :clean
	@echo "## xNVMe: clobber"
	@git clean -dfx
	@git clean -dfX
	@git checkout .
	@rmdir /s /q subprojects\fio
	@rmdir /s /q subprojects\spdk
	@rmdir /s /q subprojects\liburing
	@echo "## xNVMe: clobber [DONE]"
	@goto :eof
