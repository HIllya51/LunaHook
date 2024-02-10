cmake ../CMakeLists.txt -G "Visual Studio 17 2022" -A win32 -T host=x86 -B ../build/x86
cmake --build ../build/x86 --config Release --target ALL_BUILD -j 14

cmake -DLANGUAGE=Chinese ../CMakeLists.txt -G "Visual Studio 17 2022" -A win32 -T host=x86 -B ../build/x86_zh
cmake --build ../build/x86_zh --config Release --target ALL_BUILD -j 14