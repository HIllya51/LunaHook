add_library(host
	host.cpp
	textthread.cpp
)
target_precompile_headers(host REUSE_FROM pch)

include_directories(.)

add_executable(LunaHostCLI LunaHostCLI.cpp)
target_precompile_headers(LunaHostCLI REUSE_FROM pch)

add_library(LunaHostDll MODULE LunaHostDll.cpp)
target_precompile_headers(LunaHostDll REUSE_FROM pch)

set_target_properties(LunaHostCLI PROPERTIES OUTPUT_NAME "LunaHostCLI${bitappendix}")
set_target_properties(LunaHostDll PROPERTIES OUTPUT_NAME "LunaHost${bitappendix}")
target_link_libraries(LunaHostCLI pch host ${YY_Thunks_for_WinXP})
target_link_libraries(LunaHostDll pch host ${YY_Thunks_for_WinXP})
add_subdirectory(GUI)