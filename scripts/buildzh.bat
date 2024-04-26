cmake -DBUILD_PLUGIN=OFF -DLANGUAGE=Chinese ../CMakeLists.txt -G "Visual Studio 17 2022" -A win32 -T host=x86 -B ../build/x86_zh
cmake --build ../build/x86_zh --config Release --target ALL_BUILD -j 14

cmake -DBUILD_PLUGIN=OFF -DLANGUAGE=Chinese ../CMakeLists.txt -G "Visual Studio 17 2022" -A x64 -T host=x64 -B ../build/x64_zh
cmake --build ../build/x64_zh --config Release --target ALL_BUILD -j 14
