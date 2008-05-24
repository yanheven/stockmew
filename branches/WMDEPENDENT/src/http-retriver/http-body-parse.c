#include "http-body-parse.h"
#include "http-retriver.h"
#include "http-retriver-utils.h"
#include <stdlib.h>

int parse_body_string_sina(struct stock_price* price_info, char* string)
{
	struct string_element *string_array = NULL;
	struct string_element *pse = NULL;
	int sub_string_count = 0;
	char *substring = NULL;

	int i = 0;

	string_array = (struct string_element*)malloc(sizeof(struct string_element));
	string_array->string = NULL;
	string_array->next = NULL;

	// Split the string of body
	sub_string_count = split_string_by_char(string_array, string, ',');
	pse = string_array->next;
	for(i = 0; i < sub_string_count && pse != NULL; pse = pse->next, i++)
	{
		if (i == 3)
		{
			substring = pse->string;
			if (substring)
				price_info->current_price = atof(substring);
		}

		if (i == sub_string_count - 1)
		{
			substring = pse->string;
			if (substring)
			{
				int substrlen = strlen(substring);
				substrlen = substrlen - 3 < 8 ? substrlen - 3:8;
				//price_info->time_stamp = (char *)malloc(substrlen);
				memcpy(price_info->time_stamp, substring, substrlen);
				price_info->time_stamp[substrlen] = '\0';
			}
		}
	}

	while(string_array)
	{
		struct string_element *psa = string_array->next;
		if (string_array->string) free(string_array->string);
		free(string_array);
		string_array = psa;
	}

	return 1;
}
