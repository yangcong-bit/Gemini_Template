@echo off
:: 强制将控制台代码页切换为 UTF-8，解决乱码问题
chcp 65001 > nul
color 0A

echo ==================================================
echo   开始清理 Keil MDK 编译产生的中间垃圾文件...
echo ==================================================

:: 延时一会给用户看一眼
ping -n 2 127.0.0.1 > nul

:: 1. 删除编译生成的中间目标文件
del /Q /S *.o
del /Q /S *.d
del /Q /S *.crf
del /Q /S *.map
del /Q /S *.axf
del /Q /S *.htm
del /Q /S *.sct
del /Q /S *.dep
del /Q /S *.lnp
del /Q /S *.lst
del /Q /S *.obj
del /Q /S *.iex

:: 2. 删除 Keil 的用户界面布局缓存文件
del /Q /S *.uvoptx
del /Q /S *.uvgui.*
del /Q /S *.uvguix.*

:: 3. 删除调试产生的日志文件
del /Q /S JLinkLog.txt
del /Q /S *.bak
del /Q /S *.scvd

echo.
echo ==================================================
echo   清理完成！你的工程现在非常干净了。
echo ==================================================
pause