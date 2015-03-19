#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
// #include <libgen.h>
#include <unistd.h>
#include <signal.h>
#include <string>
#include <assert.h>
#include <sys/wait.h>

#include "inferutil.h"

#include <libIRDB-core.hpp>

using namespace std;
using namespace libIRDB;

#define BOGUS_VALUE        2000000000
#define BOGUS_VALUE_2     -2

uintptr_t malloc_address = 0L;

int success = 0;

#define EXIT_CODE_TEST_SUCCESS 0
#define EXIT_CODE_TEST_FAILURE 1
#define EXIT_CODE_TEST_INVALID 2


static void send_request(int fd, struct request *req)
{
	int bytes_written;

	bytes_written = write(fd, req, sizeof(struct request));
}

static void send_quit_command(int fd)
{
	struct request req;
	clear_request(&req);
	req.command = CMD_QUIT;
	send_request(fd, &req);
}

static void get_response(int fd, struct response *res)
{
	int bytes_read;

	bytes_read = read(fd, res, sizeof(struct response));
//	cout << "get_response(): bytes_read = " << bytes_read << endl;
}

static void set_argument_int(struct argument *arg, int val)
{
	if (arg) {
		arg->type = ARG_INT;
		arg->val.num = val;
	}
}

static void set_argument_ptr(struct argument *arg, const uintptr_t address)
{
	if (arg) {
		arg->type = ARG_PTR;
		arg->val.addr = address;
	}
}

static void set_argument_str(struct argument *arg, char *str)
{
	if (arg) {
		arg->type = ARG_BYTES;
		arg->val.bytes.num_bytes = strlen(str)+1;
		strcpy(arg->val.bytes.bytes, str);
	}
}

int call_proto_i_pbb(uintptr_t fn, uintptr_t rptr, int bogus1, int bogus2, int *ok)
{
	struct request req;
	struct response res;

	clear_request(&req);
	clear_response(&res);

	req.command = CMD_CALL;
	req.num_args = 4;
	set_argument_ptr(&req.arg1, fn);
	set_argument_ptr(&req.arg2, rptr);
	set_argument_int(&req.arg3, bogus1);
	set_argument_int(&req.arg4, bogus2);
	req.outarg_type = ARG_INT;

	send_request(CINDERELLA_DRIVER_WRITE, &req);
	get_response(CINDERELLA_DRIVER_READ, &res);

	*ok = res.ok;
	return res.ok ? res.outarg.val.addr : (uintptr_t) NULL;
}

int call_proto_i_p(uintptr_t fn, uintptr_t rptr, int *ok)
{
	return call_proto_i_pbb(fn, rptr, BOGUS_VALUE, BOGUS_VALUE_2, ok);
}

int call_proto_i_ppb(uintptr_t fn, uintptr_t rptr1, uintptr_t rptr2, int bogus, int *ok)
{
	struct request req;
	struct response res;

	clear_request(&req);
	clear_response(&res);

	req.command = CMD_CALL;
	req.num_args = 4;
	set_argument_ptr(&req.arg1, fn);
	set_argument_ptr(&req.arg2, rptr1);
	set_argument_ptr(&req.arg3, rptr2);
	set_argument_int(&req.arg4, bogus);
	req.outarg_type = ARG_PTR;

	send_request(CINDERELLA_DRIVER_WRITE, &req);
	get_response(CINDERELLA_DRIVER_READ, &res);

	*ok = res.ok;
	return res.ok ? res.outarg.val.addr : (uintptr_t) NULL;
}

int call_proto_i_pp(uintptr_t fn, uintptr_t rptr1, uintptr_t rptr2, int *ok)
{
	return call_proto_i_ppb(fn, rptr1, rptr2, BOGUS_VALUE, ok);
}

uintptr_t call_proto_p_p(uintptr_t fn, uintptr_t p, int *ok)
{
	struct request req;
	struct response res;

	clear_request(&req);
	clear_response(&res);

	req.command = CMD_CALL;
	req.num_args = 4;
	set_argument_ptr(&req.arg1, fn);
	set_argument_ptr(&req.arg2, p);
	set_argument_int(&req.arg3, BOGUS_VALUE);
	set_argument_int(&req.arg4, BOGUS_VALUE);
	req.outarg_type = ARG_PTR;

	send_request(CINDERELLA_DRIVER_WRITE, &req);
	get_response(CINDERELLA_DRIVER_READ, &res);

	*ok = res.ok;
	return res.ok ? res.outarg.val.addr : (uintptr_t) NULL;
}

uintptr_t call_proto_p_s(uintptr_t fn, char *str, int *ok)
{
	struct request req;
	struct response res;

	clear_request(&req);
	clear_response(&res);

	req.command = CMD_CALL;
	req.num_args = 4;
	set_argument_ptr(&req.arg1, fn);
	set_argument_str(&req.arg2, str);
	set_argument_int(&req.arg3, BOGUS_VALUE);
	set_argument_int(&req.arg4, BOGUS_VALUE);
	req.outarg_type = ARG_PTR;

	send_request(CINDERELLA_DRIVER_WRITE, &req);
	get_response(CINDERELLA_DRIVER_READ, &res);

	*ok = res.ok;
	return res.ok ? res.outarg.val.addr : (uintptr_t) NULL;
}

