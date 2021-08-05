#include <stdio.h>
#define _GNU_SOURCE
#include <math.h>

float f1 = 0.5f;
float f2 = 1.0f;
float f3 = 1.5f;

#define FLOAT f1
#define FLOAT2 f2
#define FLOAT3 f3

int i1 = 1;
int pi1 = 1;
long int li1 = 1;

#define INT i1
#define PINT &pi1
#define LINT li1

double d1 = 0.5;
double pd1 = 2.0f;
long double ld1 = 2.0f;

#define DOUBLE d1
#define PDOUBLE &pd1
#define LDOUBLE ld1

char *a = "a";
#define PCHAR a

#define ARG1(N, ...) N
#define ARG2(_1, N, ...) N
#define ARG3(_1, _2, N, ...) N
#define ARG4(_1, _2, _3, N, ...) N
#define ARGC(...) ARGC_(__VA_ARGS__, 5, 4, 3, 2, 1)
#define ARGC_(_1, _2, _3, _4, _5, N, ...) N

#define CONCAT(X, Y) _CONCAT(X,Y)
#define _CONCAT(X, Y) X ## Y
#define CALL(Y, ...) _CALL(Y, __VA_ARGS__)
#define _CALL(Y, ...) Y(__VA_ARGS__)
#define STR(X) XSTR(X)
#define XSTR(X) #X
#define TEST(...) \
	CALL(CONCAT(TEST, ARGC(__VA_ARGS__)), __VA_ARGS__)
#define TEST3(...) \
	STR(ARG1(__VA_ARGS__)), ARG1(__VA_ARGS__)(ARG2(__VA_ARGS__))
#define TEST4(...) \
	STR(ARG1(__VA_ARGS__)), ARG1(__VA_ARGS__)(ARG2(__VA_ARGS__), ARG3(__VA_ARGS__))
#define TEST5(...) \
	STR(ARG1(__VA_ARGS__)), ARG1(__VA_ARGS__)(ARG2(__VA_ARGS__), ARG3(__VA_ARGS__), ARG4(__VA_ARGS__))

