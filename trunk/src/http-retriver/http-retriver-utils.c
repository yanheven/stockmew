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

#ifdef WINDOWS
#include <Winbase.h>
struct thread_data
{
  void (*fun) (void *);
  void *arg;
};

static DWORD WINAPI thread_helper (void *arg)
{
  struct thread_data *td = (struct thread_data *) arg;

  td->fun (td->arg);

  return 0;
}

int run_with_timeout(double seconds, void (*fun) (void *), void *arg)
{
	HANDLE thread_hnd;
	struct thread_data thread_arg;
	DWORD thread_id;
	int rc;

	if (seconds == 0)
	{
		fun (arg);
		return 0;
	}

	thread_arg.fun = fun;
	thread_arg.arg = arg;
	thread_hnd = CreateThread (NULL, 0, thread_helper, &thread_arg, 0, &thread_id);
	if (!thread_hnd)
	{
		fun (arg);
		return 0;
	}

	if (WaitForSingleObject (thread_hnd, (DWORD)(1000 * seconds)) == WAIT_OBJECT_0)
	{
		rc = 0;
	}
	else
	{
		TerminateThread (thread_hnd, 1);
		rc = 1;
	}

	CloseHandle (thread_hnd); 
	thread_hnd = NULL;
	return rc;
}
#else
int run_with_timeout(double seconds, void (*fun) (void *), void *arg)
{
	fun (arg);
	return 0;
}
#endif
