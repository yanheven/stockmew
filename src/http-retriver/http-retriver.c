#include "http-retriver.h"
#include "http-retriver-utils.h"

#ifdef WINDOWS
#include <Winsock2.h>
#pragma comment(lib,"ws2.lib")
#pragma comment( lib, "Cellcore.lib" )
#define close(fd) closesocket (fd)
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#endif

#include <string.h>
#include <stdlib.h>


#ifndef MIN
#define MIN(i, j) ((i) <= (j) ? (i) : (j))
#endif

/* Categories.  */
enum {
	/* In C99 */
	_sch_isblank  = 0x0001,	/* space \t */
	_sch_iscntrl  = 0x0002,	/* nonprinting characters */
	_sch_isdigit  = 0x0004,	/* 0-9 */
	_sch_islower  = 0x0008,	/* a-z */
	_sch_isprint  = 0x0010,	/* any printing character including ' ' */
	_sch_ispunct  = 0x0020,	/* all punctuation */
	_sch_isspace  = 0x0040,	/* space \t \n \r \f \v */
	_sch_isupper  = 0x0080,	/* A-Z */
	_sch_isxdigit = 0x0100,	/* 0-9A-Fa-f */

	/* Extra categories useful to cpplib.  */
	_sch_isidst	= 0x0200,	/* A-Za-z_ */
	_sch_isvsp    = 0x0400,	/* \n \r */
	_sch_isnvsp   = 0x0800,	/* space \t \f \v \0 */

	/* Combinations of the above.  */
	_sch_isalpha  = _sch_isupper|_sch_islower,	/* A-Za-z */
	_sch_isalnum  = _sch_isalpha|_sch_isdigit,	/* A-Za-z0-9 */
	_sch_isidnum  = _sch_isidst|_sch_isdigit,	/* A-Za-z0-9_ */
	_sch_isgraph  = _sch_isalnum|_sch_ispunct,	/* isprint and not space */
	_sch_iscppsp  = _sch_isvsp|_sch_isnvsp	/* isspace + \0 */
};

/* Shorthand */
#define bl _sch_isblank
#define cn _sch_iscntrl
#define di _sch_isdigit
#define is _sch_isidst
#define lo _sch_islower
#define nv _sch_isnvsp
#define pn _sch_ispunct
#define pr _sch_isprint
#define sp _sch_isspace
#define up _sch_isupper
#define vs _sch_isvsp
#define xd _sch_isxdigit

/* Masks.  */
#define L  lo|is   |pr  /* lower case letter */
#define XL lo|is|xd|pr  /* lowercase hex digit */
#define U  up|is   |pr  /* upper case letter */
#define XU up|is|xd|pr  /* uppercase hex digit */
#define D  di   |xd|pr  /* decimal digit */
#define P  pn      |pr  /* punctuation */
#define _  pn|is   |pr  /* underscore */

#define C           cn  /* control character */
#define Z  nv      |cn  /* NUL */
#define M  nv|sp   |cn  /* cursor movement: \f \v */
#define V  vs|sp   |cn  /* vertical space: \r \n */
#define T  nv|sp|bl|cn  /* tab */
#define S  nv|sp|bl|pr  /* space */

const unsigned short _sch_istable[256] =
{
	Z,  C,  C,  C,   C,  C,  C,  C,   /* NUL SOH STX ETX  EOT ENQ ACK BEL */
	C,  T,  V,  M,   M,  V,  C,  C,   /* BS  HT  LF  VT   FF  CR  SO  SI  */
	C,  C,  C,  C,   C,  C,  C,  C,   /* DLE DC1 DC2 DC3  DC4 NAK SYN ETB */
	C,  C,  C,  C,   C,  C,  C,  C,   /* CAN EM  SUB ESC  FS  GS  RS  US  */
	S,  P,  P,  P,   P,  P,  P,  P,   /* SP  !   "   #    $   %   &   '   */
	P,  P,  P,  P,   P,  P,  P,  P,   /* (   )   *   +    ,   -   .   /   */
	D,  D,  D,  D,   D,  D,  D,  D,   /* 0   1   2   3    4   5   6   7   */
	D,  D,  P,  P,   P,  P,  P,  P,   /* 8   9   :   ;    <   =   >   ?   */
	P, XU, XU, XU,  XU, XU, XU,  U,   /* @   A   B   C    D   E   F   G   */
	U,  U,  U,  U,   U,  U,  U,  U,   /* H   I   J   K    L   M   N   O   */
	U,  U,  U,  U,   U,  U,  U,  U,   /* P   Q   R   S    T   U   V   W   */
	U,  U,  U,  P,   P,  P,  P,  _,   /* X   Y   Z   [    \   ]   ^   _   */
	P, XL, XL, XL,  XL, XL, XL,  L,   /* `   a   b   c    d   e   f   g   */
	L,  L,  L,  L,   L,  L,  L,  L,   /* h   i   j   k    l   m   n   o   */
	L,  L,  L,  L,   L,  L,  L,  L,   /* p   q   r   s    t   u   v   w   */
	L,  L,  L,  P,   P,  P,  P,  C,   /* x   y   z   {    |   }   ~   DEL */

	/* high half of unsigned char is locale-specific, so all tests are
	false in "C" locale */
	0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
	0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
	0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
	0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,

	0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
	0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
	0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
	0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
};
#define _sch_test(c, bit) (_sch_istable[(c) & 0xff] & (unsigned short)(bit))
#define ISDIGIT(c)  _sch_test(c, _sch_isdigit)
#define ISSPACE(c)  _sch_test(c, _sch_isspace)