uintptr_t call_proto_p_si(uintptr_t fn, char *str, int i, int *ok)
{
	struct request req;
	struct response res;

	clear_request(&req);
	clear_response(&res);

	req.command = CMD_CALL;
	req.num_args = 4;
	set_argument_ptr(&req.arg1, fn);
	set_argument_str(&req.arg2, str);
	set_argument_int(&req.arg3, i);
	set_argument_int(&req.arg4, BOGUS_VALUE);
	req.outarg_type = ARG_PTR;

	send_request(CINDERELLA_DRIVER_WRITE, &req);
	get_response(CINDERELLA_DRIVER_READ, &res);

	*ok = res.ok;
	return res.ok ? res.outarg.val.addr : (uintptr_t) NULL;
}

uintptr_t call_proto_p_pp(uintptr_t fn, uintptr_t p1, uintptr_t p2, int *ok)
{
	struct request req;
	struct response res;

	clear_request(&req);
	clear_response(&res);

	req.command = CMD_CALL;
	req.num_args = 4;
	set_argument_ptr(&req.arg1, fn);
	set_argument_ptr(&req.arg2, p1);
	set_argument_ptr(&req.arg3, p2);
	set_argument_int(&req.arg4, BOGUS_VALUE);
	req.outarg_type = ARG_PTR;

	send_request(CINDERELLA_DRIVER_WRITE, &req);
	get_response(CINDERELLA_DRIVER_READ, &res);

	*ok = res.ok;
	return res.ok ? res.outarg.val.addr : (uintptr_t) NULL;
}

uintptr_t call_proto_p_i(uintptr_t fn, int i, int *ok)
{
	struct request req;
	struct response res;

	clear_request(&req);
	clear_response(&res);

	req.command = CMD_CALL;
	req.num_args = 4;
	set_argument_ptr(&req.arg1, fn);
	set_argument_int(&req.arg2, i);
	set_argument_int(&req.arg3, BOGUS_VALUE);
	set_argument_int(&req.arg4, BOGUS_VALUE_2);
	req.outarg_type = ARG_PTR;

	send_request(CINDERELLA_DRIVER_WRITE, &req);
	get_response(CINDERELLA_DRIVER_READ, &res);

	*ok = res.ok;
	return res.ok ? res.outarg.val.addr : (uintptr_t) NULL;
}

uintptr_t call_proto_p_ii(uintptr_t fn, int i1, int i2, int *ok)
{
	struct request req;
	struct response res;

	clear_request(&req);
	clear_response(&res);

	req.command = CMD_CALL;
	req.num_args = 4;
	set_argument_ptr(&req.arg1, fn);
	set_argument_int(&req.arg2, i1);
	set_argument_int(&req.arg3, i2);
	set_argument_int(&req.arg4, BOGUS_VALUE);
	req.outarg_type = ARG_PTR;

	send_request(CINDERELLA_DRIVER_WRITE, &req);
	get_response(CINDERELLA_DRIVER_READ, &res);

	*ok = res.ok;
	return res.ok ? res.outarg.val.addr : (uintptr_t) NULL;
}

int call_proto_i_ppi(uintptr_t fn, uintptr_t rptr1, uintptr_t rptr2, int i, int *ok)
{
	struct request req;
	struct response res;

	clear_request(&req);
	clear_response(&res);

	req.command = CMD_CALL;
	req.num_args = 4;
	set_argument_ptr(&req.arg1, fn);
	set_argument_ptr(&req.arg2, rptr1);
	set_argument_ptr(&req.arg3, rptr2);
	set_argument_int(&req.arg4, i);
	req.outarg_type = ARG_PTR;

	send_request(CINDERELLA_DRIVER_WRITE, &req);
	get_response(CINDERELLA_DRIVER_READ, &res);

	*ok = res.ok;
	return res.ok ? res.outarg.val.addr : (uintptr_t) NULL;
}

uintptr_t call_proto_p_pib(uintptr_t fn, uintptr_t ptr, int i, int bogus, int *ok)
{
	struct request req;
	struct response res;
	clear_request(&req);
	clear_response(&res);

	req.command = CMD_CALL;
	req.num_args = 4;
	set_argument_ptr(&req.arg1, fn);
	set_argument_ptr(&req.arg2, ptr);
	set_argument_int(&req.arg3, i);
	set_argument_int(&req.arg4, bogus); // bogus value on purpose
	req.outarg_type = ARG_PTR;

	send_request(CINDERELLA_DRIVER_WRITE, &req);
	get_response(CINDERELLA_DRIVER_READ, &res);

	*ok = res.ok;
	return res.ok ? res.outarg.val.addr : (uintptr_t) NULL;
}

uintptr_t call_proto_p_pi(uintptr_t fn, uintptr_t ptr, int i, int *ok)
{
	return call_proto_p_pib(fn, ptr, i, BOGUS_VALUE, ok);
}

uintptr_t call_proto_p_pii(uintptr_t fn, uintptr_t ptr, int i1, int i2, int *ok)
{
	struct request req;
	struct response res;
	clear_request(&req);
	clear_response(&res);

	req.command = CMD_CALL;
	req.num_args = 4;
	set_argument_ptr(&req.arg1, fn);
	set_argument_ptr(&req.arg2, ptr);
	set_argument_int(&req.arg3, i1);
	set_argument_int(&req.arg4, i2);
	req.outarg_type = ARG_PTR;

	send_request(CINDERELLA_DRIVER_WRITE, &req);
	get_response(CINDERELLA_DRIVER_READ, &res);

	*ok = res.ok;
	return res.ok ? res.outarg.val.addr : (uintptr_t) NULL;
}

