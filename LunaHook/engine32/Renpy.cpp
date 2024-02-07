#include"Renpy.h"

#include"python/python.h" 
	
bool Renpy::attach_function() {
    
    return InsertRenpyHook();
}  