struct ghbnwt_context {
	const char *host_name;
	struct hostent *hptr;
};

enum rp {
	rel_none, rel_name, rel_value, rel_both
};

struct request_header {
	char *name, *value;
	enum rp release_policy;
};

struct request {
	const char *method;
	char *arg;

	struct request_header  *headers;
	int hcount, hcapacity;
};

int url_parse (const char* url, char **host_name, char **full_path)
{
	char* scheme = "http://";
	char separator = '/';
	int scheme_len = strlen(scheme);
	const char* ps = NULL;
	const char* pe = NULL;
	int i;

	for (i = 0; i < scheme_len; i++)
	{
		if (url[i] != scheme[i]) 
			break;
	}

	if (i < scheme_len) return 0;
	ps = url;
	for (i = 0; i < scheme_len; i++,ps++);
	pe = strchr(ps, separator);

	*host_name = (char *)malloc(pe - ps + 1);
	memcpy(*host_name, ps, (pe - ps));
	(*host_name)[pe - ps] = '\0';

	*full_path = (char *)malloc(strlen(url) -(pe - ps) +1);
	memcpy(*full_path, pe, (strlen(url) -(pe - ps)));
	(*full_path)[strlen(url) -(pe - ps)] = '\0';

	return 1;
}

static void
release_header (struct request_header *hdr)
{
	switch (hdr->release_policy)
	{
	case rel_none:
		break;
	case rel_name:
		free (hdr->name);
		break;
	case rel_value:
		free (hdr->value);
		break;
	case rel_both:
		free (hdr->name);
		free (hdr->value);
		break;
	}
}

static void
request_set_header (struct request *req, char *name, char *value,
					enum rp release_policy)
{
	struct request_header *hdr;
	int i;

	if (!value)
	{
		/* A NULL value is a no-op; if freeing the name is requested,
		free it now to avoid leaks.  */
		if (release_policy == rel_name || release_policy == rel_both)
			free (name);
		return;
	}

	for (i = 0; i < req->hcount; i++)
	{
		hdr = &req->headers[i];
		if (0 == strcmp (name, hdr->name))
		{
			/* Replace existing header. */
			release_header (hdr);
			hdr->name = name;
			hdr->value = value;
			hdr->release_policy = release_policy;
			return;
		}
	}

	/* Install new header. */

	if (req->hcount >= req->hcapacity)
	{
		req->hcapacity <<= 1;
		req->headers = (struct request_header*)realloc(req->headers, req->hcapacity * sizeof (*hdr));
	}
	hdr = &req->headers[req->hcount++];
	hdr->name = name;
	hdr->value = value;
	hdr->release_policy = release_policy;
}

static struct request *prepare_request(char *host_name, char *full_path)
{
	const char *meth = "GET";
	struct request *req = NULL;

	req = (struct request *)malloc(sizeof(struct request));

	// Set method
	req->method = meth;
	req->arg = full_path;

	// Initialize header parameter
	req->hcapacity = 8;
	req->headers = (struct request_header*)malloc (sizeof(struct request_header*)*req->hcapacity);
	req->hcount = 0;

	//	request_set_header (req, "Referer", (char *) hs->referer, rel_none);
	request_set_header (req, "Accept", "*/*", rel_none);
	request_set_header (req, "Host", host_name, rel_value);
	request_set_header (req, "Connection", "Keep-Alive", rel_none);

	return req;
}


