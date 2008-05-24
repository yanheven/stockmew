#ifndef UTILS_H
#define UTILS_H

struct string_element{
	char* string;
	struct string_element *next;
};

int split_string_by_char(struct string_element *string_array, char *string, char split_char);

int run_with_timeout(double seconds, void (*fun) (void *), void *arg);

#endif /* UTILS_H */
