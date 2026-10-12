@echo off
cd /d "%~dp0"
set SDL_DYNAMIC_API=%~dp0kirby_bezel.dll
start "" "%~dp0KirbyRecomp.exe" --fullscreen 1 --state-dir "%~dp0saves"
