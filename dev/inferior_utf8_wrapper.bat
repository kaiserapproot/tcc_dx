@echo off
chcp 65001 >nul
%*
exit /b %errorlevel%