char *organize_request_string(struct request * req)
{
	char *request_string = NULL;
	char *p = NULL;
	int i, size;

	/* Count the request size. */
	size = 0;

	/* METHOD " " ARG " " "HTTP/1.0" "\r\n" */
	size += strlen (req->method) + 1 + strlen (req->arg) + 1 + 8 + 2;

	for (i = 0; i < req->hcount; i++)
	{
		struct request_header *hdr = &req->headers[i];
		/* NAME ": " VALUE "\r\n" */
		size += strlen (hdr->name) + 2 + strlen (hdr->value) + 2;
	}

	/* "\r\n\0" */
	size += 3;

	p = request_string = (char*)malloc(sizeof(char)*size);

	memcpy (p, req->method, strlen (req->method));
	p += strlen(req->method);
	*p++ = ' ';

	memcpy (p, req->arg, strlen (req->arg));
	p += strlen(req->arg);
	*p++ = ' ';

	memcpy (p, "HTTP/1.0\r\n", 10); p += 10;

	for (i = 0; i < req->hcount; i++)
	{
		struct request_header *hdr = &req->headers[i];
		memcpy (p, hdr->name, strlen (hdr->name));
		p += strlen(hdr->name);
		*p++ = ':', *p++ = ' ';
		memcpy (p, hdr->value, strlen (hdr->value));
		p += strlen(hdr->value);
		*p++ = '\r', *p++ = '\n';
	}

	*p++ = '\r', *p++ = '\n', *p++ = '\0';

	return request_string;
}

static const char * response_head_terminator (const char *start, const char *peeked, int peeklen)
{
	const char *p, *end;

	/* If at first peek, verify whether HUNK starts with "HTTP".  If
	not, this is a HTTP/0.9 request and we must bail out without
	reading anything.  */
	if (start == peeked && 0 != memcmp (start, "HTTP", MIN (peeklen, 4)))
		return start;

	/* Look for "\n[\r]\n", and return the following position if found.
	Start two chars before the current to cover the possibility that
	part of the terminator (e.g. "\n\r") arrived in the previous
	batch.  */
	p = peeked - start < 2 ? start : peeked - 2;
	end = peeked + peeklen;

	/* Check for \n\r\n or \n\n anywhere in [p, end-2). */
	for (; p < end - 2; p++)
		if (*p == '\n')
		{
			if (p[1] == '\r' && p[2] == '\n')
				return p + 3;
			else if (p[1] == '\n')
				return p + 2;
		}
		/* p==end-2: check for \n\n directly preceding END. */
		if (p[0] == '\n' && p[1] == '\n')
			return p + 2;

		return NULL;
}

struct recv_arg
{
	int s;
	char* buf;
	int len;
	int flags;
	int res;
};

void recv_with_timeout_callback(void* arg)
{
	struct recv_arg* rvarg = (struct recv_arg*) arg;
	rvarg->res = recv(rvarg->s, rvarg->buf, rvarg->len, rvarg->flags);
}

char* receive_response(int sock, char** body, int* body_size)
{
	long bufsize = 512;
	char *buf = (char*)malloc (bufsize);
	int tail = 0;
	*body_size = 0;

	while (1)
	{
		const char *end;
		int remain;
		struct recv_arg rvarg;

		memset(&rvarg, 0, sizeof(struct recv_arg));
		rvarg.s = sock;
		rvarg.buf = buf + tail;
		rvarg.len = bufsize - 1 - tail;
		rvarg.flags = 0;
		/* First, peek at the available data. */

		if (run_with_timeout(30, recv_with_timeout_callback, (void*)&rvarg))
		{
			free (buf);
			return NULL;
		}
		else if (rvarg.res < 0)
		{
			free (buf);
			return NULL;
		}

		end = response_head_terminator(buf, buf + tail, rvarg.res);
		if (end)
		{
			/* The data contains the terminator: we'll drain the data up
			to the end of the terminator.  */
			remain = end - (buf + tail);
			*body = (char*)malloc(rvarg.res - remain + 1);
			memcpy(*body, end, rvarg.res - remain);
			(*body)[rvarg.res - remain] = '\0';
			*body_size = rvarg.res - remain;

			if (remain == 0)
			{
				/* No more data needs to be read. */
				buf[tail] = '\0';
				return buf;
			}
			if (bufsize - 1 < tail + remain)
			{
				bufsize = tail + remain + 1;
				buf = (char*)realloc(buf, bufsize);
				buf[tail] = '\0';
				return buf;
			}
		}

		tail += rvarg.res;
		buf[tail] = '\0';

		if (rvarg.res == 0)
		{
			if (tail == 0)
			{
				/* EOF without anything having been read */
				free (buf);
				/*errno = 0;*/
				return NULL;
			}
			else
				/* EOF seen: return the data we've read. */
				return buf;
		}

		/* Keep looping until all the data arrives. */
		if (tail == bufsize - 1)
		{
			bufsize <<= 1;
			buf = (char*)realloc (buf, bufsize);
		}
	}
}