int main() {
	printf("%s: %f\n", TEST(acos,FLOAT,EXPECTED));
	printf("%s: %f\n", TEST(atan2,FLOAT,FLOAT2,EXPECTED));
	printf("%s: %f\n", TEST(asin,FLOAT,EXPECTED));
	printf("%s: %f\n", TEST(atan,FLOAT,EXPECTED));
	printf("%s: %f\n", TEST(atan2,FLOAT,FLOAT2,EXPECTED));
	printf("%s: %f\n", TEST(cos,FLOAT,EXPECTED));
	printf("%s: %f\n", TEST(sin,FLOAT,EXPECTED));
	printf("%s: %f\n", TEST(tan,FLOAT,EXPECTED));
	printf("%s: %f\n", TEST(cosh,FLOAT,EXPECTED));
	printf("%s: %f\n", TEST(sinh,FLOAT,EXPECTED));
	printf("%s: %f\n", TEST(tanh,FLOAT,EXPECTED));
	printf("%s: %f\n", TEST(acosh,FLOAT,EXPECTED));
	printf("%s: %f\n", TEST(asinh,FLOAT,EXPECTED));
	printf("%s: %f\n", TEST(atanh,FLOAT,EXPECTED));
	printf("%s: %f\n", TEST(exp,FLOAT,EXPECTED));
	printf("%s: %f\n", TEST(frexp,FLOAT,PINT,EXPECTED));
	printf("%s: %f\n", TEST(ldexp,FLOAT,INT,EXPECTED));
	printf("%s: %f\n", TEST(log,FLOAT,EXPECTED));
	printf("%s: %f\n", TEST(log10,FLOAT,EXPECTED));
	printf("%s: %f\n", TEST(modf,FLOAT,PDOUBLE,EXPECTED));
	//printf("%s: %f\n", TEST(exp10,DOUBLE,EXPECTED));
	//printf("%s: %f\n", TEST(pow10,DOUBLE,EXPECTED));
	printf("%s: %f\n", TEST(expm1,FLOAT,EXPECTED));
	printf("%s: %f\n", TEST(log1p,FLOAT,EXPECTED));
	printf("%s: %f\n", TEST(logb,FLOAT,EXPECTED));
	printf("%s: %f\n", TEST(exp2,FLOAT,EXPECTED));
	printf("%s: %f\n", TEST(log2,FLOAT,EXPECTED));
	printf("%s: %f\n", TEST(pow,FLOAT,FLOAT2,EXPECTED));
	printf("%s: %f\n", TEST(sqrt,FLOAT,EXPECTED));
	printf("%s: %f\n", TEST(hypot,FLOAT,FLOAT2,EXPECTED));
	printf("%s: %f\n", TEST(cbrt,FLOAT,EXPECTED));
	printf("%s: %f\n", TEST(ceil,FLOAT,EXPECTED));
	printf("%s: %f\n", TEST(fabs,FLOAT,EXPECTED));
	printf("%s: %f\n", TEST(floor,FLOAT,EXPECTED));
	printf("%s: %f\n", TEST(fmod,FLOAT,FLOAT2,EXPECTED));
	printf("%s: %f\n", TEST(drem,FLOAT,FLOAT2,EXPECTED));
	printf("%s: %f\n", TEST(significand,FLOAT,EXPECTED));
	printf("%s: %f\n", TEST(copysign,FLOAT,FLOAT2,EXPECTED));
	printf("%s: %f\n", TEST(nan,PCHAR,EXPECTED));
	printf("%s: %f\n", TEST(j0,FLOAT,EXPECTED));
	printf("%s: %f\n", TEST(j1,FLOAT,EXPECTED));
	printf("%s: %f\n", TEST(jn,INT,FLOAT,EXPECTED));
	printf("%s: %f\n", TEST(y0,FLOAT,EXPECTED));
	printf("%s: %f\n", TEST(y1,FLOAT,EXPECTED));
	printf("%s: %f\n", TEST(yn,INT,FLOAT,EXPECTED));
	printf("%s: %f\n", TEST(erf,FLOAT,EXPECTED));
	printf("%s: %f\n", TEST(erfc,FLOAT,EXPECTED));
	printf("%s: %f\n", TEST(lgamma,FLOAT,EXPECTED));
	printf("%s: %f\n", TEST(tgamma,FLOAT,EXPECTED));
	printf("%s: %f\n", TEST(gamma,FLOAT,EXPECTED));
	printf("%s: %f\n", TEST(lgamma_r,FLOAT,PINT,EXPECTED));
	printf("%s: %f\n", TEST(rint,FLOAT,EXPECTED));
	printf("%s: %f\n", TEST(nextafter,FLOAT,FLOAT2,EXPECTED));
	printf("%s: %f\n", TEST(nexttoward,FLOAT,LDOUBLE, EXPECTED));
	printf("%s: %f\n", TEST(remainder,FLOAT,FLOAT2,EXPECTED));
	printf("%s: %f\n", TEST(scalbn,FLOAT,INT,EXPECTED));
	printf("%s: %f\n", TEST(scalbln,FLOAT,LINT,EXPECTED));
	printf("%s: %f\n", TEST(nearbyint,FLOAT,EXPECTED));
	printf("%s: %f\n", TEST(round,FLOAT,EXPECTED));
	printf("%s: %f\n", TEST(trunc,FLOAT,EXPECTED));
	printf("%s: %f\n", TEST(remquo,FLOAT,FLOAT2,PINT,EXPECTED));
	printf("%s: %f\n", TEST(fdim,FLOAT,FLOAT2,EXPECTED));
	printf("%s: %f\n", TEST(fmax,FLOAT,FLOAT2,EXPECTED));
	printf("%s: %f\n", TEST(fmin,FLOAT,FLOAT2,EXPECTED));
	printf("%s: %f\n", TEST(fma,FLOAT,FLOAT2,FLOAT3,EXPECTED));
	printf("%s: %f\n", TEST(scalb,FLOAT,FLOAT2,EXPECTED));
	return 0;
}
