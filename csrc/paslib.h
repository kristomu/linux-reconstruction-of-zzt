#ifndef __PASLIB_H__
#define __PASLIB_H__

#define maxint ((int)(1 << (sizeof(integer)*8-1)) - 1)
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

double sin(double);
double cos(double);
double tan(double);
double atan(double);

double exp(double);
double log(double);

double sqrt(double);

#ifdef __cplusplus
}
#endif

/*
 * Macros for some Pascal standard functions
 */


#define trunc(x)  ((integer)(x))

#define pred(type,x) ((type)((x) - 1))

#define succ(type,x) ((type)((x) + 1))

#define pack(a_l,a_h,a,i,z) memcpy(z, &(a)[(i)-(a_l)], sizeof(z))

#define unpack(z,a_l,a_h,a,i) memcpy(&(a)[(i)-(a_l)], (z), sizeof(z))

#define bitsize(x) (sizeof(x)*8)

#define odd(x) ((x) & 1)

#define chr(n) ((char)(n))

#define ord(c) ((int)(unsigned char)(c))

#ifdef __cplusplus

inline int           sqr(signed char x)    {
	return x*x;
}
inline unsigned      sqr(unsigned char x)  {
	return x*x;
}
inline int           sqr(short x)          {
	return x*x;
}
inline unsigned      sqr(unsigned short x) {
	return x*x;
}
inline int           sqr(int x)            {
	return x*x;
}
inline unsigned      sqr(unsigned x)       {
	return x*x;
}
inline long          sqr(long x)           {
	return x*x;
}
inline unsigned long sqr(unsigned long x)  {
	return x*x;
}
inline float         sqr(float x)          {
	return x*x;
}
inline double        sqr(double x)         {
	return x*x;
}

extern "C" void timestamp(integer* day,  integer* month, integer* year,
	integer* hour, integer* min,   integer* sec);

extern "C" real get_realtime();

inline void timestamp(integer & day,  integer & month, integer & year,
	integer & hour, integer & min,   integer & sec) {
	timestamp(&day, &month, &year, &hour, &min, &sec);
}

#else

#ifndef abs
#define abs(x) ((x) < 0 ? -(x) : (x))
#endif
#define sqr(x) ((x)*(x))

void timestamp(integer* day,  integer* month, integer* year,
	integer* hour, integer* min,   integer* sec);

real get_realtime();

integer round(real);

#endif

#ifdef TURBO_PASCAL

#ifdef USE_CONIO
#include <conio.h>
#define readkey()    _getche()
#define keypressed() _kbhit()
#endif

#define lo(x) ((x) & 0xFF)
#define hi(x) (((x) >> 8) & 0xFF)
#define ptr(seg, ofs) (void*)(((seg) << 16) | (ofs))

char* get_date();
char* get_time();

#define EXTERN extern

const double pi = 3.14159265359;

#include <ctype.h>

#define ParamCount       paramcount
#define ExitProc         exitproc
#define RandSeed         randseed
#define DirectVideo      directvideo
#define ParamStr(i)      paramstr(i)
#define GetMem(p, size)  getmem(p, size)
#define FreeMem(p, size) freemem(p, size)
#define ReAllocMem(p, size) reallocmem(p, size)
#define FillChar(p, size, value) fillchar(p, size, value)

#define upcase(c)        ((char)toupper(c))
#define lowcase(c)       ((char)tolower(c))

extern "C" int paramcount;
extern "C" char const* const* param_array;
extern "C" boolean directvideo;
extern "C" pointer exitproc;
extern "C" unsigned randseed;

inline string paramstr(int index) {
	assert(index <= paramcount);
	return string(param_array[index]);
}

inline void halt(int code = EXIT_FAILURE) {
	exit(code);
}

template<typename T> void fillchar(T & ptr, size_t size, char value) {
	memset(ptr, value, size);
}
template<typename T> void getmem(T & ptr, size_t size) {
	(T)ptr = malloc(size);
}
template<typename T> void freemem(T & ptr, size_t size) {
	free(ptr);
}
template<typename T> T reallocmem(T & ptr, size_t size) {
	ptr = (T)realloc(ptr, size);
	return ptr;
}

#endif

#endif


