#ifndef HTTP_BODY_PARSE
#define HTTP_BODY_PARSE

struct stock_price{
	double current_price;
	double heighest_price;
	double lowest_price;
	char time_stamp[9];
};


typedef int (*parse_body_string_fun)(struct stock_price* price_info, char* string);

int parse_body_string_sina(struct stock_price* price_info, char* string);

#endif /* HTTP_BODY_PARSE */