int find_free(const int infd, const int outfd, const uintptr_t malloc_address, const uintptr_t fn)
{
	struct request req;
	struct response res;
	uintptr_t newaddress = 0L;

	clear_request(&req);
	clear_response(&res);

	req.command = CMD_CALL;
	req.num_args = 2;
	set_argument_ptr(&req.arg1, malloc_address);
	set_argument_int(&req.arg2, 20000);
	req.outarg_type = ARG_PTR;

	send_request(outfd, &req);
	get_response(infd, &res);
	
	if (res.ok)
	{
		newaddress = res.outarg.val.addr;
		printf("malloc(20000) returned %p\n", newaddress);
	}
	else
	{
		fprintf(stderr, "something went wrong with malloc call\n");
		return 0;
	}

	printf("find_free(): now call maybe free: %p with arg: %p\n", fn, newaddress);
	clear_request(&req);
	clear_response(&res);

	req.command = CMD_CALL;
	req.num_args = 2;
	set_argument_ptr(&req.arg1, fn);
	set_argument_ptr(&req.arg2, newaddress);
	req.outarg_type = ARG_NONE;

	printf("find_free(): send_request\n");
	send_request(outfd, &req);
	printf("find_free(): get_response\n");
	get_response(infd, &res);

	printf("find_free(): result = %d\n", res.ok);
	return res.ok;
}

uintptr_t call_memset(uintptr_t maybe_memset, uintptr_t ptr, int c, int size, int *ok)
{
	return call_proto_p_pii(maybe_memset, ptr, c, size, ok);
}

uintptr_t call_memchr(uintptr_t maybe_memchr, uintptr_t ptr, int c, int size, int *ok)
{
	return call_proto_p_pii(maybe_memchr, ptr, c, size, ok);
}

uintptr_t call_strchr(uintptr_t maybe_strchr, uintptr_t ptr, int c, int *ok)
{
	uintptr_t p1 = call_proto_p_pib(maybe_strchr, ptr, c, 0, ok);
	assert(*ok);
	uintptr_t p2 = call_proto_p_pib(maybe_strchr, ptr, c, -1, ok);
	assert(*ok);
	uintptr_t p3 = call_proto_p_pib(maybe_strchr, ptr, c, BOGUS_VALUE, ok);
	assert(*ok);

	assert(p1 == p2);
	assert(p2 == p3);
	
	return p1;
}

uintptr_t call_strrchr(uintptr_t maybe_strrchr, uintptr_t ptr, int c, int *ok)
{
	uintptr_t p1 = call_proto_p_pib(maybe_strrchr, ptr, c, 0, ok);
	assert(*ok);
	uintptr_t p2 = call_proto_p_pib(maybe_strrchr, ptr, c, -1, ok);
	assert(*ok);
	uintptr_t p3 = call_proto_p_pib(maybe_strrchr, ptr, c, BOGUS_VALUE, ok);
	assert(*ok);

	assert(p1 == p2);
	assert(p2 == p3);
	
	return p1;
}

uintptr_t call_malloc(uintptr_t maybe_malloc, int size, int *ok)
{
	return call_proto_p_i(maybe_malloc, size, ok);
}

uintptr_t call_calloc(uintptr_t maybe_calloc, int count, int size, int *ok)
{
	return call_proto_p_ii(maybe_calloc, count, size, ok);
}

//  char* strdup(const char *s1);
uintptr_t call_strdup(uintptr_t maybe_strdup, char *str, int *ok)
{
	return call_proto_p_s(maybe_strdup, str, ok);
}

//  char* strndup(const char *s1, size);
uintptr_t call_strndup(uintptr_t maybe_strdup, char *str, int size, int *ok)
{
	return call_proto_p_si(maybe_strdup, str, size, ok);
}

int call_strlen(uintptr_t maybe_strlen, uintptr_t ptr, int *ok)
{
	int i1 = call_proto_i_pbb(maybe_strlen, ptr, 2, BOGUS_VALUE, ok);
	assert(*ok);
	int i2 = call_proto_i_pbb(maybe_strlen, ptr, BOGUS_VALUE, -5, ok);
	assert(*ok);
	int i3 = call_proto_i_pbb(maybe_strlen, ptr, -2, BOGUS_VALUE, ok);
	assert(*ok);

	assert (i1 == i2);
	assert (i2 == i3);

	return i1;
}

uintptr_t call_strtok(uintptr_t maybe_strtok, uintptr_t str, uintptr_t sep, int *ok)
{
	return call_proto_p_pp(maybe_strtok, str, sep, ok);
}

int call_strcmp(uintptr_t maybe_strcmp, uintptr_t s1, uintptr_t s2, int *ok)
{
	int r1 = call_proto_i_ppb(maybe_strcmp, s1, s2, 2, ok);
	assert(*ok);
	int r2 = call_proto_i_ppb(maybe_strcmp, s1, s2, 200000, ok);
	assert(*ok);
	int r3 = call_proto_i_ppb(maybe_strcmp, s1, s2, -2, ok);
	assert(*ok);
	assert (r1==r2);
	assert (r2==r3);
	return r1;
}

int call_strncmp(uintptr_t maybe_strncmp, uintptr_t s1, uintptr_t s2, int n, int *ok)
{
	return call_proto_i_ppi(maybe_strncmp, s1, s2, n, ok);
}

int call_memcpy(uintptr_t maybe_memcpy, uintptr_t dst, uintptr_t src, int size, int *ok)
{
	return call_proto_i_ppi(maybe_memcpy, dst, src, size, ok);
}

int call_strlcpy(uintptr_t maybe_strlcpy, uintptr_t dst, uintptr_t src, int size, int *ok)
{
	return call_proto_i_ppi(maybe_strlcpy, dst, src, size, ok);
}

int call_strlcat(uintptr_t maybe_strlcat, uintptr_t dst, uintptr_t src, int size, int *ok)
{
	return call_proto_i_ppi(maybe_strlcat, dst, src, size, ok);
}

