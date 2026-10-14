@echo off
rem ===================================================================
rem  双击本文件即可把固件烧录到 STM32（放在项目根目录，方便找）
rem  前提：ST-Link 已插在电脑上，板子已通电（USB 或 3.3V）
rem ===================================================================
setlocal enabledelayedexpansion

set "CLT=D:\STM32CubeCLT\STM32CubeCLT_1.22.0"
set "PROG=%CLT%\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe"
set "PROJ=%~dp0AIcar"
set "ELF="

for /d %%d in ("%PROJ%\cmake-build-debug-stm32*") do (
    if exist "%%d\AIcar.elf" set "ELF=%%d\AIcar.elf"
)

if not defined ELF (
    echo.
    echo [错误] 没找到 AIcar.elf，请先在 CLion 里点一次“构建”。
    echo.
    pause
    exit /b 1
)

echo 正在烧录：!ELF!
echo.
"%PROG%" -c port=SWD freq=1000 -w "!ELF!" -v -rst

echo.
echo ==================================================
echo 若上面出现 Download verified successfully，即烧录成功。
echo 之后板上 PC13 那颗灯应快速闪烁（手柄未接时的正常提示）。
echo ==================================================
pause
