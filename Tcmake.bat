@echo off
rem USAGE tcmake metacomp/decomp/mf

rem -----------------------------------
rem Edit Turbo C main path here
rem -----------------------------------
set TCPATH=h:\c\tc

set OPATH=%PATH%
set PATH=%TCPATH%;%PATH%

%TCPATH%\tcc.exe -mh -w-sus -f87 -I%TCPATH%\include -L%TCPATH%\lib %1
del %1.obj

set PATH=%OPATH%
set OPATH=
set TCPATH=

pause
