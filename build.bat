@echo off
rem ShopSphere build script for Windows (MinGW g++ 6.3 or newer).
rem
rem   build.bat          build bin\shopsphere.exe and the test programs
rem   build.bat test     build, then run all tests
rem   build.bat demo     build, then run the scripted demo
rem   build.bat run      build, then start the app
rem   build.bat clean    delete build\ and bin\
setlocal
cd /d "%~dp0"

if /I "%~1"=="clean" goto :clean

where g++ >nul 2>nul
if errorlevel 1 (
    echo g++ was not found. Install MinGW and add C:\MinGW\bin to PATH.
    exit /b 1
)

set "CXXFLAGS=-std=c++14 -Wall -Wextra -O2 -Iinclude"
if not exist bin mkdir bin
if not exist build mkdir build
if exist build\objects.rsp del /q build\objects.rsp

echo.
echo  ShopSphere - compiling modules
echo  ------------------------------
for %%d in (util core cart payment services ui) do (
    for %%f in (src\%%d\*.cpp) do (
        echo    %%d/%%~nxf
        g++ %CXXFLAGS% -c src/%%d/%%~nxf -o build/%%d_%%~nf.o || goto :fail
        echo build/%%d_%%~nf.o>>build\objects.rsp
    )
)

echo    linking bin\shopsphere.exe
g++ %CXXFLAGS% src/main.cpp @build/objects.rsp -o bin/shopsphere.exe || goto :fail
for %%t in (test_all test_partA test_partB) do (
    echo    linking bin\%%t.exe
    g++ %CXXFLAGS% tests/%%t.cpp @build/objects.rsp -o bin/%%t.exe || goto :fail
)
echo  Build OK.
echo.

if /I "%~1"=="test" goto :test
if /I "%~1"=="demo" goto :demo
if /I "%~1"=="run" goto :run
exit /b 0

:test
bin\test_all.exe || goto :fail
bin\test_partA.exe >nul || goto :fail
bin\test_partB.exe >nul || goto :fail
echo  Part A and Part B drivers ran successfully.
exit /b 0

:demo
bin\shopsphere.exe --demo %2 %3
exit /b %errorlevel%

:run
bin\shopsphere.exe %2 %3
exit /b %errorlevel%

:clean
if exist build rmdir /s /q build
if exist bin rmdir /s /q bin
echo  Cleaned.
exit /b 0

:fail
echo.
echo  Build or test FAILED.
exit /b 1