int call_strncat(uintptr_t maybe_strncat, uintptr_t dst, uintptr_t src, int size, int *ok)
{
	return call_proto_i_ppi(maybe_strncat, dst, src, size, ok);
}

int call_strspn(uintptr_t maybe_strspn, uintptr_t s1, uintptr_t s2, int *ok)
{
	int i1 = call_proto_i_ppb(maybe_strspn, s1, s2, 0, ok);
	assert (*ok);
	int i2 = call_proto_i_ppb(maybe_strspn, s1, s2, -2, ok);
	assert (*ok);
	int i3 = call_proto_i_ppb(maybe_strspn, s1, s2, BOGUS_VALUE, ok);
	assert (*ok);

	assert(i1 == i2);
	assert(i2 == i3);

	return i1;
}

int call_strcspn(uintptr_t maybe_strcspn, uintptr_t s1, uintptr_t s2, int *ok)
{
	int i1 = call_proto_i_ppb(maybe_strcspn, s1, s2, 0, ok);
	assert (*ok);
	int i2 = call_proto_i_ppb(maybe_strcspn, s1, s2, -2, ok);
	assert (*ok);
	int i3 = call_proto_i_ppb(maybe_strcspn, s1, s2, BOGUS_VALUE, ok);
	assert (*ok);

	assert(i1 == i2);
	assert(i2 == i3);

	return i1;
}

// void* realloc(void *ptr, size_t size);
uintptr_t call_realloc(uintptr_t maybe_realloc, uintptr_t ptr, int size, int *ok)
{
	return call_proto_p_pi(maybe_realloc, ptr, size, ok);
}

void call_read(char *buf, uintptr_t addr, int numbytes, int *ok)
{
	struct request req;
	struct response res;

	clear_request(&req);
	clear_response(&res);

	req.command = CMD_READ;
	req.num_args = 4;
	set_argument_ptr(&req.arg1, addr);
	set_argument_int(&req.arg2, numbytes);
	set_argument_int(&req.arg3, -50000000); // bogus on purpose
	set_argument_int(&req.arg3, 20000000); // bogus on purpose
	req.outarg_type = ARG_BYTES;

	send_request(CINDERELLA_DRIVER_WRITE, &req);
	get_response(CINDERELLA_DRIVER_READ, &res);

	*ok = res.ok;
	if (*ok)
		memcpy(buf, res.outarg.val.bytes.bytes, numbytes);
}

uintptr_t call_allocate(int size, int init_value, int *ok)
{
	struct request req;
	struct response res;

	clear_request(&req);
	clear_response(&res);

	req.command = CMD_ALLOC;
	req.num_args = 2;
	set_argument_int(&req.arg1, size);
	set_argument_int(&req.arg2, init_value);
	req.outarg_type = ARG_PTR;

	send_request(CINDERELLA_DRIVER_WRITE, &req);
	get_response(CINDERELLA_DRIVER_READ, &res);

	*ok = res.ok;
	return res.ok ? res.outarg.val.addr : (uintptr_t) NULL;
}

void call_write(const uintptr_t address, char *str, int *ok)
{
	struct request req;
	struct response res;

	clear_request(&req);
	clear_response(&res);

	req.command = CMD_WRITE;
	req.num_args = 2;
	set_argument_ptr(&req.arg1, address);
	set_argument_str(&req.arg2, str);
	req.outarg_type = ARG_NONE;

	send_request(CINDERELLA_DRIVER_WRITE, &req);
	get_response(CINDERELLA_DRIVER_READ, &res);

	*ok = res.ok;
}

int test_for_strlen(const uintptr_t maybe_strlen)
{
	char buf[1024];
	int ok;
	int len = 0;
	uintptr_t ptr = call_allocate(1000, 28, &ok);
	if (!ok) return 0;

	call_write(ptr, "h", &ok);

	len = call_strlen(maybe_strlen, ptr, &ok);
	assert(len == 1);

	call_write(ptr, "the quick brown fox", &ok);
	len = call_strlen(maybe_strlen, ptr, &ok);
	assert(len == strlen("the quick brown fox"));

	char *str = "hello %s %d %c zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz";
	call_write(ptr, str, &ok);
	len = call_strlen(maybe_strlen, ptr, &ok);
	assert(len == strlen(str));

	int len5 = call_strlen(maybe_strlen, ptr+5, &ok);
	assert(len == (len5 + 5));

	return 1;
}

int test_for_strdup(const uintptr_t maybe_strdup)
{
	char buf[2048];
	char buf2[2048];
	char *str  = "the quick brown fox %s %c %d xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
	char *str2 = "abcdefgh13242314132432 %s %d %c1zzfadfadsfdsafdsafdsafdfdsfsdafdaf";
	int ok;

	uintptr_t ptr = call_allocate(1000, 0, &ok);

	call_write(ptr, str, &ok);
	uintptr_t new_str = call_strdup(maybe_strdup, str, &ok);
printf("test_for_strdup(): allocated[%p] new_str[%p]\n", ptr, new_str);
	assert(new_str != ptr);
	assert(new_str != (uintptr_t) NULL);

	call_read(buf, (uintptr_t) new_str, strlen(str)+1, &ok);
	assert(strncmp(buf, str, strlen(str))==0);

	call_write(new_str, str2, &ok);
	call_read(buf, (uintptr_t) new_str, strlen(str2)+1, &ok);
	call_read(buf2, (uintptr_t) ptr, strlen(str)+1, &ok);
	assert(strncmp(buf, buf2, strlen(str2))!=0);

	return ok;
}

