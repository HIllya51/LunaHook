add_executable(LunaHost WIN32 controls.cpp main.cpp processlistwindow.cpp  LunaHost.cpp window.cpp luna.rc pluginmanager.cpp pluginexample.cpp)
target_precompile_headers(LunaHost REUSE_FROM pch) 
set_target_properties(LunaHost PROPERTIES OUTPUT_NAME "LunaHost${bitappendix}")

target_link_libraries(LunaHost pch host ${YY_Thunks_for_WinXP})
