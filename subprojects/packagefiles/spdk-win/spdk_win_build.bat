echo off

set cur_path=%~dp0

:: change line end to LF
git config core.eol lf
git config core.autocrlf input
git rm --cached -r .
git reset --hard

:: dos2unix to newly added script
dos2unix %cur_path%spdk_win_patches.sh

:: apply spdk patches
wsl -e sudo ./spdk_win_patches.sh

:: intall prerequisites
wsl -e sudo ./scripts/pkgdep/debian.sh

:: start build
wsl -e sudo ./../wpdk/build.sh
