# Check if the script is run with Administrator privileges
if (-not ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")) {
    Write-Host "$($MyInvocation.MyCommand.Name) must be run with Administrator privileges"
    exit 1
}

# Setup PATH for this session
$env:PATH = "$env:PATH;$env:ALLUSERSPROFILE\chocolatey\bin"
$env:PATH = "$env:PATH;$env:SystemDrive\tools\msys64\mingw64\bin"
$env:PATH = "$env:PATH;$env:SystemDrive\tools\msys64\usr\bin"
$env:PATH = "$env:PATH;$env:SystemDrive\tools\msys64\usr\lib"
$env:PATH = "$env:PATH;$env:SystemDrive\tools\msys64"

# Persist the changed PATH for future sessions
$persist_path = $env:PATH
Set-ItemProperty -Path "HKCU:\Environment" -Name "PATH" -Value $persist_path

# Setup PKG_CONFIG_PATH for this session
$env:PKG_CONFIG_PATH = "$env:PKG_CONFIG_PATH;$env:SystemDrive\tools\msys64\usr\lib\pkgconfig"

# Persist the changed PKG_CONFIG_PATH for future sessions
$persist_path = $env:PKG_CONFIG_PATH
Set-ItemProperty -Path "HKCU:\Environment" -Name "PKG_CONFIG_PATH" -Value $persist_path

# Set execution policy to allow script execution
Set-ExecutionPolicy Bypass -Scope Process -Force

# Download and install Chocolatey
Invoke-Expression ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))

# Use Chocolatey to install msys2, git, and meson
choco install msys2 git -y -r

# Use msys2 to install the compilers and other tools
Start-Process msys2_shell -ArgumentList '-no-start -here -use-full-path -defterm -c "pacman --noconfirm -Syy --needed base-devel mingw-w64-x86_64-toolchain mingw-w64-x86_64-meson mingw-w64-x86_64-python mingw-w64-x86_64-python-pip"' -NoNewWindow -Wait

# Check for existence of required tools
$tools = @("choco", "git", "gcc", "meson")
foreach ($tool in $tools) {
    if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) {
        Write-Host "$tool is not installed or not in the PATH."
        exit 1
    } else {
        Write-Host "$tool is installed."
    }
}