int test_for_strndup(const uintptr_t maybe_strdup)
{
	char buf[2048];
	char *str  = "the quick brown fox %s %c %d xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
	int ok;
	int i;

	printf("test_for_strndup(): enter\n");
	uintptr_t ptr = call_allocate(1000, 0, &ok);

	call_write(ptr, str, &ok);
	uintptr_t new_str = call_strndup(maybe_strdup, str, 5, &ok);

	assert(new_str != ptr);
	assert(new_str != (uintptr_t) NULL);

	call_read(buf, (uintptr_t) new_str, 6, &ok);
	printf("test_for_strndup(): buf[%s] str[%s]\n");
	assert(strncmp(buf,"the q", 5) == 0);

	printf("test_for_strndup(): exit\n");
	return 1;
}

int test_for_malloc(const uintptr_t address)
{
	char *str = "%s xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx xxxxxxxxxxxxxxxxxxxx xxxxxxxxxxxxxxxxx xxxxxxxxxxxxxxxxhello how %s %d are %c %% you?               \n                        %s";
	int ok = 0;
	uintptr_t malloc_address = call_malloc(address, 2000, &ok);
	if (!ok || !malloc_address) return 0;

	call_write(malloc_address, str, &ok);
	if (!ok) return 0;

	char buf[2000];
	call_read(buf, malloc_address, strlen(str)+1, &ok);
	assert(strncmp(buf,str,strlen(str)) == 0);

	// try larger value of malloc
	char str2[10000];
	uintptr_t ptr = call_malloc(address, 2000000, &ok);
	memset(str2, 'x', 500);
	str2[500] = '\0';
	if (!ok) return 0;

	call_write(ptr, str2, &ok);
	call_read(buf, ptr, strlen(str2)+1, &ok);
	assert(strncmp(buf,str2,500) == 0);

	// can only write max 512 byte at a time (infer.h)
	// so advance pointer within malloc'ed region and try writing

	// write on remote site starting at malloc'ed[25000]
	uintptr_t ptr2 = ptr + 25000;
	call_write(ptr2, str2, &ok);
	call_read(buf, ptr2, strlen(str2)+1, &ok);
	assert(strncmp(buf,str2,500) == 0);

	// write on remote site starting at malloc'ed[100000]
	ptr2 = ptr + 100000;
	call_write(ptr2, str2, &ok);
	call_read(buf, ptr2, strlen(str2)+1, &ok);
	assert(strncmp(buf,str2,500) == 0);

	// write on remote site starting at malloc'ed[1000000]
	ptr2 = ptr + 1000000;
	call_write(ptr2, str2, &ok);
	call_read(buf, ptr2, strlen(str2)+1, &ok);
	assert(strncmp(buf,str2,500) == 0);

	return ok;
}

int test_for_calloc(const uintptr_t maybe_calloc)
{
	int ok = 0;
	int i;
	int count = 10;
	int size = 20;
	char buf[1024];

	// call calloc
	uintptr_t rptr = call_calloc(maybe_calloc, count, size, &ok);

	// verify memory returned is zeroed out
	call_read(buf, rptr, count*size, &ok);
	for (i = 0; i < count*size; i++)
		assert(buf[i] == 0);

	char *str = (char*)malloc(count*size);
	for (i = 0; i < count*size; i+=2)
	{
		str[i]= 'a';
		str[i+1]= 'b';
	}
	str[count*size-1] = '\0';

	call_write(rptr, str, &ok);
	call_read(buf, rptr, strlen(str)+1, &ok);
	assert(strncmp(buf,str,strlen(str)) == 0);
	assert(buf[10]=='a');
	assert(buf[11]=='b');
	assert(buf[12]=='a');

	return ok;
}

int test_for_memset(const uintptr_t address)
{
	int ok;
	char buf[1024];

	uintptr_t ptr = call_allocate(256, 61, &ok); // 0..255: 61
	uintptr_t ptr2 = call_memset(address, ptr+246, 54, 1, &ok); // 246: 54
	uintptr_t ptr3 = call_memset(address, ptr+254, 56, 1, &ok); // 254: 56

	assert(ptr != (uintptr_t) NULL);
	assert(ptr2 == ptr + 246);
	assert(ptr3 == ptr + 254);

	call_read(buf, ptr, 256, &ok);
	assert(ok == 1);

	assert(buf[0] == 61);
	assert(buf[246] == 54);
	assert(buf[254] == 56);
	assert(buf[255] == 61);

	return 1;
}

int test_for_memcpy(const uintptr_t maybe_memcpy)
{
	int ok;
	char buf[1024];

	char *str = "hello how are you?";
	uintptr_t src = call_allocate(256, 61, &ok); 
	uintptr_t dst = call_allocate(256, 62, &ok); 

	assert(src);
	assert(dst);
	assert(src != dst);

	call_write(src, str, &ok);
	assert(ok);

	uintptr_t r = call_memcpy(maybe_memcpy, dst, src, strlen(str)+1, &ok);
	assert(r == dst);

	call_read(buf, dst, 256, &ok);
	assert(strncmp(buf,str,strlen(str))==0);

	return 1;
}

// void strclpy(dst,src,size);
// dst will be NULL terminated
// src is a C-string
// size is size of the buffer 
int test_for_strlcpy(const uintptr_t maybe_strlcpy)
{
	int ok;
	char buf[1024] = "";

	char *str = "12345678abcdefghijkl";
	uintptr_t src = call_allocate(256, 61, &ok); 
	uintptr_t dst = call_allocate(8, 62, &ok); 

	assert(src);
	assert(dst);
	assert(src != dst);

	call_write(src, str, &ok);
	assert(ok);

	int src_len = call_strlcpy(maybe_strlcpy, dst, src, 8, &ok);
	assert(ok);
//	assert(src_len == strlen(str)); // unreliable -- CGC examplar doesn't comply w/ API

	call_read(buf, dst, 8, &ok);
	assert(strcmp(buf,"1234567")==0);

	return 1;
}

