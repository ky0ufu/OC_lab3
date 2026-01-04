@echo off
setlocal

set BUILD_DIR=build-win
set CONFIG=Release

cmake -S . -B %BUILD_DIR% -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=%CONFIG%
if errorlevel 1 exit /b %errorlevel%

cmake --build %BUILD_DIR%
exit /b %errorlevel%