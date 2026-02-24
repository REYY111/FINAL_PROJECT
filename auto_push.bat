@echo off
title AUTO GITHUB BACKUP

cd /d %~dp0

:loop
echo Checking changes...

git add .

git diff --cached --quiet
IF %ERRORLEVEL%==0 (
    echo No changes
) ELSE (
    git commit -m "auto backup %date% %time%"
    git push
)

timeout /t 120 >nul
goto loop