// void strclpy(dst,src,size);
// dst will be NULL terminated
// src is a C-string
// size is size of the buffer 
int test_for_strlcat(const uintptr_t maybe_strlcat)
{
	int ok;
	char buf[1024] = "";

	char *s1 = "hello, ";
	char *s2 = "how are you? yo yo yo yo";
	uintptr_t src = call_allocate(256, 61, &ok); 
	uintptr_t dst = call_allocate(256, 62, &ok); 

	call_write(src, s2, &ok);
	call_write(dst, s1, &ok);

	int src_len = call_strlcat(maybe_strlcat, dst, src, 200, &ok);
	call_read(buf, dst, 200, &ok);
	assert(strcmp(buf,"hello, how are you? yo yo yo yo")==0);

	// strlcat guarantees null termination
	call_write(dst, s1, &ok);
	src_len = call_strlcat(maybe_strlcat, dst, src, 11, &ok);
	call_read(buf, dst, 200, &ok);
	assert(strcmp(buf,"hello, how") == 0);

	return 1;
}

// void strclpy(dst,src,size);
// dst will not be NULL terminated necessarily
// src is a C-string
// size is size of the buffer 
int test_for_strncat(const uintptr_t maybe_strncat)
{
	int ok;
	char buf[1024] = "";

	char *s1 = "hello, ";
	char *s2 = "how are you? yo yo yo yo";
	uintptr_t src = call_allocate(256, 61, &ok); 
	uintptr_t dst = call_allocate(256, 62, &ok); 

	call_write(src, s2, &ok);
	call_write(dst, s1, &ok);

	int src_len = call_strlcat(maybe_strncat, dst, src, 200, &ok);
	call_read(buf, dst, 200, &ok);
	assert(strcmp(buf,"hello, how are you? yo yo yo yo")==0);

	// strncat does not guarantees null termination
	call_write(dst, s1, &ok);
	src_len = call_strlcat(maybe_strncat, dst, src, 12, &ok);
	call_read(buf, dst, 200, &ok);
	assert(strcmp(buf,"hello, how are you?") == 0);

	return 1;
}

int test_for_strcmp(const uintptr_t maybe_strcmp)
{
	int ok;

	char *s1 = "hello me 1";
	char *s2 = "hello me 2";
	char *s3 = "hello me 3";

	uintptr_t s1a = call_allocate(256, 61, &ok); 
	uintptr_t s2a = call_allocate(256, 62, &ok); 
	uintptr_t s3a = call_allocate(256, 63, &ok); 

	call_write(s1a, s1, &ok);
	call_write(s2a, s2, &ok);
	call_write(s3a, s3, &ok);

	int r;

	r = call_strcmp(maybe_strcmp, s1a, s1a, &ok);
	assert(r == 0);

	r = call_strcmp(maybe_strcmp, s1a, s2a, &ok);
	assert(r < 0);

	r = call_strcmp(maybe_strcmp, s1a, s3a, &ok);
	assert(r < 0);

	r = call_strcmp(maybe_strcmp, s2a, s3a, &ok);
	assert(r < 0);

	r = call_strcmp(maybe_strcmp, s3a, s2a, &ok);
	assert(r > 0);

	return 1;
}

int test_for_strchr(const uintptr_t maybe_strchr)
{
	int ok;

	char *s1 = "hello me 1";
	int c = (int) 'e';

	uintptr_t rptr_s1 = call_allocate(256, 61, &ok); 

	call_write(rptr_s1, s1, &ok);

	uintptr_t rptr = call_strchr(maybe_strchr, rptr_s1, c, &ok);

	char buf[128];
	call_read(buf, rptr, 1, &ok);
	assert(buf[0] == c);
	assert((rptr - rptr_s1) == 1);
	
	return 1;
}

int test_for_memchr(const uintptr_t maybe_memchr)
{
	int ok;

	char *s1 = "hello me 1";
	int m = (int) 'm';

	uintptr_t rptr_s1 = call_allocate(256, 61, &ok); 

	call_write(rptr_s1, s1, &ok);

	uintptr_t rptr = call_memchr(maybe_memchr, rptr_s1, m, 8, &ok);

	char buf[128];
	call_read(buf, rptr, 1, &ok);
	assert(buf[0] == m);
	assert((rptr - rptr_s1) == 6);

	rptr = call_memchr(maybe_memchr, rptr_s1, m, 1, &ok);
	assert(rptr == 0);
	
	return 1;
}

// last occurence
int test_for_strrchr(const uintptr_t maybe_strrchr)
{
	int ok;

	char *s1 = "heleo me 1";
	int c = (int) 'e';

	uintptr_t rptr_s1 = call_allocate(256, 61, &ok); 

	call_write(rptr_s1, s1, &ok);

	uintptr_t rptr = call_strchr(maybe_strrchr, rptr_s1, c, &ok);

	char buf[128];
	call_read(buf, rptr, 1, &ok);
	assert(buf[0] == c);
	assert((rptr - rptr_s1) == 7);
	
	return 1;
}

