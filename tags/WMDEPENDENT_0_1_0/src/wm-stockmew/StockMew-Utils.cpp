#include "stdafx.h"
#include "StockMew-Utils.h"

bool verifydoubleexpress(char* string)
{
	register char* p = string;
	for ( ; *p != '\0'; p++)
		if (!isdigit(*p) && *p != '.')
			break;
	if (*p)
		return false;
	else
		return true;
}

bool verifystockcodeexpress(char* string)
{
	register char* p = string;
	register int i = 0;
	if (*p != '0' && *p != '6')
		return false;
	for (; *p != 0; p++, i++)
		if (!isdigit(*p))
			break;

	if (i != 6 || *p != 0)
		return false;
	return true;
}
