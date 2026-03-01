@echo off
setlocal

where pwsh >nul 2>nul
if errorlevel 1 (
    powershell -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0setup-agent-worktrees.ps1" %*
) else (
    pwsh -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0setup-agent-worktrees.ps1" %*
)
set "EXIT_CODE=%ERRORLEVEL%"

endlocal & exit /b %EXIT_CODE%
