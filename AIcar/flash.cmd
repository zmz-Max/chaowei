@echo off
chcp 936 >nul
setlocal
set "CLT=D:\STM32CubeCLT\STM32CubeCLT_1.22.0"
set "PROG=%CLT%\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe"
set "ELF=%~dp0cmake-build-debug-stm32-1\AIcar.elf"
if not exist "%ELF%" set "ELF=%~dp0cmake-build-debug-stm32\AIcar.elf"
if not exist "%ELF%" set "ELF=%~dp0build\Debug\AIcar.elf"
if not exist "%ELF%" goto nofile
echo.
echo [flash] ELF = %ELF%
echo.
"%PROG%" -c port=SWD freq=1000 -w "%ELF%" -v -rst
echo.
echo ==================================================
echo 出现 Download verified successfully 即烧录成功。
echo 烧完后 PC13 灯应快速闪烁（手柄未接时的正常提示）。
echo ==================================================
goto end
:nofile
echo.
echo [错误] 没找到 AIcar.elf，请先在 CLion 里点一次“构建”。
:end
echo.
pause
