@echo off
setlocal

cd /d "%~dp0"

set "CMAKE_EXE=cmake"
where cmake >nul 2>nul
if errorlevel 1 (
  for %%V in (18 2022) do (
    for %%E in (Community Professional Enterprise BuildTools) do (
      if exist "C:\Program Files\Microsoft Visual Studio\%%V\%%E\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
        set "CMAKE_EXE=C:\Program Files\Microsoft Visual Studio\%%V\%%E\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
        goto :found_cmake
      )
    )
  )
  echo [error] CMake was not found.
  echo Install CMake or Visual Studio CMake tools, then retry.
  exit /b 1
)
:found_cmake

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
"%CMAKE_EXE%" -S . -B build -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" -DVCPKG_FEATURE_FLAGS=manifests
if errorlevel 1 exit /b %errorlevel%

echo [2/2] Build Debug...
"%CMAKE_EXE%" --build build --config Debug -j 8
if errorlevel 1 exit /b %errorlevel%

echo [ok] Build finished: %cd%\build\Debug\W3Editor.exe
exit /b 0