int test_for_strncmp(const uintptr_t maybe_strncmp)
{
	int ok;

	char *s1 = "hello me 1";
	char *s2 = "hello me 2";
	char *s3 = "hello me 3";

	uintptr_t s1a = call_allocate(256, 61, &ok); 
	uintptr_t s2a = call_allocate(256, 62, &ok); 
	uintptr_t s3a = call_allocate(256, 63, &ok); 

	call_write(s1a, s1, &ok);
	call_write(s2a, s2, &ok);
	call_write(s3a, s3, &ok);

	int r;

	r = call_strncmp(maybe_strncmp, s1a, s1a, 100, &ok);
	assert(r == 0);

	r = call_strncmp(maybe_strncmp, s1a, s2a, 100, &ok);
	assert(r < 0);

	r = call_strncmp(maybe_strncmp, s1a, s3a, 100, &ok);
	assert(r < 0);

	r = call_strncmp(maybe_strncmp, s2a, s3a, 100, &ok);
	assert(r < 0);

	r = call_strncmp(maybe_strncmp, s3a, s2a, 100, &ok);
	assert(r > 0);

	r = call_strncmp(maybe_strncmp, s1a, s2a, 5, &ok);
	assert(r == 0);

	return 1;
}

int test_for_strspn(const uintptr_t maybe_strspn)
{
	int ok;

	char *s1 = "hello jane";
	char *s2 = "hello bob";
	char *s3 = "llo";
	char *s4 = "bob";

	uintptr_t rptr_s1 = call_allocate(256, 61, &ok); 
	uintptr_t rptr_s2 = call_allocate(256, 62, &ok); 
	uintptr_t rptr_s3 = call_allocate(256, 63, &ok); 
	uintptr_t rptr_s4 = call_allocate(256, 64, &ok); 

	call_write(rptr_s1, s1, &ok);
	call_write(rptr_s2, s2, &ok);
	call_write(rptr_s3, s3, &ok);
	call_write(rptr_s4, s4, &ok);

	int span;

	span = call_strspn(maybe_strspn, rptr_s1, rptr_s2, &ok);
	assert(span == strspn(s1,s2));

	span = call_strspn(maybe_strspn, rptr_s2, rptr_s1, &ok);
	assert(span == strspn(s2,s1));

	span = call_strspn(maybe_strspn, rptr_s2, rptr_s3, &ok);
	assert(span == strspn(s2,s3));

	span = call_strspn(maybe_strspn, rptr_s3, rptr_s4, &ok);
	assert(span == strspn(s3,s4));

	return 1;
}

int test_for_strcspn(const uintptr_t maybe_strcspn)
{
	int ok;

	char *s1 = "abcde jane";
	char *s2 = "abcde bob";
	char *s3 = "bob";
	char *s4 = "bcd";

	uintptr_t rptr_s1 = call_allocate(256, 61, &ok); 
	uintptr_t rptr_s2 = call_allocate(256, 62, &ok); 
	uintptr_t rptr_s3 = call_allocate(256, 63, &ok); 
	uintptr_t rptr_s4 = call_allocate(256, 64, &ok); 

	call_write(rptr_s1, s1, &ok);
	call_write(rptr_s2, s2, &ok);
	call_write(rptr_s3, s3, &ok);
	call_write(rptr_s4, s4, &ok);

	int span;

	span = call_strcspn(maybe_strcspn, rptr_s1, rptr_s2, &ok);
	assert(span == strcspn(s1,s2));

	span = call_strcspn(maybe_strcspn, rptr_s2, rptr_s1, &ok);
	assert(span == strcspn(s2,s1));

	span = call_strcspn(maybe_strcspn, rptr_s2, rptr_s3, &ok);
	assert(span == strcspn(s2,s3));

	span = call_strcspn(maybe_strcspn, rptr_s3, rptr_s4, &ok);
	assert(span == strcspn(s3,s4));

	return 1;
}

int test_for_strtok(const uintptr_t maybe_strtok)
{
	int ok;

	char *s = "i am very hungry";
	char *tok = " ";

	uintptr_t rptr_s1 = call_allocate(256, 61, &ok); 
	call_write(rptr_s1, s, &ok);

	uintptr_t rptr_tok = call_allocate(256, 62, &ok); 
	call_write(rptr_tok, tok, &ok);

	char buf[128];
	uintptr_t rptr = call_strtok(maybe_strtok, rptr_s1, rptr_tok, &ok);
	call_read(buf, rptr, 10, &ok);
	printf("test_for_strtok(): buf[%s]\n", buf);
	assert(strcmp(buf,"i") == 0);

	rptr = call_strtok(maybe_strtok, 0, rptr_tok, &ok);
	call_read(buf, rptr, 10, &ok);
	printf("test_for_strtok(): buf[%s]\n", buf);
	assert(strcmp(buf,"am") == 0);

	rptr = call_strtok(maybe_strtok, 0, rptr_tok, &ok);
	call_read(buf, rptr, 10, &ok);
	printf("test_for_strtok(): buf[%s]\n", buf);
	assert(strcmp(buf,"very") == 0);

	rptr = call_strtok(maybe_strtok, 0, rptr_tok, &ok);
	call_read(buf, rptr, 10, &ok);
	printf("test_for_strtok(): buf[%s]\n", buf);
	assert(strcmp(buf,"hungry") == 0);

	rptr = call_strtok(maybe_strtok, 0, rptr_tok, &ok);
	assert(rptr == 0);

	return 1;
}



