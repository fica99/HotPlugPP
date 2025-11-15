@echo off
REM format-code.bat - Format all C++ source files in the project (Windows)
REM This is a cross-platform wrapper that calls the CMake script

cd /d "%~dp0"
cmake -P format-code.cmake
