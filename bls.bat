@echo off
@REM hack to suppress batch job termination confirmation while retaining error code
setlocal enabledelayedexpansion
py ./scripts/driver.py build/cli.py %* & set EXITCODE=!ERRORLEVEL! & call;
exit /b %EXITCODE%