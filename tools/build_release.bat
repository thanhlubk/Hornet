@echo off
cd /d %~dp0..
rmdir /s /q temp
mkdir temp
cd temp

cmake .. ^
 -DCMAKE_PREFIX_PATH="C:/Qt/6.8.3/msvc2022_64/lib/cmake" ^
 -DCMAKE_BUILD_TYPE=Release

cmake --build . --config Release