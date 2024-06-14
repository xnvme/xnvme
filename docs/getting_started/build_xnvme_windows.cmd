# Clone the xNVMe repos
git clone https://github.com/xnvme/xnvme.git
cd xnvme

# Install toolchain and libraries on Windows (2022)
Set-ExecutionPolicy Bypass -Scope Process -Force
.\toolbox\pkgs\windows-2022.ps1

# Setup helper-function to invoke commands inside msys2 environment from Powershell
function run { param([string]$cmd=$(throw "You must specify a command.")); & msys2_shell -no-start -here -use-full-path -defterm -mingw64 -c "$cmd" }

# configure xNVMe (adjust this to correct path)
run "meson setup builddir --prefix=/usr"

# build xNVMe
run "meson compile -C builddir"

# install xNVMe
run "meson install -C builddir"

# uninstall xNVMe
# cd builddir && run "meson --internal uninstall"