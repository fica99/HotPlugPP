@echo off
REM check-format.bat - Check if C++ code follows the style guidelines (Windows)
REM This is a cross-platform wrapper that calls the CMake script

cd /d "%~dp0"
cmake -P check-format.cmake
