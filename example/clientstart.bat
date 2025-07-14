echo off
set path=%path%;%SVN_HOME%

@REM 设置脚本文件路径
set SCRIPTS_DIR=%cd%

@REM 设置bin文件路径
cd ../bin
set BIN_DIR=%cd%
cd

@REM 进入bin文件
cd /d %BIN_DIR%

@REM 执行脚本
DsLuaGame.exe ../example/client/main.lua

pause

