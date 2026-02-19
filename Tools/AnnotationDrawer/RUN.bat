@echo off
cd /d "%~dp0"
powershell -ExecutionPolicy Bypass -File "draw_annotations.ps1"
pause
