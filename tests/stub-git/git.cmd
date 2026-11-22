@echo off
setlocal
set "root=%~2"
set "author="
set "stamp="
if "%root:~-1%"=="\" goto :eof
if "%root:~-1%"=="/" goto :eof
if "%~3"=="rev-parse" goto shallow
:next
if "%~1"=="" goto answer
if "%~1"=="%%at" set "author=yes"
if "%~1"==":(glob)*.c" set "stamp=1700000000"
if "%~1"==":(glob)docs/source/*" set "stamp=1697408000"
if "%~1"==":(glob)src/*.c" set "stamp=1700000000"
if "%~1"==":(glob)docs/source/src/*" set "stamp=1698272000"
if "%~1"==":(glob)lib/*.c" set "stamp=1700000000"
if "%~1"==":(glob)docs/source/lib/*" set "stamp=1698790400"
shift
goto next
:answer
if defined author if defined stamp echo %stamp%
goto :eof
:shallow
if "%root:~-18%"=="source-doc-shallow" echo true
if not "%root:~-18%"=="source-doc-shallow" echo false
