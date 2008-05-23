#include "http-retriver-utils.h"
#include <stdlib.h>

int split_string_by_char(struct string_element *string_array, char *string, char split_char)
{
	int string_count;
	char *start;
	char *p;

	string_count = 0;
	p = start = string;
	while(*p!='\0')
	{
		for ( ; *p != split_char && *p != '\0'; p++);
		if (p != start)
		{
			struct string_element *pse = (struct string_element *)malloc(sizeof(struct string_element));
			char *pstr = (char *)malloc(p - start + 1);
			memcpy(pstr, start, p - start);
			pstr[p - start] = '\0';
			pse->string = pstr;
			pse->next = NULL;
			string_array->next = pse;
			string_array = pse;
			string_count++;
		}

		if (*p != '\0')
			start = ++p;
	}

	return string_count;
}
