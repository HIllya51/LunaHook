cmake ../CMakeLists.txt -G "Visual Studio 17 2022" -A x64 -T host=x64 -B ../build/x64
cmake --build ../build/x64 --config Release --target ALL_BUILD -j 14

cmake -DLANGUAGE=Chinese ../CMakeLists.txt -G "Visual Studio 17 2022" -A x64 -T host=x64 -B ../build/x64_zh
cmake --build ../build/x64_zh --config Release --target ALL_BUILD -j 14
