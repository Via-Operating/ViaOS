REM Converted to batch by brad

@echo off
REM DON'T DELETE THIS! THIS IS AN IMPORTANT FILE!

set HEADER=viaSh.h

> %HEADER% echo // DON'T DELETE THIS FILE!
>> %HEADER% echo #ifndef __ALL_HEADERS__
>> %HEADER% echo #define __ALL_HEADERS__

for %%f in (term\viaSh\*.h) do (
    echo #include <via/shell/%%~nxf> >> %HEADER%
)

>> %HEADER% echo #endif
