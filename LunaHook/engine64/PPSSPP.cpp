#include"PPSSPP.h"
#include"ppsspp/psputils.hpp"

bool PPSSPP::attach_function()
{
	return InsertPPSSPPcommonhooks();
}
