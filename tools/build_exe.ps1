# build_exe.ps1 - build dist\Beacon.exe (one file, no console window) with PyInstaller.
#
#   cd tools
#   powershell -ExecutionPolicy Bypass -File build_exe.ps1     (or double-click build_exe.bat)
#
# Installs the requirements and PyInstaller first if they are missing.
$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot

Write-Host "== Python packages"
python -m pip install --quiet -r requirements.txt
python -m pip install --quiet "pyinstaller>=6.0"
if ($LASTEXITCODE -ne 0) { throw "pip failed" }

Write-Host "== App icon"
python -m scoreboard.theme                       # (re)draws scoreboard\assets\beacon.ico / .png
if ($LASTEXITCODE -ne 0) { throw "icon failed" }

Write-Host "== PyInstaller"
$assets = Join-Path $PSScriptRoot "scoreboard\assets"
$ico = Join-Path $assets "beacon.ico"
$png = Join-Path $assets "beacon.png"
$main = Join-Path $PSScriptRoot "beacon.pyw"
python -m PyInstaller --noconfirm --clean --onefile --windowed `
    --name Beacon `
    --icon $ico `
    --add-data "$ico;scoreboard\assets" `
    --add-data "$png;scoreboard\assets" `
    --collect-data customtkinter `
    --collect-submodules bleak `
    --collect-submodules winrt `
    --distpath dist --workpath build\pyinstaller --specpath build `
    $main
if ($LASTEXITCODE -ne 0) { throw "PyInstaller failed" }

$exe = Join-Path $PSScriptRoot "dist\Beacon.exe"
Write-Host ""
Write-Host "Built: $exe ($([math]::Round((Get-Item $exe).Length / 1MB, 1)) MB)"
