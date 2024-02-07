if(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
  set(Detours ${CMAKE_CURRENT_LIST_DIR}/Detours-4.0.1/lib.X64/detours.lib)
else()
  set(YY_Thunks_for_WinXP ${CMAKE_CURRENT_LIST_DIR}/YY-Thunks-1.0.7-Binary/objs/X86/YY_Thunks_for_WinXP.obj)
  set(Detours ${CMAKE_CURRENT_LIST_DIR}/Detours-4.0.1/lib.X86/detours.lib)
endif()

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/minhook)

include_directories(${CMAKE_CURRENT_LIST_DIR})
include_directories(${CMAKE_CURRENT_LIST_DIR}/Detours-4.0.1/include)



if(${CMAKE_SIZEOF_VOID_P} EQUAL 4)
    set(LTLPlatform "Win32")
    set(SupportWinXP "true")
endif()
#https://github.com/Chuyu-Team/VC-LTL5
include("${CMAKE_CURRENT_LIST_DIR}/VC-LTL helper for cmake.cmake")