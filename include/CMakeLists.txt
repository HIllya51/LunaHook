

add_library(pch hookcode.cpp common.cpp stringutils.cpp)
target_precompile_headers(pch PUBLIC common.h)
