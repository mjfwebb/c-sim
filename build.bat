@echo off
cls

@rem Modify the path for your vcvars setup
set "__localVCVarsPath=F:\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
set __outputMessage=

@rem Setup vcvars environment
if not defined VSCMD_ARG_TGT_ARCH (
    if exist "%__localVCVarsPath%" (
        call "%__localVCVarsPath%"
    ) else (
        set "__outputMessage=Unable to locate vcvars path at: %__localVCVarsPath%"
        goto end
    )
)

setlocal ENABLEDELAYEDEXPANSION

    if "%~1"=="-d" (
        set "debugMode=1"
    ) else (
        set "debugMode=0"
    )

    set "exeName=cultivation.exe"
    set "compilerFlags=-W4 -WX -O2 -nologo -Zc:strictStrings -GR- -favor:INTEL64 -cgthreads8 -MP /EHsc"
    set "ignoreWarnings=-wd4100 -wd4996"
    set "includeDirs=..\include"
    set "linkerFlags=-INCREMENTAL:NO"

    if %debugMode%==1 (
        set "compilerFlags=%compilerFlags% -FC -Z7"
    )

    if not exist build\NUL mkdir build

    pushd build

        @rem Compilation
        cl %compilerFlags% %ignoreWarnings% -I %includeDirs% ..\src\main.c ..\src\fonts.c ..\src\colors.c ..\src\defs.c -Fe..\%exeName% -link %linkerFlags% ..\lib\*.lib

        if %ERRORLEVEL%==0 (
        set "__outputMessage=Build successful"
        start /MIN "" Powershell -c "(New-Object Media.SoundPlayer '..\success.wav').PlaySync();"
        ) else (
        set "__outputMessage=Build failed"
        start /MIN "" Powershell -c "(New-Object Media.SoundPlayer '..\fail.wav').PlaySync();"
        )

    popd

    @rem Cleanup
    if %debugMode%==0 (
        rmdir /s /q build
    )

    echo %__outputMessage%

endlocal

:end
set __localVCVarsPath=
set __outputMessage=
