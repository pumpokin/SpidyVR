@echo off
rem Offers the Spidy VR tutorial again at the next START VR.
del "%LOCALAPPDATA%\Spidy\tutorial.txt" 2>nul
echo The tutorial will be offered next time you start VR.
pause
