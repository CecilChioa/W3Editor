@echo off
setlocal

cd /d "%~dp0"

if "%VCPKG_ROOT%"=="" (
  echo [error] VCPKG_ROOT is not set.
  echo Example: set "VCPKG_ROOT=C:\tools\vcpkg"
  exit /b 1
)

if not exist "%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" (
  echo [error] Invalid VCPKG_ROOT: "%VCPKG_ROOT%"
  echo Missing: %VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
  exit /b 1
)

echo [1/2] CMake configure...
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" -DVCPKG_FEATURE_FLAGS=manifests
if errorlevel 1 exit /b %errorlevel%

echo [2/2] Build Debug...
cmake --build build --config Debug -j 8
if errorlevel 1 exit /b %errorlevel%

echo [ok] Build finished: %cd%\build\Debug\W3Editor.exe
exit /b 0

