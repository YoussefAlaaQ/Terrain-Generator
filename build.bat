@echo off

cls

IF NOT EXIST "build" (
    mkdir build
)

copy "dependancy\SDL3.dll" "build\"
cls

REM Store include & linker flags (no quotes!)
set incPath=/I \inc
set libPath=/LIBPATH:\lib\SDL3\MSVC\lib\x64

set libs=SDL3.lib user32.lib shell32.lib Gdi32.lib Opengl32.lib

pushd build
cl -DGAME_SLOW /nologo /Zi /INCREMENTAL:NO %incPath% ../main.cpp^
  /Fe:main.exe /favor:AMD64^
  /link /NOLOGO /INCREMENTAL:NO %libPath% %libs%
popd
