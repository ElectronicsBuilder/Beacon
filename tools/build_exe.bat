@echo off
rem Double-click: builds dist\Beacon.exe (see build_exe.ps1)
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0build_exe.ps1"
pause
