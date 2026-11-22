@echo off
:next
if "%~1"=="" goto :eof
if "%~1"==":(glob)src/*.c" echo 1700000000
if "%~1"==":(glob)docs/source/src/*" echo 1698272000
if "%~1"==":(glob)lib/*.c" echo 1700000000
if "%~1"==":(glob)docs/source/lib/*" echo 1698790400
shift
goto next
