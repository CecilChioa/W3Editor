@echo off
setlocal

cd /d "%~dp0"

set "EXE=%cd%\build\Debug\W3Editor.exe"
if not exist "%EXE%" (
  echo [error] %EXE% not found.
  echo Run build_debug.bat first.
  exit /b 1
)

start "" "%EXE%"
exit /b 0

