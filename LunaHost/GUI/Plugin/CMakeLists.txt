
add_library(ToClipboard MODULE pluginexample.cpp) 
target_precompile_headers(ToClipboard REUSE_FROM pch)
set_target_properties(ToClipboard PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/plugin${bitappendix}")