// void *realloc(void *ptr, size_t size);
int test_for_realloc(const uintptr_t malloc_adddress, const uintptr_t maybe_realloc)
{
	int ok = 0;
	uintptr_t oldptr = call_malloc(malloc_address, 20, &ok);
	if (oldptr && ok)
	{
		char buf[1024];
		char *str = "bonzai! %c %f %s %d";
		call_write(oldptr, str, &ok);

		uintptr_t newptr = call_realloc(maybe_realloc, oldptr, 200, &ok);

		assert(newptr != (uintptr_t) NULL); 
		assert(newptr != oldptr);

		call_read(buf, newptr, strlen(str), &ok);
		assert(strncmp(buf, str, strlen(str))==0);

		return ok;
	}

	return ok;
}

void sig_chld(int signo) 
{
	fprintf(stderr, "prince: SIGNAL %d raised: exit: success = %d\n", signo, success);

	if (success) 
		exit(EXIT_CODE_TEST_SUCCESS); 
	else
		exit(EXIT_CODE_TEST_FAILURE);
}

int test_prince(string executable, string libcFunction, Function_t *functionToTest) {
	int pipefd[2];
	int pipefd2[2];
	pid_t pid;
	int status, died;
	struct sigaction act;

 /* We don't want to block any other signals in this example */
    sigemptyset(&act.sa_mask);

    /*
     * We're only interested in children that have terminated, not ones
     * which have been stopped (eg user pressing control-Z at terminal)
     */
    act.sa_flags = SA_NOCLDSTOP;
    act.sa_handler = sig_chld;

    if (sigaction(SIGCHLD, &act, NULL) < 0) 
    {
        fprintf(stderr, "sigaction failed\n");
	return EXIT_CODE_TEST_FAILURE;
    }

	const char *target = executable.c_str();
	const char *fn = libcFunction.c_str();
	uintptr_t address = functionToTest->GetEntryPoint()->GetAddress()->GetVirtualOffset();

	pipe (pipefd);
	pipe (pipefd2);

	printf("pipes: %d %d\n", pipefd[0], pipefd[1]);
	int p1, p2;
	p1 = dup2(pipefd[0], CINDERELLA_READ);
	p2 = dup2(pipefd[1], CINDERELLA_DRIVER_WRITE);
	printf("dup pipes (driver->cinderella): %d %d\n", p1, p2);
	p1 = dup2(pipefd2[0], CINDERELLA_DRIVER_READ);
	p2 = dup2(pipefd2[1], CINDERELLA_WRITE);
	printf("dup pipes (cinderella->driver): %d %d\n", p1, p2);

	close(pipefd[0]);
	close(pipefd[1]);
	close(pipefd2[0]);
	close(pipefd2[1]);

	switch(pid=fork()) {
		case -1: fprintf(stderr, "can't fork\n");
			exit(-1);
   
		case 0 : // this is the code the child runs 
			close(0); // close stdin
			close(CINDERELLA_DRIVER_READ);
			close(CINDERELLA_DRIVER_WRITE);
//			execl(target, target, (char*)0);
			execl(target, basename((char*)target), (char*)0);
			break;

		default: // this is the code the parent runs 
			close(CINDERELLA_READ);
			close(CINDERELLA_WRITE);
			fprintf(stderr, "closing pipes %d %d\n", pipefd[0], pipefd[1]);
			success = 0;
			if (strcmp(fn, "malloc") == 0)
				success = test_for_malloc(address);
			else if (strcmp(fn, "free") == 0)
			{
				success = find_free(CINDERELLA_DRIVER_READ, CINDERELLA_DRIVER_WRITE, malloc_address, address);
				if (!success) break;
			}
			else if (strcmp(fn, "calloc") == 0)
			{
				success = test_for_calloc(address);
			}
			else if (strcmp(fn, "realloc") == 0)
			{
				success = test_for_realloc(malloc_address, address);
			}
			else if (strcmp(fn, "strlen") == 0)
			{
				success = test_for_strlen(address);
			}
			else if (strcmp(fn, "strdup") == 0)
			{
				success = test_for_strdup(address);
			}
			else if (strcmp(fn, "strndup") == 0)
			{
				success = test_for_strndup(address);
			}
			else if (strcmp(fn, "memset") == 0)
			{
				success = test_for_memset(address);
			}
			else if (strcmp(fn, "memcpy") == 0)
			{
				success = test_for_memcpy(address);
			}
			else if (strcmp(fn, "memchr") == 0)
			{
				success = test_for_memchr(address);
			}
			else if (strcmp(fn, "strlcpy") == 0)
			{
				success = test_for_strlcpy(address);
			}
			else if (strcmp(fn, "strlcat") == 0)
			{
				success = test_for_strlcat(address);
			}
			else if (strcmp(fn, "strncat") == 0)
			{
				success = test_for_strncat(address);
			}
			else if (strcmp(fn, "strcmp") == 0)
			{
				success = test_for_strcmp(address);
			}
			else if (strcmp(fn, "strncmp") == 0)
			{
				success = test_for_strncmp(address);
			}
			else if (strcmp(fn, "strchr") == 0)
			{
				success = test_for_strchr(address);
			}
			else if (strcmp(fn, "strrchr") == 0)
			{
				success = test_for_strrchr(address);
			}
			else if (strcmp(fn, "strspn") == 0)
			{
				success = test_for_strspn(address);
			}
			else if (strcmp(fn, "strcspn") == 0)
			{
				success = test_for_strcspn(address);
			}
			else if (strcmp(fn, "strtok") == 0)
			{
				success = test_for_strtok(address);
			}

			fprintf(stderr, "waiting for child to exit: success = %d\n", success);

			kill(pid, SIGKILL);

//			waitpid(pid, &status, 0);
 	}

	return success ? EXIT_CODE_TEST_SUCCESS : EXIT_CODE_TEST_FAILURE;
}
