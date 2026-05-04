@echo off
set filename=%1

:: 使用 for 循环提取父目录
for %%F in ("%filename%") do set dirname=%%~dpF

:: 输出结果
echo %dirname%