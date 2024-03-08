
add_library(commonengine python/python2.cpp python/python3.cpp python/python.cpp pchooks/pchooks.cpp)
target_precompile_headers(commonengine REUSE_FROM pch)

