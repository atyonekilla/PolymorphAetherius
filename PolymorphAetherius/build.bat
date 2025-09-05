@echo off
echo Building PolymorphAetherius...
echo.

msbuild PolymorphAetherius.sln /p:Configuration=Release /p:Platform=x64

if %errorlevel% equ 0 (
    echo Build successful!
    echo Output: bin\x64\Release\PolymorphAetherius.exe
) else (
    echo Build failed!
    exit /b 1
)