int parse_response(char* response_string)
{
	const char *p, *end;
	int count = 0;
	int status = -1;

	if (!response_string)
		return -1;
	if (*response_string == '\0')
		return 200;

	p = response_string;

	while (1)
	{
		/* Break upon encountering an empty line. */
		if (!p[0] || (p[0] == '\r' && p[1] == '\n') || p[0] == '\n')
			break;

		count++;
		/* Find the end of HDR, including continuations. */
		do
		{
			end = strchr (p, '\n');
			if (end)
				p = end + 1;
			else
				p += strlen (p);
		}
		while (*p == ' ' || *p == '\t');
	}
	if (count == 0)
		return -1;

	if (!end)
		return -1;

	p = response_string;
	/* "HTTP" */
	if (end - p < 4 || 0 != strncmp (p, "HTTP", 4))
		return -1;
	p += 4;

	/* Match the HTTP version.  This is optional because Gnutella
	servers have been reported to not specify HTTP version.  */
	if (p < end && *p == '/')
	{
		++p;
		while (p < end && ISDIGIT (*p))
			++p;
		if (p < end && *p == '.')
			++p; 
		while (p < end && ISDIGIT (*p))
			++p;
	}

	while (p < end && ISSPACE (*p))
		++p;
	if (end - p < 3 || !ISDIGIT (p[0]) || !ISDIGIT (p[1]) || !ISDIGIT (p[2]))
		return -1;

	status = 100 * (p[0] - '0') + 10 * (p[1] - '0') + (p[2] - '0');
	return status;
}

char* read_body(int sock, char* buf, int bufsize)
{
	struct recv_arg rvarg;
	int sum_read;
	int empty_buf_start;
	int granularity;

	granularity = 1024;
	empty_buf_start = 0;
	sum_read = 0;

	if (buf)
	{
		buf = (char*)realloc(buf, granularity + bufsize);
		sum_read = empty_buf_start = bufsize;
	}
	else
		buf = (char*)malloc(granularity);
	bufsize += granularity;

	do
	{
		memset(&rvarg, 0, sizeof(struct recv_arg));
		rvarg.s = sock;
		rvarg.buf = buf + empty_buf_start;
		rvarg.len = bufsize - sum_read;
		rvarg.flags = 0;
		/* First, peek at the available data. */

		if (run_with_timeout(30, recv_with_timeout_callback, (void*)&rvarg))
		{
			free(buf);
			return NULL;
		}

		if (rvarg.res > 0)
		{
			sum_read += rvarg.res;
			empty_buf_start += rvarg.res;
		}
		if (sum_read >= bufsize)
		{
			buf = (char*)realloc (buf, bufsize + granularity);
			bufsize += granularity;
		}
	}
	while (rvarg.res > 0);

	if (sum_read == bufsize)
		buf = (char*)realloc (buf, bufsize + 1);
	buf[sum_read]='\0';

	if (rvarg.res == -1)
	{
		free(buf);
		return NULL;
	}
		
	return buf;
}

int retrieve_stock_price(struct stock_price* price_info, char* url, parse_body_string_fun parse_body_string)
{
	int nret = 0;
	struct request * request = NULL;
	char *host_name = NULL;
	char *full_path = NULL;

	char *request_string = NULL;
	char *response_string = NULL;
	char *body_string = NULL;
	int body_size;
	char *p = NULL;

	struct ghbnwt_context *ctx = NULL;
	struct sockaddr_in sa;

	int res;
	int bufsize;

	int sock;

	url_parse (url, &host_name, &full_path);
	request = prepare_request(host_name, full_path);
	nret = 1;

	// Get server's ip adress.
	ctx =(struct ghbnwt_context *) malloc(sizeof(struct ghbnwt_context));
	ctx->host_name = host_name;
	ctx->hptr = gethostbyname (ctx->host_name);
	if (!ctx->hptr)
	{
		nret = -1;
		goto exit;
	}

	// Get socket descriptor.
	sock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Connect to server.
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons (80);
	memcpy (&sa.sin_addr, *ctx->hptr->h_addr_list, 4);
	if(-1 == connect (sock, (const struct sockaddr *)&sa, sizeof(sa)))
	{
		nret = -1;
		goto exit;
	}

	// Send request.
	p = request_string = organize_request_string(request);
	bufsize = strlen(request_string);
	res = 0;
	while (bufsize > 0)
	{
		res = send(sock, p, bufsize, 0);
		if (res <= 0)
			break;
		p += res;
		bufsize -= res;
	}

	// Receive response.
	response_string = receive_response(sock, &body_string, &body_size);
	if (response_string == NULL)
	{
		nret = -1;
	}
	else if (parse_response(response_string) == 200)
	{
		body_string = read_body(sock, body_string, body_size);
		if (body_string)
		{
			// Parse string of body
			if (body_string)
				parse_body_string(price_info, body_string);
		}
	}
	else
	{
		nret = -1;
	}

exit:
	// Close a connection.
	close(sock);

	// Reclaim heap memory space.
	free(host_name);
	free(full_path);
	free(request->headers);
	free(request);
	free(request_string);
	free(response_string);
	free(body_string);

	return nret;
}
