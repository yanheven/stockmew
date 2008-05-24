#ifndef HTTP_RETRIVER_H
#define HTTP_RETRIVER_H

#include "http-body-parse.h"

int retrieve_stock_price(struct stock_price* price_info, char* url, parse_body_string_fun parse_body_string);

#endif /* HTTP_RETRIVER_H */
