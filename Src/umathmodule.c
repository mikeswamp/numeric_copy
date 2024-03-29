
#include "Python.h"
#include "Numeric/arrayobject.h"
#include "Numeric/ufuncobject.h"
#include "abstract.h"
#include <math.h>

#ifndef CHAR_BIT
#define CHAR_BIT 8
#endif

#ifndef LONG_BIT
#define LONG_BIT (CHAR_BIT * sizeof(long))
#endif

#ifndef INT_BIT
#define INT_BIT (CHAR_BIT * sizeof(int))
#endif

#ifndef UINT_BIT
#define UINT_BIT (CHAR_BIT * sizeof(unsigned int))
#endif

#ifndef SHORT_BIT
#define SHORT_BIT (CHAR_BIT * sizeof(short))
#endif

#ifndef USHORT_BIT
#define USHORT_BIT (CHAR_BIT * sizeof(unsigned short))
#endif

#ifndef USHRT_MAX
#define USHRT_MAX ((1 << (USHORT_BIT)) - 1)
#endif

/* A whole slew of basic math functions are provided by Konrad Hinsen. */

#if !defined(__STDC__) && !defined(_MSC_VER)
extern double fmod (double, double);
extern double frexp (double, int *);
extern double ldexp (double, int);
extern double modf (double, double *);
#endif
#ifndef M_PI
# define M_PI 3.141592653589793238462643383279502884197
#endif

#if !HAVE_INVERSE_HYPERBOLIC
static double acosh(double x)
{
    return log(x + sqrt((x-1.0)*(x+1.0)));
}

static double asinh(double xx)
{
    double x;
    int sign;
    if (xx < 0.0) {
	sign = -1;
	x = -xx;
    }
    else {
	sign = 1;
	x = xx;
    }
    return sign*log(x + sqrt(x*x+1.0));
}

static double atanh(double x)
{
    return 0.5*log((1.0+x)/(1.0-x));
}
#endif


#if defined(HAVE_HYPOT)
#if !defined(NeXT) && !defined(_MSC_VER)
extern double hypot(double, double);
#endif
#else
double hypot(double x, double y)
{
    double yx;

    x = fabs(x);
    y = fabs(y);
    if (x < y) {
	double temp = x;
	x = y;
	y = temp;
    }
    if (x == 0.)
	return 0.;
    else {
	yx = y/x;
	return x*sqrt(1.+yx*yx);
    }
}
#endif

#ifdef i860
/* Cray APP has bogus definition of HUGE_VAL in <math.h> */
#undef HUGE_VAL
#endif

#ifdef HUGE_VAL
#define CHECK(x) if (errno != 0) ; 	else if (-HUGE_VAL <= (x) && (x) <= HUGE_VAL) ; 	else errno = ERANGE
#else
#define CHECK(x) /* Don't know how to check */
#endif



/* First, the C functions that do the real work */

/* constants */
static Py_complex c_1 = {1., 0.};
static Py_complex c_half = {0.5, 0.};
static Py_complex c_i = {0., 1.};
static Py_complex c_i2 = {0., 0.5};
static Py_complex c_sqrt(Py_complex x)
{
    Py_complex r;
    double s,d;
    if (x.real == 0. && x.imag == 0.)
	r = x;
    else {
	s = sqrt(0.5*(fabs(x.real) + hypot(x.real,x.imag)));
	d = 0.5*x.imag/s;
	if (x.real > 0.) {
	    r.real = s;
	    r.imag = d;
	}
	else if (x.imag >= 0.) {
	    r.real = d;
	    r.imag = s;
	}
	else {
	    r.real = -d;
	    r.imag = -s;
	}
    }
    return r;
}

static Py_complex c_log(Py_complex x)
{
    Py_complex r;
    double l = hypot(x.real,x.imag);
    r.imag = atan2(x.imag, x.real);
    r.real = log(l);
    return r;
}

static Py_complex c_prodi(Py_complex x)
{
    Py_complex r;
    r.real = -x.imag;
    r.imag = x.real;
    return r;
}

static Py_complex c_acos(Py_complex x)
{
    return c_neg(c_prodi(c_log(c_sum(x,c_prod(c_i,
					      c_sqrt(c_diff(c_1,c_prod(x,x))))))));
}

static Py_complex c_acosh(Py_complex x)
{
    return c_log(c_sum(x,c_prod(c_i,
				c_sqrt(c_diff(c_1,c_prod(x,x))))));
}

static Py_complex c_asin(Py_complex x)
{
    return c_neg(c_prodi(c_log(c_sum(c_prod(c_i,x),
				     c_sqrt(c_diff(c_1,c_prod(x,x)))))));
}

static Py_complex c_asinh(Py_complex x)
{
    return c_neg(c_log(c_diff(c_sqrt(c_sum(c_1,c_prod(x,x))),x)));
}

static Py_complex c_atan(Py_complex x)
{
    return c_prod(c_i2,c_log(c_quot(c_sum(c_i,x),c_diff(c_i,x))));
}

static Py_complex c_atanh(Py_complex x)
{
    return c_prod(c_half,c_log(c_quot(c_sum(c_1,x),c_diff(c_1,x))));
}

static Py_complex c_cos(Py_complex x)
{
    Py_complex r;
    r.real = cos(x.real)*cosh(x.imag);
    r.imag = -sin(x.real)*sinh(x.imag);
    return r;
}

static Py_complex c_cosh(Py_complex x)
{
    Py_complex r;
    r.real = cos(x.imag)*cosh(x.real);
    r.imag = sin(x.imag)*sinh(x.real);
    return r;
}

static Py_complex c_exp(Py_complex x)
{
    Py_complex r;
    double l = exp(x.real);
    r.real = l*cos(x.imag);
    r.imag = l*sin(x.imag);
    return r;
}

static Py_complex c_log10(Py_complex x)
{
    Py_complex r;
    double l = hypot(x.real,x.imag);
    r.imag = atan2(x.imag, x.real)/log(10.);
    r.real = log10(l);
    return r;
}

static Py_complex c_sin(Py_complex x)
{
    Py_complex r;
    r.real = sin(x.real)*cosh(x.imag);
    r.imag = cos(x.real)*sinh(x.imag);
    return r;
}

static Py_complex c_sinh(Py_complex x)
{
    Py_complex r;
    r.real = cos(x.imag)*sinh(x.real);
    r.imag = sin(x.imag)*cosh(x.real);
    return r;
}

static Py_complex c_tan(Py_complex x)
{
    Py_complex r;
    double sr,cr,shi,chi;
    double rs,is,rc,ic;
    double d;
    sr = sin(x.real);
    cr = cos(x.real);
    shi = sinh(x.imag);
    chi = cosh(x.imag);
    rs = sr*chi;
    is = cr*shi;
    rc = cr*chi;
    ic = -sr*shi;
    d = rc*rc + ic*ic;
    r.real = (rs*rc+is*ic)/d;
    r.imag = (is*rc-rs*ic)/d;
    return r;
}

static Py_complex c_tanh(Py_complex x)
{
    Py_complex r;
    double si,ci,shr,chr;
    double rs,is,rc,ic;
    double d;
    si = sin(x.imag);
    ci = cos(x.imag);
    shr = sinh(x.real);
    chr = cosh(x.real);
    rs = ci*shr;
    is = si*chr;
    rc = ci*chr;
    ic = si*shr;
    d = rc*rc + ic*ic;
    r.real = (rs*rc+is*ic)/d;
    r.imag = (is*rc-rs*ic)/d;
    return r;
}

static long powll(long x, long n, int nbits)
     /* Overflow check: overflow will occur if log2(abs(x)) * n > nbits. */
{
    long r = 1;
    long p = x;
    double logtwox;
    long mask = 1;
    if (n < 0) PyErr_SetString(PyExc_ValueError, "Integer to a negative power");
    if (x != 0) {
	logtwox = log10 (fabs ( (double) x))/log10 ( (double) 2.0);
	if (logtwox * (double) n > (double) nbits)
	    PyErr_SetString(PyExc_ArithmeticError, "Integer overflow in power.");
    }
    while (mask > 0 && n >= mask) {
	if (n & mask)
	    r *= p;
	mask <<= 1;
	p *= p;
    }
    return r;
}

static void UBYTE_add(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((unsigned char *)op)=*((unsigned char *)i1) + *((unsigned char *)i2);
    }
}
static void SBYTE_add(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((signed char *)op)=*((signed char *)i1) + *((signed char *)i2);
    }
}
static void SHORT_add(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((short *)op)=*((short *)i1) + *((short *)i2);
    }
}
static void USHORT_add(char **args, int *dimensions, int *steps, void *func) {
  int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
  char *i1=args[0], *i2=args[1], *op=args[2];
  for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
    *((unsigned short *)op)=*((unsigned short *)i1) + *((unsigned short *)i2);
  }
}
static void INT_add(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((int *)op)=*((int *)i1) + *((int *)i2);
    }
}
static void UINT_add(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((unsigned int *)op)=*((unsigned int *)i1) + *((unsigned int *)i2);
    }
}
static void LONG_add(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((long *)i1) + *((long *)i2);
    }
}
static void FLOAT_add(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((float *)op)=*((float *)i1) + *((float *)i2);
    }
}
static void DOUBLE_add(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((double *)op)=*((double *)i1) + *((double *)i2);
    }
}
static void CFLOAT_add(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	((float *)op)[0]=((float *)i1)[0] + ((float *)i2)[0]; ((float *)op)[1]=((float *)i1)[1] + ((float *)i2)[1];
    }
}
static void CDOUBLE_add(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	((double *)op)[0]=((double *)i1)[0] + ((double *)i2)[0]; ((double *)op)[1]=((double *)i1)[1] + ((double *)i2)[1];
    }
}
static void UBYTE_subtract(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((unsigned char *)op)=*((unsigned char *)i1) - *((unsigned char *)i2);
    }
}
static void SBYTE_subtract(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((signed char *)op)=*((signed char *)i1) - *((signed char *)i2);
    }
}
static void SHORT_subtract(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((short *)op)=*((short *)i1) - *((short *)i2);
    }
}
static void USHORT_subtract(char **args, int *dimensions, int *steps, void *func) {
  int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
  char *i1=args[0], *i2=args[1], *op=args[2];
  for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
    *((unsigned short *)op)=*((unsigned short *)i1) - *((unsigned short *)i2);
  }
}
static void INT_subtract(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((int *)op)=*((int *)i1) - *((int *)i2);
    }
}
static void UINT_subtract(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((unsigned int *)op)=*((unsigned int *)i1) - *((unsigned int *)i2);
    }
}
static void LONG_subtract(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((long *)i1) - *((long *)i2);
    }
}
static void FLOAT_subtract(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((float *)op)=*((float *)i1) - *((float *)i2);
    }
}
static void DOUBLE_subtract(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((double *)op)=*((double *)i1) - *((double *)i2);
    }
}
static void CFLOAT_subtract(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	((float *)op)[0]=((float *)i1)[0] - ((float *)i2)[0]; ((float *)op)[1]=((float *)i1)[1] - ((float *)i2)[1];
    }
}
static void CDOUBLE_subtract(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	((double *)op)[0]=((double *)i1)[0] - ((double *)i2)[0]; ((double *)op)[1]=((double *)i1)[1] - ((double *)i2)[1];
    }
}
static void UBYTE_multiply(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    unsigned int x;
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	x = (unsigned int) (*((unsigned char *)i1)) * (unsigned int) (*((unsigned char *)i2));
	if (x > 255) {
	    PyErr_SetString (PyExc_ArithmeticError, "Integer overflow in multiply.");
	    return;
	}
	*((unsigned char *)op)=(unsigned char) x;
    }
}
static void SBYTE_multiply(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    int x;
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	x = (int) (*((signed char *)i1)) * (int) (*((signed char *)i2));
	if (x > 127 || x < -128) {
	    PyErr_SetString (PyExc_ArithmeticError, "Integer overflow in multiply.");
	    return;
	}
	*((signed char *)op)=(signed char) x;
    }
}
static void SHORT_multiply(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    short a, b, ah, bh, x, y;
    int s;
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	s = 1;
	a = *((short *)i1);
	b = *((short *)i2);
	ah = a >> (SHORT_BIT/2);
	bh = b >> (SHORT_BIT/2);
	/* Quick test for common case: two small positive shorts */
	if (ah == 0 && bh == 0) {
	    if ((x=a*b) < 0) {
		PyErr_SetString (PyExc_ArithmeticError, "Integer overflow in multiply.");
		return;
	    }
	    else {
		*((short *)op)=x;
		continue;
	    }
	}
	/* Arrange that a >= b >= 0 */
	if (a < 0) {
	    a = -a;
	    if (a < 0) {
		/* Largest negative */
		if (b == 0 || b == 1) {
		    *((short *)op)=a*b;
		    continue;
		}
		else {
		    PyErr_SetString (PyExc_ArithmeticError, "Integer overflow in multiply.");
		    return;
		}
	    }
	    s = -s;
	    ah = a >> (SHORT_BIT/2);
	}
	if (b < 0) {
	    b = -b;
	    if (b < 0) {
		/* Largest negative */
		if (a == 0 || a == 1) {
		    *((short *)op)=a*b;
		    continue;
		}
		else {
		    PyErr_SetString (PyExc_ArithmeticError, "Integer overflow in multiply.");
		    return;
		}
	    }
	    s = -s;
	    bh = b >> (SHORT_BIT/2);
	}
	/* 1) both ah and bh > 0 : then report overflow */
	if (ah != 0 && bh != 0) {
	    PyErr_SetString (PyExc_ArithmeticError, "Integer overflow in multiply.");
	    return;
	}
	/* 2) both ah and bh = 0 : then compute a*b and report
	   overflow if it comes out negative */
	if (ah == 0 && bh == 0) {
	    if ((x=a*b) < 0) {
		PyErr_SetString (PyExc_ArithmeticError, "Integer overflow in multiply.");
		return;
	    }
	    else {
		*((short *)op)=s * x;
		continue;
	    }
	}
	if (a < b) {
	    /* Swap */
	    x = a;
	    a = b;
	    b = x;
	    ah = bh;
	    /* bh not used beyond this point */
	}
	/* 3) ah > 0 and bh = 0  : compute ah*bl and report overflow if
	   it's >= 2^31
	   compute al*bl and report overflow if it's negative
	   add (ah*bl)<<32 to al*bl and report overflow if
	   it's negative
	   (NB b == bl in this case, and we make a = al) */
	y = ah*b;
	if (y >= (1 << (SHORT_BIT/2 - 1))) {
	    PyErr_SetString (PyExc_ArithmeticError, "Integer overflow in multiply.");
	    return;
	}
	a &= (1 << (SHORT_BIT/2)) - 1;
	x = a*b;
	if (x < 0) {
	    PyErr_SetString (PyExc_ArithmeticError, "Integer overflow in multiply.");
	    return;
	}
	x += y << (SHORT_BIT/2);
	if (x < 0) {
	    PyErr_SetString (PyExc_ArithmeticError, "Integer overflow in multiply.");
	    return;
	}
	*((short *)op)=s*x;
    }
}
static void USHORT_multiply(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    unsigned int x;
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	x = (unsigned int) (*((unsigned short *)i1)) * (unsigned int) (*((unsigned short *)i2));
	if (x > USHRT_MAX) {
	    PyErr_SetString (PyExc_ArithmeticError, "Integer overflow in multiply.");
	    return;
	}
	*((unsigned short *)op)=(unsigned short) x;
    }
}

static void INT_multiply(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    int a, b, ah, bh, x, y;
    int s;
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	s = 1;
	a = *((int *)i1);
	b = *((int *)i2);
	ah = a >> (INT_BIT/2);
	bh = b >> (INT_BIT/2);
	/* Quick test for common case: two small positive ints */
	if (ah == 0 && bh == 0) {
	    if ((x=a*b) < 0) {
		PyErr_SetString (PyExc_ArithmeticError, "Integer overflow in multiply.");
		return;
	    }
	    else {
		*((int *)op)=x;
		continue;
	    }
	}
	/* Arrange that a >= b >= 0 */
	if (a < 0) {
	    a = -a;
	    if (a < 0) {
		/* Largest negative */
		if (b == 0 || b == 1) {
		    *((int *)op)=a*b;
		    continue;
		}
		else {
		    PyErr_SetString (PyExc_ArithmeticError, "Integer overflow in multiply.");
		    return;
		}
	    }
	    s = -s;
	    ah = a >> (INT_BIT/2);
	}
	if (b < 0) {
	    b = -b;
	    if (b < 0) {
		/* Largest negative */
		if (a == 0 || a == 1) {
		    *((int *)op)=a*b;
		    continue;
		}
		else {
		    PyErr_SetString (PyExc_ArithmeticError, "Integer overflow in multiply.");
		    return;
		}
	    }
	    s = -s;
	    bh = b >> (INT_BIT/2);
	}
	/* 1) both ah and bh > 0 : then report overflow */
	if (ah != 0 && bh != 0) {
	    PyErr_SetString (PyExc_ArithmeticError, "Integer overflow in multiply.");
	    return;
	}
	/* 2) both ah and bh = 0 : then compute a*b and report
	   overflow if it comes out negative */
	if (ah == 0 && bh == 0) {
	    if ((x=a*b) < 0) {
		PyErr_SetString (PyExc_ArithmeticError, "Integer overflow in multiply.");
		return;
	    }
	    else {
		*((int *)op)=s * x;
		continue;
	    }
	}
	if (a < b) {
	    /* Swap */
	    x = a;
	    a = b;
	    b = x;
	    ah = bh;
	    /* bh not used beyond this point */
	}
	/* 3) ah > 0 and bh = 0  : compute ah*bl and report overflow if
	   it's >= 2^31
	   compute al*bl and report overflow if it's negative
	   add (ah*bl)<<32 to al*bl and report overflow if
	   it's negative
	   (NB b == bl in this case, and we make a = al) */
	y = ah*b;
	if (y >= (1 << (INT_BIT/2 - 1))) {
	    PyErr_SetString (PyExc_ArithmeticError, "Integer overflow in multiply.");
	    return;
	}
	a &= (1 << (INT_BIT/2)) - 1;
	x = a*b;
	if (x < 0) {
	    PyErr_SetString (PyExc_ArithmeticError, "Integer overflow in multiply.");
	    return;
	}
	x += y << (INT_BIT/2);
	if (x < 0) {
	    PyErr_SetString (PyExc_ArithmeticError, "Integer overflow in multiply.");
	    return;
	}
	*((int *)op)=s*x;
    }
}

static void UINT_multiply(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    unsigned int a, b, ah, bh, x, y;
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	a = *((unsigned int *)i1);
	b = *((unsigned int *)i2);
	ah = a >> (INT_BIT/2);
	bh = b >> (INT_BIT/2);
	/* Quick test for common case: two small positive ints */
	if (ah == 0 && bh == 0) {  /* result should fit into bits available. */
            *((unsigned int *)op)= a*b;
            continue;
        }
	/* 1) both ah and bh > 0 : then report overflow */
	if (ah != 0 && bh != 0) {
	    PyErr_SetString (PyExc_ArithmeticError, "Integer overflow in multiply.");
	    return;
	}
        /* Otherwise one and only one of ah or bh is non-zero.  Make it so a > b (ah >0 and bh=0) */
	if (a < b) {  
	    /* Swap */
	    x = a;
	    a = b;
	    b = x;
	    ah = bh;
	    /* bh not used beyond this point */
	}
        /* Now a = ah */
	/* 3) ah > 0 and bh = 0  : compute ah*bl and report overflow if
	   it's >= 2^(INT_BIT/2)  -- shifted_version won't fit in unsigned int.

           Then compute al*bl (this should fit in the allotated space)

	   compute al*bl and report overflow if it's negative
	   add (ah*bl)<<32 to al*bl and report overflow if
	   it's negative
	   (NB b == bl in this case, and we make a = al) */
	y = ah*b;
	if (y >= (1 << (INT_BIT/2))) {
	    PyErr_SetString (PyExc_ArithmeticError, "Integer overflow in multiply.");
	    return;
	}
	a &= (1 << (INT_BIT/2)) - 1;  /* mask off ah so a is now al */
	x = a*b;  /* al * bl */
	x += y << (INT_BIT/2);  /* add ah * bl * 2^SHIFT */
        /* This could have caused overflow.  One way to know is to check to see if x < al 
             Not sure if this get's all cases */
	if (x < a) {
	    PyErr_SetString (PyExc_ArithmeticError, "Integer overflow in multiply.");
	    return;
	}
	*((unsigned int *)op)=x;
    }
}

static void LONG_multiply(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    long a, b, ah, bh, x, y;
    int s;
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	s = 1;
	a = *((long *)i1);
	b = *((long *)i2);
	ah = a >> (LONG_BIT/2);
	bh = b >> (LONG_BIT/2);
	/* Quick test for common case: two small positive ints */
	if (ah == 0 && bh == 0) {
	    if ((x=a*b) < 0) {
		PyErr_SetString (PyExc_ArithmeticError, "Integer overflow in multiply.");
		return;
	    }
	    else {
		*((long *)op)=x;
		continue;
	    }
	}
	/* Arrange that a >= b >= 0 */
	if (a < 0) {
	    a = -a;
	    if (a < 0) {
		/* Largest negative */
		if (b == 0 || b == 1) {
		    *((long *)op)=a*b;
		    continue;
		}
		else {
		    PyErr_SetString (PyExc_ArithmeticError, "Integer overflow in multiply.");
		    return;
		}
	    }
	    s = -s;
	    ah = a >> (LONG_BIT/2);
	}
	if (b < 0) {
	    b = -b;
	    if (b < 0) {
		/* Largest negative */
		if (a == 0 || a == 1) {
		    *((long *)op)=a*b;
		    continue;
		}
		else {
		    PyErr_SetString (PyExc_ArithmeticError, "Integer overflow in multiply.");
		    return;
		}
	    }
	    s = -s;
	    bh = b >> (LONG_BIT/2);
	}
	/* 1) both ah and bh > 0 : then report overflow */
	if (ah != 0 && bh != 0) {
	    PyErr_SetString (PyExc_ArithmeticError, "Integer overflow in multiply.");
	    return;
	}
	/* 2) both ah and bh = 0 : then compute a*b and report
	   overflow if it comes out negative */
	if (ah == 0 && bh == 0) {
	    if ((x=a*b) < 0) {
		PyErr_SetString (PyExc_ArithmeticError, "Integer overflow in multiply.");
		return;
	    }
	    else {
		*((long *)op)=s * x;
		continue;
	    }
	}
	if (a < b) {
	    /* Swap */
	    x = a;
	    a = b;
	    b = x;
	    ah = bh;
	    /* bh not used beyond this point */
	}
	/* 3) ah > 0 and bh = 0  : compute ah*bl and report overflow if
	   it's >= 2^31
	   compute al*bl and report overflow if it's negative
	   add (ah*bl)<<32 to al*bl and report overflow if
	   it's negative
	   (NB b == bl in this case, and we make a = al) */
	y = ah*b;
	if (y >= (1L << (LONG_BIT/2 - 1))) {
	    PyErr_SetString (PyExc_ArithmeticError, "Integer overflow in multiply.");
	    return;
	}
	a &= (1L << (LONG_BIT/2)) - 1;
	x = a*b;
	if (x < 0) {
	    PyErr_SetString (PyExc_ArithmeticError, "Integer overflow in multiply.");
	    return;
	}
	x += y << (LONG_BIT/2);
	if (x < 0) {
	    PyErr_SetString (PyExc_ArithmeticError, "Integer overflow in multiply.");
	    return;
	}
	*((long *)op)=s*x;
    }
}
static void FLOAT_multiply(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((float *)op)=*((float *)i1) * *((float *)i2);
    }
}
static void DOUBLE_multiply(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((double *)op)=*((double *)i1) * *((double *)i2);
    }
}
static void UBYTE_divide(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((unsigned char *)op)=*((unsigned char *)i2) == 0 ? PyErr_SetString(PyExc_ZeroDivisionError, "divide by zero"),0 : *((unsigned char *)i1) / *((unsigned char *)i2);
    }
}
static void SBYTE_divide(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((signed char *)op)=*((signed char *)i2) == 0 ? PyErr_SetString(PyExc_ZeroDivisionError, "divide by zero"),0 : *((signed char *)i1) / *((signed char *)i2);
    }
}
static void SHORT_divide(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((short *)op)=*((short *)i2) == 0 ? PyErr_SetString(PyExc_ZeroDivisionError, "divide by zero"),0 : *((short *)i1) / *((short *)i2);
    }
}
static void USHORT_divide(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
        *((unsigned short *)op)=*((unsigned short *)i2) == 0 ? PyErr_SetString(PyExc_ZeroDivisionError, "divide by zero"),0 : *((unsigned short *)i1) / *((unsigned short *)i2);
    }
}
static void INT_divide(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((int *)op)=*((int *)i2) == 0 ? PyErr_SetString(PyExc_ZeroDivisionError, "divide by zero"),0 : *((int *)i1) / *((int *)i2);
    }
}
static void UINT_divide(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((unsigned int *)op)=*((unsigned int *)i2) == 0 ? PyErr_SetString(PyExc_ZeroDivisionError, "divide by zero"),0 : *((int *)i1) / *((int *)i2);
    }
}
static void LONG_divide(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((long *)i2) == 0 ? PyErr_SetString(PyExc_ZeroDivisionError, "divide by zero"),0 : *((long *)i1) / *((long *)i2);
    }
}
static void FLOAT_divide(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((float *)op)=*((float *)i1) / *((float *)i2);
    }
}
static void DOUBLE_divide(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((double *)op)=*((double *)i1) / *((double *)i2);
    }
}

static void UBYTE_floor_divide(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((unsigned char *)op)=*((unsigned char *)i2) == 0 ? PyErr_SetString(PyExc_ZeroDivisionError, "divide by zero"),0 : *((unsigned char *)i1) / *((unsigned char *)i2);
    }
}
static void SBYTE_floor_divide(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((signed char *)op)=*((signed char *)i2) == 0 ? PyErr_SetString(PyExc_ZeroDivisionError, "divide by zero"),0 : *((signed char *)i1) / *((signed char *)i2);
    }
}
static void SHORT_floor_divide(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((short *)op)=*((short *)i2) == 0 ? PyErr_SetString(PyExc_ZeroDivisionError, "divide by zero"),0 : *((short *)i1) / *((short *)i2);
    }
}
static void USHORT_floor_divide(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((unsigned short *)op)=*((unsigned short *)i2) == 0 ? PyErr_SetString(PyExc_ZeroDivisionError, "divide by zero"),0 : *((unsigned short *)i1) / *((unsigned short *)i2);
    }
}
static void INT_floor_divide(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((int *)op)=*((int *)i2) == 0 ? PyErr_SetString(PyExc_ZeroDivisionError, "divide by zero"),0 : *((int *)i1) / *((int *)i2);
    }
}
static void UINT_floor_divide(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((unsigned int *)op)=*((unsigned int *)i2) == 0 ? PyErr_SetString(PyExc_ZeroDivisionError, "divide by zero"),0 : *((int *)i1) / *((int *)i2);
    }
}
static void LONG_floor_divide(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((long *)i2) == 0 ? PyErr_SetString(PyExc_ZeroDivisionError, "divide by zero"),0 : *((long *)i1) / *((long *)i2);
    }
}
static void FLOAT_floor_divide(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((float *)op)=floor((double) (*((float *)i1) / *((float *)i2)));
    }
}
static void DOUBLE_floor_divide(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((double *)op)=floor(*((double *)i1) / *((double *)i2));
    }
}

static void UBYTE_true_divide(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((float *)op)=*((unsigned char *)i2) == 0 ? PyErr_SetString(PyExc_ZeroDivisionError, "UB divide by zero"),0 : ((float)*((unsigned char *)i1)) / ((float)*((unsigned char *)i2));
    }
}
static void SBYTE_true_divide(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((float *)op)=*((signed char *)i2) == 0 ? PyErr_SetString(PyExc_ZeroDivisionError, "SB divide by zero"),0 : ((float)*((signed char *)i1)) / ((float)*((signed char *)i2));
    }
}
static void SHORT_true_divide(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((float *)op)=*((short *)i2) == 0 ? PyErr_SetString(PyExc_ZeroDivisionError, "S divide by zero"),0 : ((float)*((short *)i1)) / ((float)*((short *)i2));
    }
}
static void USHORT_true_divide(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((float *)op)=*((unsigned short *)i2) == 0 ? PyErr_SetString(PyExc_ZeroDivisionError, "S divide by zero"),0 : ((float)*((unsigned short *)i1)) / ((float)*((unsigned short *)i2));
    }
}
static void INT_true_divide(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((double *)op)=*((int *)i2) == 0 ? PyErr_SetString(PyExc_ZeroDivisionError, "I divide by zero"),0 : ((double)*((int *)i1)) / ((double)*((int *)i2));
    }
}
static void UINT_true_divide(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((float *)op)=*((unsigned int *)i2) == 0 ? PyErr_SetString(PyExc_ZeroDivisionError, "I divide by zero"),0 : ((float)*((int *)i1)) / ((float)*((int *)i2));
    }
}
static void LONG_true_divide(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((double *)op)=*((long *)i2) == 0 ? PyErr_SetString(PyExc_ZeroDivisionError, "L divide by zero"),0 : ((double)*((long *)i1)) / ((double)*((long *)i2));
    }
}
static void FLOAT_true_divide(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((float *)op)=*((float *)i1) / *((float *)i2);
    }
}
static void DOUBLE_true_divide(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((double *)op)=*((double *)i1) / *((double *)i2);
    }
}

static void UBYTE_divide_safe(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((unsigned char *)op)=*((unsigned char *)i2) == 0 ? PyErr_SetString(PyExc_ZeroDivisionError, "divide by zero"),0 : *((unsigned char *)i1) / *((unsigned char *)i2);
    }
}
static void SBYTE_divide_safe(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((signed char *)op)=*((signed char *)i2) == 0 ? PyErr_SetString(PyExc_ZeroDivisionError, "divide by zero"),0 : *((signed char *)i1) / *((signed char *)i2);
    }
}
static void SHORT_divide_safe(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((short *)op)=*((short *)i2) == 0 ? PyErr_SetString(PyExc_ZeroDivisionError, "divide by zero"),0 : *((short *)i1) / *((short *)i2);
    }
}
static void USHORT_divide_safe(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
        *((unsigned short *)op)=*((unsigned short *)i2) == 0 ? PyErr_SetString(PyExc_ZeroDivisionError, "divide by zero"),0 : *((unsigned short *)i1) / *((unsigned short *)i2);
    }
}
static void INT_divide_safe(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((int *)op)=*((int *)i2) == 0 ? PyErr_SetString(PyExc_ZeroDivisionError, "divide by zero"),0 : *((int *)i1) / *((int *)i2);
    }
}
static void UINT_divide_safe(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((unsigned int *)op)=*((unsigned int *)i2) == 0 ? PyErr_SetString(PyExc_ZeroDivisionError, "divide by zero"),0 : *((int *)i1) / *((int *)i2);
    }
}
static void LONG_divide_safe(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((long *)i2) == 0 ? PyErr_SetString(PyExc_ZeroDivisionError, "divide by zero"),0 : *((long *)i1) / *((long *)i2);
    }
}
static void FLOAT_divide_safe(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((float *)op)=*((float *)i2) == 0 ? PyErr_SetString(PyExc_ZeroDivisionError, "divide by zero"),0 : *((float *)i1) / *((float *)i2);
    }
}
static void DOUBLE_divide_safe(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((double *)op)=*((double *)i2) == 0 ? PyErr_SetString(PyExc_ZeroDivisionError, "divide by zero"),0 : *((double *)i1) / *((double *)i2);
    }
}
static void UBYTE_conjugate(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((unsigned char *)op)=*((unsigned char *)i1);}}
static void SBYTE_conjugate(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((signed char *)op)=*((signed char *)i1);}}
static void SHORT_conjugate(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((short *)op)=*((short *)i1);}}
static void USHORT_conjugate(char **args, int *dimensions, int *steps, void *func) 
  {int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((unsigned short *)op)=*((unsigned short *)i1);}}
static void INT_conjugate(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((int *)op)=*((int *)i1);}}
static void UINT_conjugate(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((unsigned int *)op)=*((unsigned int *)i1);}}
static void LONG_conjugate(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((long *)op)=*((long *)i1);}}
static void FLOAT_conjugate(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((float *)op)=*((float *)i1);}}
static void DOUBLE_conjugate(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((double *)op)=*((double *)i1);}}
static void CFLOAT_conjugate(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {((float *)op)[0]=((float *)i1)[0]; ((float *)op)[1]=-((float *)i1)[1];}}
static void CDOUBLE_conjugate(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {((double *)op)[0]=((double *)i1)[0]; ((double *)op)[1]=-((double *)i1)[1];}}
static void UBYTE_remainder(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((unsigned char *)op)=*((unsigned char *)i1) % *((unsigned char *)i2);
    }
}
static void SBYTE_remainder(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((signed char *)op)=*((signed char *)i1) % *((signed char *)i2);
    }
}
static void SHORT_remainder(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((short *)op)=*((short *)i1) % *((short *)i2);
    }
}
static void USHORT_remainder(char **args, int *dimensions, int *steps, void *func) {
  int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
  char *i1=args[0], *i2=args[1], *op=args[2];
  for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
    *((unsigned short *)op)=*((unsigned short *)i1) % *((unsigned short *)i2);
  }
}
static void INT_remainder(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((int *)op)=*((int *)i1) % *((int *)i2);
    }
}
static void UINT_remainder(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((unsigned int *)op)=*((unsigned int *)i1) % *((unsigned int *)i2);
    }
}
static void LONG_remainder(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((long *)i1) % *((long *)i2);
    }
}
static void UBYTE_power(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((unsigned char *)op)=(unsigned char)powll(*((unsigned char *)i1), *((unsigned char *)i2), 8);
    }
}
static void SBYTE_power(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((signed char *)op)=(signed char)powll(*((signed char *)i1), *((signed char *)i2), 7);
    }
}
static void SHORT_power(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((short *)op)=(short)powll(*((short *)i1), *((short *)i2), SHORT_BIT - 1);
    }
}
static void USHORT_power(char **args, int *dimensions, int *steps, void *func) {
  int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
  char *i1=args[0], *i2=args[1], *op=args[2];
  for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
    *((unsigned short *)op)=(unsigned short)powll(*((unsigned short *)i1), *((unsigned short *)i2), USHORT_BIT - 1);
  }
}
static void INT_power(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((int *)op)=powll(*((int *)i1), *((int *)i2), INT_BIT - 1);
    }
}
static void UINT_power(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((unsigned int *)op)=powll(*((unsigned int *)i1), *((unsigned int *)i2), INT_BIT - 1);
    }
}
static void LONG_power(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=powll(*((long *)i1), *((long *)i2), LONG_BIT - 1);
    }
}

static void UBYTE_absolute(char **args, int *dimensions, int *steps, void *func)
{int i; char *i1=args[0], *op=args[1]; for (i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((unsigned char *)op) = *((unsigned char *)i1);}}
static void SBYTE_absolute(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((signed char *)op)=*((signed char *)i1)<0 ? -*((signed char *)i1) : *((signed char *)i1);}}
static void SHORT_absolute(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((short *)op)=*((short *)i1)<0 ? -*((short *)i1) : *((short *)i1);}}
static void USHORT_absolute(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((unsigned short *)op)=*((unsigned short *)i1);}}
static void INT_absolute(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((int *)op)=*((int *)i1)<0 ? -*((int *)i1) : *((int *)i1);}}
static void UINT_absolute(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((unsigned int *)op)=*((unsigned int *)i1);}}
static void LONG_absolute(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((long *)op)=*((long *)i1)<0 ? -*((long *)i1) : *((long *)i1);}}
static void FLOAT_absolute(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((float *)op)=*((float *)i1)<0 ? -*((float *)i1) : *((float *)i1);}}
static void DOUBLE_absolute(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((double *)op)=*((double *)i1)<0 ? -*((double *)i1) : *((double *)i1);}}
static void CFLOAT_absolute(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((float *)op)=(float)sqrt(((float *)i1)[0]*((float *)i1)[0] + ((float *)i1)[1]*((float *)i1)[1]);}}
static void CDOUBLE_absolute(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((double *)op)=sqrt(((double *)i1)[0]*((double *)i1)[0] + ((double *)i1)[1]*((double *)i1)[1]);}}
static void UBYTE_negative(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((unsigned char *)op)=-*((unsigned char *)i1);}}
static void SBYTE_negative(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((signed char *)op)=-*((signed char *)i1);}}
static void SHORT_negative(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((short *)op)=-*((short *)i1);}}
static void USHORT_negative(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((unsigned short *)op)=-*((unsigned short *)i1);}}
static void INT_negative(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((int *)op)=-*((int *)i1);}}
static void UINT_negative(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((unsigned int *)op)=-*((unsigned int *)i1);}}
static void LONG_negative(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((long *)op)=-*((long *)i1);}}
static void FLOAT_negative(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((float *)op)=-*((float *)i1);}}
static void DOUBLE_negative(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((double *)op)=-*((double *)i1);}}
static void CFLOAT_negative(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {((float *)op)[0]=-((float *)i1)[0]; ((float *)op)[1]=-((float *)i1)[1];}}
static void CDOUBLE_negative(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {((double *)op)[0]=-((double *)i1)[0]; ((double *)op)[1]=-((double *)i1)[1];}}
static void UBYTE_greater(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((unsigned char *)i1) > *((unsigned char *)i2);
    }
}
static void SBYTE_greater(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((signed char *)i1) > *((signed char *)i2);
    }
}
static void SHORT_greater(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((short *)i1) > *((short *)i2);
    }
}
static void USHORT_greater(char **args, int *dimensions, int *steps, void *func) {
  int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
  char *i1=args[0], *i2=args[1], *op=args[2];
  for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
    *((long *)op)=*((unsigned short *)i1) > *((unsigned short *)i2);
  }
}
static void INT_greater(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((int *)i1) > *((int *)i2);
    }
}
static void UINT_greater(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((unsigned int *)i1) > *((unsigned int *)i2);
    }
}
static void LONG_greater(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((long *)i1) > *((long *)i2);
    }
}
static void FLOAT_greater(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((float *)i1) > *((float *)i2);
    }
}
static void DOUBLE_greater(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((double *)i1) > *((double *)i2);
    }
}
static void UBYTE_greater_equal(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((unsigned char *)i1) >= *((unsigned char *)i2);
    }
}
static void SBYTE_greater_equal(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((signed char *)i1) >= *((signed char *)i2);
    }
}
static void SHORT_greater_equal(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((short *)i1) >= *((short *)i2);
    }
}
static void USHORT_greater_equal(char **args, int *dimensions, int *steps, void *func) {
  int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
  char *i1=args[0], *i2=args[1], *op=args[2];
  for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
    *((long *)op)=*((unsigned short *)i1) >= *((unsigned short *)i2);
  }
}
static void INT_greater_equal(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((int *)i1) >= *((int *)i2);
    }
}
static void UINT_greater_equal(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((unsigned int *)i1) >= *((unsigned int *)i2);
    }
}
static void LONG_greater_equal(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((long *)i1) >= *((long *)i2);
    }
}
static void FLOAT_greater_equal(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((float *)i1) >= *((float *)i2);
    }
}
static void DOUBLE_greater_equal(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((double *)i1) >= *((double *)i2);
    }
}
static void UBYTE_less(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((unsigned char *)i1) < *((unsigned char *)i2);
    }
}
static void SBYTE_less(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((signed char *)i1) < *((signed char *)i2);
    }
}
static void SHORT_less(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((short *)i1) < *((short *)i2);
    }
}
static void USHORT_less(char **args, int *dimensions, int *steps, void *func) {
  int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
  char *i1=args[0], *i2=args[1], *op=args[2];
  for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
    *((long *)op)=*((unsigned short *)i1) < *((unsigned short *)i2);
  }
}
static void INT_less(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((int *)i1) < *((int *)i2);
    }
}
static void UINT_less(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((unsigned int *)i1) < *((unsigned int *)i2);
    }
}
static void LONG_less(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((long *)i1) < *((long *)i2);
    }
}
static void FLOAT_less(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((float *)i1) < *((float *)i2);
    }
}
static void DOUBLE_less(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((double *)i1) < *((double *)i2);
    }
}
static void UBYTE_less_equal(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((unsigned char *)i1) <= *((unsigned char *)i2);
    }
}
static void SBYTE_less_equal(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((signed char *)i1) <= *((signed char *)i2);
    }
}
static void SHORT_less_equal(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((short *)i1) <= *((short *)i2);
    }
}
static void USHORT_less_equal(char **args, int *dimensions, int *steps, void *func) {
  int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
  char *i1=args[0], *i2=args[1], *op=args[2];
  for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
    *((long *)op)=*((unsigned short *)i1) <= *((unsigned short *)i2);
  }
}
static void INT_less_equal(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((int *)i1) <= *((int *)i2);
    }
}
static void UINT_less_equal(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((unsigned int *)i1) <= *((unsigned int *)i2);
    }
}
static void LONG_less_equal(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((long *)i1) <= *((long *)i2);
    }
}
static void FLOAT_less_equal(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((float *)i1) <= *((float *)i2);
    }
}
static void DOUBLE_less_equal(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((double *)i1) <= *((double *)i2);
    }
}
static void CHAR_equal(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((char *)i1) == *((char *)i2);
    }
}
static void UBYTE_equal(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((unsigned char *)i1) == *((unsigned char *)i2);
    }
}
static void SBYTE_equal(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((signed char *)i1) == *((signed char *)i2);
    }
}
static void SHORT_equal(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((short *)i1) == *((short *)i2);
    }
}
static void USHORT_equal(char **args, int *dimensions, int *steps, void *func) {
  int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
  char *i1=args[0], *i2=args[1], *op=args[2];
  for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
    *((long *)op)=*((unsigned short *)i1) == *((unsigned short *)i2);
  }
}
static void INT_equal(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((int *)i1) == *((int *)i2);
    }
}
static void UINT_equal(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((unsigned int *)i1) == *((unsigned int *)i2);
    }
}
static void LONG_equal(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((long *)i1) == *((long *)i2);
    }
}
static void FLOAT_equal(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((float *)i1) == *((float *)i2);
    }
}
static void DOUBLE_equal(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((double *)i1) == *((double *)i2);
    }
}
static void CFLOAT_equal(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
      *((long *)op)=(((float *)i1)[0] == ((float *)i2)[0]) && (((float *)i1)[1] == ((float *)i2)[1]);
    }
}
static void CDOUBLE_equal(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
      *((long *)op)=(((double *)i1)[0] == ((double *)i2)[0]) && (((double *)i1)[1] == ((double *)i2)[1]);
    }
}
static void OBJECT_equal(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
      *((long *)op)=(PyObject_Compare(*((PyObject **)i1),*((PyObject **)i2)) == 0);
    }
}
static void CHAR_not_equal(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((char *)i1) != *((char *)i2);
    }
}
static void UBYTE_not_equal(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((unsigned char *)i1) != *((unsigned char *)i2);
    }
}
static void SBYTE_not_equal(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((signed char *)i1) != *((signed char *)i2);
    }
}
static void SHORT_not_equal(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((short *)i1) != *((short *)i2);
    }
}
static void USHORT_not_equal(char **args, int *dimensions, int *steps, void *func) {
  int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
  char *i1=args[0], *i2=args[1], *op=args[2];
  for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
    *((long *)op)=*((unsigned short *)i1) != *((unsigned short *)i2);
  }
}
static void INT_not_equal(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((int *)i1) != *((int *)i2);
    }
}
static void UINT_not_equal(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((unsigned int *)i1) != *((unsigned int *)i2);
    }
}
static void LONG_not_equal(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((long *)i1) != *((long *)i2);
    }
}
static void FLOAT_not_equal(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((float *)i1) != *((float *)i2);
    }
}
static void DOUBLE_not_equal(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((double *)i1) != *((double *)i2);
    }
}
static void CFLOAT_not_equal(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
      *((long *)op)=(((float *)i1)[0] != ((float *)i2)[0]) || (((float *)i1)[1] != ((float *)i2)[1]);
    }
}
static void CDOUBLE_not_equal(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
      *((long *)op)=(((double *)i1)[0] != ((double *)i2)[0]) || (((double *)i1)[1] != ((double *)i2)[1]);
    }
}
static void OBJECT_not_equal(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
      *((long *)op)=(PyObject_Compare(*((PyObject **)i1),*((PyObject **)i2)) != 0);
    }
}
static void UBYTE_logical_and(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((unsigned char *)op)=*((unsigned char *)i1) && *((unsigned char *)i2);
    }
}
static void SBYTE_logical_and(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((signed char *)op)=*((signed char *)i1) && *((signed char *)i2);
    }
}
static void SHORT_logical_and(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((short *)op)=*((short *)i1) && *((short *)i2);
    }
}
static void USHORT_logical_and(char **args, int *dimensions, int *steps, void *func) {
  int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
  char *i1=args[0], *i2=args[1], *op=args[2];
  for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
    *((unsigned short *)op)=*((unsigned short *)i1) && *((unsigned short *)i2);
  }
}
static void INT_logical_and(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((int *)op)=*((int *)i1) && *((int *)i2);
    }
}
static void UINT_logical_and(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((unsigned int *)op)=*((unsigned int *)i1) && *((unsigned int *)i2);
    }
}
static void LONG_logical_and(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((long *)i1) && *((long *)i2);
    }
}
static void FLOAT_logical_and(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((float *)op)=(float)(*((float *)i1) && *((float *)i2));
    }
}
static void DOUBLE_logical_and(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((double *)op)=(double)(*((double *)i1) && *((double *)i2));
    }
}
static void UBYTE_logical_or(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((unsigned char *)op)=*((unsigned char *)i1) || *((unsigned char *)i2);
    }
}
static void SBYTE_logical_or(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((signed char *)op)=*((signed char *)i1) || *((signed char *)i2);
    }
}
static void SHORT_logical_or(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((short *)op)=*((short *)i1) || *((short *)i2);
    }
}
static void USHORT_logical_or(char **args, int *dimensions, int *steps, void *func) {
  int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
  char *i1=args[0], *i2=args[1], *op=args[2];
  for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
    *((unsigned short *)op)=*((unsigned short *)i1) || *((unsigned short *)i2);
  }
}
static void INT_logical_or(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((int *)op)=*((int *)i1) || *((int *)i2);
    }
}
static void UINT_logical_or(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((unsigned int *)op)=*((unsigned int *)i1) || *((unsigned int *)i2);
    }
}
static void LONG_logical_or(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((long *)i1) || *((long *)i2);
    }
}
static void FLOAT_logical_or(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((float *)op)=(float)(*((float *)i1) || *((float *)i2));
    }
}
static void DOUBLE_logical_or(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((double *)op)=*((double *)i1) || *((double *)i2);
    }
}
static void UBYTE_logical_xor(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((unsigned char *)op)=(*((unsigned char *)i1) || *((unsigned char *)i2)) && !(*((unsigned char *)i1) && *((unsigned char *)i2));
    }
}
static void SBYTE_logical_xor(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((signed char *)op)=(*((signed char *)i1) || *((signed char *)i2)) && !(*((signed char *)i1) && *((signed char *)i2));
    }
}
static void SHORT_logical_xor(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((short *)op)=(*((short *)i1) || *((short *)i2)) && !(*((short *)i1) && *((short *)i2));
    }
}
static void USHORT_logical_xor(char **args, int *dimensions, int *steps, void *func) {
  int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
  char *i1=args[0], *i2=args[1], *op=args[2];
  for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
    *((unsigned short *)op)=(*((unsigned short *)i1) || *((unsigned short *)i2)) && !(*((unsigned short *)i1) && *((unsigned short *)i2));
  }
}
static void INT_logical_xor(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((int *)op)=(*((int *)i1) || *((int *)i2)) && !(*((int *)i1) && *((int *)i2));
    }
}
static void UINT_logical_xor(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((unsigned int *)op)=(*((unsigned int *)i1) || *((unsigned int *)i2)) && !(*((unsigned int *)i1) && *((unsigned int *)i2));
    }
}
static void LONG_logical_xor(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=(*((long *)i1) || *((long *)i2)) && !(*((long *)i1) && *((long *)i2));
    }
}
static void FLOAT_logical_xor(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((float *)op)=(float)((*((float *)i1) || *((float *)i2)) && !(*((float *)i1) && *((float *)i2)));
    }
}
static void DOUBLE_logical_xor(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((double *)op)=(*((double *)i1) || *((double *)i2)) && !(*((double *)i1) && *((double *)i2));
    }
}
static void UBYTE_logical_not(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((unsigned char *)op)=!*((unsigned char *)i1);}}
static void SBYTE_logical_not(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((signed char *)op)=!*((signed char *)i1);}}
static void SHORT_logical_not(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((short *)op)=!*((short *)i1);}}
static void USHORT_logical_not(char **args, int *dimensions, int *steps, void *func) 
  {int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((unsigned short *)op)=!*((unsigned short *)i1);}}
static void INT_logical_not(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((int *)op)=!*((int *)i1);}}
static void UINT_logical_not(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((unsigned int *)op)=!*((unsigned int *)i1);}}
static void LONG_logical_not(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((long *)op)=!*((long *)i1);}}
static void FLOAT_logical_not(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((float *)op)=(float)(!*((float *)i1));}}
static void DOUBLE_logical_not(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((double *)op)=!*((double *)i1);}}
static void UBYTE_maximum(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((unsigned char *)op)=*((unsigned char *)i1) > *((unsigned char *)i2) ? *((unsigned char *)i1) : *((unsigned char *)i2);
    }
}
static void SBYTE_maximum(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((signed char *)op)=*((signed char *)i1) > *((signed char *)i2) ? *((signed char *)i1) : *((signed char *)i2);
    }
}
static void SHORT_maximum(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((short *)op)=*((short *)i1) > *((short *)i2) ? *((short *)i1) : *((short *)i2);
    }
}
static void USHORT_maximum(char **args, int *dimensions, int *steps, void *func) {
  int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
  char *i1=args[0], *i2=args[1], *op=args[2];
  for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
    *((unsigned short *)op)=*((unsigned short *)i1) > *((unsigned short *)i2) ? *((unsigned short *)i1) : *((unsigned short *)i2);
  }
}
static void INT_maximum(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((int *)op)=*((int *)i1) > *((int *)i2) ? *((int *)i1) : *((int *)i2);
    }
}
static void UINT_maximum(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((unsigned int *)op)=*((unsigned int *)i1) > *((unsigned int *)i2) ? *((unsigned int *)i1) : *((unsigned int *)i2);
    }
}
static void LONG_maximum(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((long *)i1) > *((long *)i2) ? *((long *)i1) : *((long *)i2);
    }
}
static void FLOAT_maximum(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((float *)op)=*((float *)i1) > *((float *)i2) ? *((float *)i1) : *((float *)i2);
    }
}
static void DOUBLE_maximum(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((double *)op)=*((double *)i1) > *((double *)i2) ? *((double *)i1) : *((double *)i2);
    }
}
static void UBYTE_minimum(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((unsigned char *)op)=*((unsigned char *)i1) < *((unsigned char *)i2) ? *((unsigned char *)i1) : *((unsigned char *)i2);
    }
}
static void SBYTE_minimum(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((signed char *)op)=*((signed char *)i1) < *((signed char *)i2) ? *((signed char *)i1) : *((signed char *)i2);
    }
}
static void SHORT_minimum(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((short *)op)=*((short *)i1) < *((short *)i2) ? *((short *)i1) : *((short *)i2);
    }
}
static void USHORT_minimum(char **args, int *dimensions, int *steps, void *func) {
  int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
  char *i1=args[0], *i2=args[1], *op=args[2];
  for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
    *((unsigned short *)op)=*((unsigned short *)i1) < *((unsigned short *)i2) ? *((unsigned short *)i1) : *((unsigned short *)i2);
  }
}
static void INT_minimum(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((int *)op)=*((int *)i1) < *((int *)i2) ? *((int *)i1) : *((int *)i2);
    }
}
static void UINT_minimum(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((unsigned int *)op)=*((unsigned int *)i1) < *((unsigned int *)i2) ? *((unsigned int *)i1) : *((unsigned int *)i2);
    }
}
static void LONG_minimum(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((long *)i1) < *((long *)i2) ? *((long *)i1) : *((long *)i2);
    }
}
static void FLOAT_minimum(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((float *)op)=*((float *)i1) < *((float *)i2) ? *((float *)i1) : *((float *)i2);
    }
}
static void DOUBLE_minimum(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((double *)op)=*((double *)i1) < *((double *)i2) ? *((double *)i1) : *((double *)i2);
    }
}
static void UBYTE_bitwise_and(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((unsigned char *)op)=*((unsigned char *)i1) & *((unsigned char *)i2);
    }
}
static void SBYTE_bitwise_and(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((signed char *)op)=*((signed char *)i1) & *((signed char *)i2);
    }
}
static void SHORT_bitwise_and(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((short *)op)=*((short *)i1) & *((short *)i2);
    }
}
static void USHORT_bitwise_and(char **args, int *dimensions, int *steps, void *func) {
  int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
  char *i1=args[0], *i2=args[1], *op=args[2];
  for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
    *((unsigned short *)op)=*((unsigned short *)i1) & *((unsigned short *)i2);
  }
}
static void INT_bitwise_and(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((int *)op)=*((int *)i1) & *((int *)i2);
    }
}
static void UINT_bitwise_and(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((unsigned int *)op)=*((unsigned int *)i1) & *((unsigned int *)i2);
    }
}
static void LONG_bitwise_and(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((long *)i1) & *((long *)i2);
    }
}
static void UBYTE_bitwise_or(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((unsigned char *)op)=*((unsigned char *)i1) | *((unsigned char *)i2);
    }
}
static void SBYTE_bitwise_or(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((signed char *)op)=*((signed char *)i1) | *((signed char *)i2);
    }
}
static void SHORT_bitwise_or(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((short *)op)=*((short *)i1) | *((short *)i2);
    }
}
static void USHORT_bitwise_or(char **args, int *dimensions, int *steps, void *func) {
  int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
  char *i1=args[0], *i2=args[1], *op=args[2];
  for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
    *((unsigned short *)op)=*((unsigned short *)i1) | *((unsigned short *)i2);
  }
}
static void INT_bitwise_or(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((int *)op)=*((int *)i1) | *((int *)i2);
    }
}
static void UINT_bitwise_or(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((unsigned int *)op)=*((unsigned int *)i1) | *((unsigned int *)i2);
    }
}
static void LONG_bitwise_or(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((long *)i1) | *((long *)i2);
    }
}
static void UBYTE_bitwise_xor(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((unsigned char *)op)=*((unsigned char *)i1) ^ *((unsigned char *)i2);
    }
}
static void SBYTE_bitwise_xor(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((signed char *)op)=*((signed char *)i1) ^ *((signed char *)i2);
    }
}
static void SHORT_bitwise_xor(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((short *)op)=*((short *)i1) ^ *((short *)i2);
    }
}
static void USHORT_bitwise_xor(char **args, int *dimensions, int *steps, void *func) {
  int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
  char *i1=args[0], *i2=args[1], *op=args[2];
  for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
    *((unsigned short *)op)=*((unsigned short *)i1) ^ *((unsigned short *)i2);
  }
}
static void INT_bitwise_xor(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((int *)op)=*((int *)i1) ^ *((int *)i2);
    }
}
static void UINT_bitwise_xor(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((unsigned int *)op)=*((unsigned int *)i1) ^ *((unsigned int *)i2);
    }
}
static void LONG_bitwise_xor(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((long *)i1) ^ *((long *)i2);
    }
}
static void UBYTE_invert(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((unsigned char *)op)=~*((unsigned char *)i1);}}
static void SBYTE_invert(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((signed char *)op)=~*((signed char *)i1);}}
static void SHORT_invert(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((short *)op)=~*((short *)i1);}}
static void USHORT_invert(char **args, int *dimensions, int *steps, void *func) 
  {int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((unsigned short *)op)=~*((unsigned short *)i1);}}
static void INT_invert(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((int *)op)=~*((int *)i1);}}
static void UINT_invert(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((unsigned int *)op)=~*((unsigned int *)i1);}}
static void LONG_invert(char **args, int *dimensions, int *steps, void *func) 
{int i; char *i1=args[0], *op=args[1]; for(i=0; i<*dimensions; i++, i1+=steps[0], op+=steps[1]) {*((long *)op)=~*((long *)i1);}}
static void UBYTE_left_shift(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((unsigned char *)op)=*((unsigned char *)i1) << *((unsigned char *)i2);
    }
}
static void SBYTE_left_shift(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((signed char *)op)=*((signed char *)i1) << *((signed char *)i2);
    }
}
static void SHORT_left_shift(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((short *)op)=*((short *)i1) << *((short *)i2);
    }
}
static void USHORT_left_shift(char **args, int *dimensions, int *steps, void *func) {
  int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
  char *i1=args[0], *i2=args[1], *op=args[2];
  for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
    *((unsigned short *)op)=*((unsigned short *)i1) << *((unsigned short *)i2);
  }
}
static void INT_left_shift(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((int *)op)=*((int *)i1) << *((int *)i2);
    }
}
static void UINT_left_shift(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((unsigned int *)op)=*((unsigned int *)i1) << *((unsigned int *)i2);
    }
}
static void LONG_left_shift(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((long *)i1) << *((long *)i2);
    }
}
static void UBYTE_right_shift(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((unsigned char *)op)=*((unsigned char *)i1) >> *((unsigned char *)i2);
    }
}
static void SBYTE_right_shift(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((signed char *)op)=*((signed char *)i1) >> *((signed char *)i2);
    }
}
static void SHORT_right_shift(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((short *)op)=*((short *)i1) >> *((short *)i2);
    }
}
static void USHORT_right_shift(char **args, int *dimensions, int *steps, void *func) {
  int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
  char *i1=args[0], *i2=args[1], *op=args[2];
  for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
    *((unsigned short *)op)=*((unsigned short *)i1) >> *((unsigned short *)i2);
  }
}
static void INT_right_shift(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((int *)op)=*((int *)i1) >> *((int *)i2);
    }
}
static void UINT_right_shift(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((unsigned int *)op)=*((unsigned int *)i1) >> *((unsigned int *)i2);
    }
}
static void LONG_right_shift(char **args, int *dimensions, int *steps, void *func) {
    int i, is1=steps[0],is2=steps[1],os=steps[2], n=dimensions[0];
    char *i1=args[0], *i2=args[1], *op=args[2];
    for(i=0; i<n; i++, i1+=is1, i2+=is2, op+=os) {
	*((long *)op)=*((long *)i1) >> *((long *)i2);
    }
}
static PyUFuncGenericFunction add_functions[] = { UBYTE_add,  SBYTE_add,  SHORT_add,  USHORT_add, INT_add,  UINT_add, LONG_add,  FLOAT_add,  DOUBLE_add,  CFLOAT_add,  CDOUBLE_add,  NULL,  };
static PyUFuncGenericFunction subtract_functions[] = { UBYTE_subtract,  SBYTE_subtract,  SHORT_subtract,  USHORT_subtract, INT_subtract, UINT_subtract, LONG_subtract,  FLOAT_subtract,  DOUBLE_subtract,  CFLOAT_subtract,  CDOUBLE_subtract,  NULL,  };
static PyUFuncGenericFunction multiply_functions[] = { UBYTE_multiply,  SBYTE_multiply,  SHORT_multiply,  USHORT_multiply, INT_multiply,  UINT_multiply, LONG_multiply,  FLOAT_multiply,  DOUBLE_multiply,  NULL,  NULL,  NULL,  };
static PyUFuncGenericFunction divide_functions[] = { UBYTE_divide,  SBYTE_divide,  SHORT_divide,  USHORT_divide, INT_divide,  UINT_divide, LONG_divide,  FLOAT_divide,  DOUBLE_divide,  NULL,  NULL,  NULL,  };
static PyUFuncGenericFunction floor_divide_functions[] = { UBYTE_floor_divide,  SBYTE_floor_divide,  SHORT_floor_divide,  USHORT_floor_divide, INT_floor_divide,  UINT_floor_divide, LONG_floor_divide,  FLOAT_floor_divide,  DOUBLE_floor_divide,  NULL,  NULL,  NULL,  };
static PyUFuncGenericFunction true_divide_functions[] = { UBYTE_true_divide,  SBYTE_true_divide,  SHORT_true_divide,  USHORT_true_divide, INT_true_divide,  UINT_true_divide, LONG_true_divide,  FLOAT_true_divide,  DOUBLE_true_divide,  NULL,  NULL,  NULL,  };
static PyUFuncGenericFunction divide_safe_functions[] = { UBYTE_divide_safe,  SBYTE_divide_safe,  SHORT_divide_safe,  USHORT_divide_safe, INT_divide_safe,  UINT_divide_safe, LONG_divide_safe,  FLOAT_divide_safe,  DOUBLE_divide_safe,  };
static PyUFuncGenericFunction conjugate_functions[] = { UBYTE_conjugate,  SBYTE_conjugate,  SHORT_conjugate,  USHORT_conjugate, INT_conjugate,  UINT_conjugate, LONG_conjugate,  FLOAT_conjugate,  DOUBLE_conjugate,  CFLOAT_conjugate,  CDOUBLE_conjugate,  NULL,  };
static PyUFuncGenericFunction remainder_functions[] = { UBYTE_remainder,  SBYTE_remainder,  SHORT_remainder,  USHORT_remainder, INT_remainder,  UINT_remainder, LONG_remainder,  NULL,  NULL,  NULL,  };
static PyUFuncGenericFunction power_functions[] = { UBYTE_power,  SBYTE_power,  SHORT_power,  USHORT_power, INT_power, UINT_power, LONG_power,  NULL,  NULL,  NULL,  NULL,  NULL,  };
static PyUFuncGenericFunction absolute_functions[] = { UBYTE_absolute, SBYTE_absolute,  SHORT_absolute,  USHORT_absolute, INT_absolute, UINT_absolute, LONG_absolute,  FLOAT_absolute,  DOUBLE_absolute,  CFLOAT_absolute,  CDOUBLE_absolute,  NULL,  };
static PyUFuncGenericFunction negative_functions[] = { UBYTE_negative, SBYTE_negative,  SHORT_negative,  USHORT_negative, INT_negative, UINT_negative, LONG_negative,  FLOAT_negative,  DOUBLE_negative,  CFLOAT_negative,  CDOUBLE_negative,  NULL,  };
static PyUFuncGenericFunction greater_functions[] = { UBYTE_greater,  SBYTE_greater,  SHORT_greater,  USHORT_greater, INT_greater,  UINT_greater, LONG_greater,  FLOAT_greater,  DOUBLE_greater,  };
static PyUFuncGenericFunction greater_equal_functions[] = { UBYTE_greater_equal,  SBYTE_greater_equal,  SHORT_greater_equal,  USHORT_greater_equal, INT_greater_equal,  UINT_greater_equal, LONG_greater_equal,  FLOAT_greater_equal,  DOUBLE_greater_equal,  };
static PyUFuncGenericFunction less_functions[] = { UBYTE_less,  SBYTE_less,  SHORT_less,  USHORT_less, INT_less,  UINT_less, LONG_less,  FLOAT_less,  DOUBLE_less,  };
static PyUFuncGenericFunction less_equal_functions[] = { UBYTE_less_equal,  SBYTE_less_equal,  SHORT_less_equal,  USHORT_less_equal, INT_less_equal,  UINT_less_equal, LONG_less_equal,  FLOAT_less_equal,  DOUBLE_less_equal,  };
static PyUFuncGenericFunction equal_functions[] = { CHAR_equal, UBYTE_equal,  SBYTE_equal,  SHORT_equal,  USHORT_equal, INT_equal,  UINT_equal, LONG_equal,  FLOAT_equal,  DOUBLE_equal, CFLOAT_equal, CDOUBLE_equal, OBJECT_equal};
static PyUFuncGenericFunction not_equal_functions[] = { CHAR_not_equal, UBYTE_not_equal,  SBYTE_not_equal,  SHORT_not_equal, USHORT_not_equal, INT_not_equal,  UINT_not_equal, LONG_not_equal,  FLOAT_not_equal,  DOUBLE_not_equal,  CFLOAT_not_equal, CDOUBLE_not_equal, OBJECT_not_equal};
static PyUFuncGenericFunction logical_and_functions[] = { UBYTE_logical_and,  SBYTE_logical_and,  SHORT_logical_and, USHORT_logical_and, INT_logical_and,  UINT_logical_and, LONG_logical_and,  FLOAT_logical_and,  DOUBLE_logical_and,  };
static PyUFuncGenericFunction logical_or_functions[] = { UBYTE_logical_or,  SBYTE_logical_or,  SHORT_logical_or,  USHORT_logical_or, INT_logical_or,  UINT_logical_or, LONG_logical_or,  FLOAT_logical_or,  DOUBLE_logical_or,  };
static PyUFuncGenericFunction logical_xor_functions[] = { UBYTE_logical_xor,  SBYTE_logical_xor,  SHORT_logical_xor,  USHORT_logical_xor, INT_logical_xor,  UINT_logical_xor, LONG_logical_xor,  FLOAT_logical_xor,  DOUBLE_logical_xor,  };
static PyUFuncGenericFunction logical_not_functions[] = { UBYTE_logical_not,  SBYTE_logical_not,  SHORT_logical_not,  USHORT_logical_not, INT_logical_not,  UINT_logical_not, LONG_logical_not,  FLOAT_logical_not,  DOUBLE_logical_not,  };
static PyUFuncGenericFunction maximum_functions[] = { UBYTE_maximum,  SBYTE_maximum,  SHORT_maximum,  USHORT_maximum, INT_maximum,  UINT_maximum, LONG_maximum,  FLOAT_maximum,  DOUBLE_maximum,  };
static PyUFuncGenericFunction minimum_functions[] = { UBYTE_minimum,  SBYTE_minimum,  SHORT_minimum,  USHORT_minimum, INT_minimum,  UINT_minimum, LONG_minimum,  FLOAT_minimum,  DOUBLE_minimum,  };
static PyUFuncGenericFunction bitwise_and_functions[] = { UBYTE_bitwise_and,  SBYTE_bitwise_and,  SHORT_bitwise_and,  USHORT_bitwise_and,  INT_bitwise_and,  UINT_bitwise_and, LONG_bitwise_and,  NULL,  };
static PyUFuncGenericFunction bitwise_or_functions[] = { UBYTE_bitwise_or,  SBYTE_bitwise_or,  SHORT_bitwise_or,  USHORT_bitwise_or,  INT_bitwise_or,  UINT_bitwise_or, LONG_bitwise_or,  NULL,  };
static PyUFuncGenericFunction bitwise_xor_functions[] = { UBYTE_bitwise_xor,  SBYTE_bitwise_xor,  SHORT_bitwise_xor,  USHORT_bitwise_xor, INT_bitwise_xor,  UINT_bitwise_xor, LONG_bitwise_xor,  NULL,  };
static PyUFuncGenericFunction invert_functions[] = { UBYTE_invert,  SBYTE_invert,  SHORT_invert,  USHORT_invert, INT_invert,  UINT_invert, LONG_invert,  NULL,  };
static PyUFuncGenericFunction left_shift_functions[] = { UBYTE_left_shift,  SBYTE_left_shift,  SHORT_left_shift,  USHORT_left_shift,  INT_left_shift,  UINT_left_shift, LONG_left_shift,  NULL,  };
static PyUFuncGenericFunction right_shift_functions[] = { UBYTE_right_shift,  SBYTE_right_shift,  SHORT_right_shift,  USHORT_right_shift,  INT_right_shift,  UINT_right_shift, LONG_right_shift,  NULL,  };

static PyUFuncGenericFunction arccos_functions[] = { NULL,  NULL,  NULL,  NULL,  NULL,  };
static PyUFuncGenericFunction ceil_functions[] = { NULL,  NULL,  NULL,  };
static PyUFuncGenericFunction arctan2_functions[] = { NULL,  NULL,  NULL,  };

static void * add_data[] = { (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL, (void *)NULL, (void *)NULL};
static void * subtract_data[] = { (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL, (void *)NULL};
static void * multiply_data[] = { (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL, (void *)NULL};
static void * divide_data[] = { (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL, (void *)NULL};
static void * floor_divide_data[] = { (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL, (void *)NULL};
static void * true_divide_data[] = { (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL, (void *)NULL };
static void * divide_safe_data[] = { (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL, (void *)NULL };
static void * conjugate_data[] = { (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL };
static void * remainder_data[] = { (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL, (void *)NULL, };
static void * power_data[] = { (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,};
static void * absolute_data[] = { (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL, (void *)NULL, (void *)NULL};
static void * negative_data[] = { (void *)NULL, (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,};
static void * equal_data[] = { (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL, (void *)NULL, }; 
static void * bitwise_and_data[] = { (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL, };
static void * bitwise_or_data[] = { (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL, (void *)NULL, };
static void * bitwise_xor_data[] = { (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL, };
static void * invert_data[] = { (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL, (void *)NULL,};
static void * left_shift_data[] = { (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,};
static void * right_shift_data[] = { (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,  (void *)NULL,};

static void * arccos_data[] = { (void *)acos,  (void *)acos,  (void *)c_acos,  (void *)c_acos,  (void *)"arccos",  };
static void * arcsin_data[] = { (void *)asin,  (void *)asin,  (void *)c_asin,  (void *)c_asin,  (void *)"arcsin",  };
static void * arctan_data[] = { (void *)atan,  (void *)atan,  (void *)c_atan,  (void *)c_atan,  (void *)"arctan",  };
static void * arccosh_data[] = { (void *)acosh,  (void *)acosh,  (void *)c_acosh,  (void *)c_acosh,  (void *)"arccosh",  };
static void * arcsinh_data[] = { (void *)asinh,  (void *)asinh,  (void *)c_asinh,  (void *)c_asinh,  (void *)"arcsinh",  };
static void * arctanh_data[] = { (void *)atanh,  (void *)atanh,  (void *)c_atanh,  (void *)c_atanh,  (void *)"arctanh",  };
static void * cos_data[] = { (void *)cos,  (void *)cos,  (void *)c_cos,  (void *)c_cos,  (void *)"cos",  };
static void * cosh_data[] = { (void *)cosh,  (void *)cosh,  (void *)c_cosh,  (void *)c_cosh,  (void *)"cosh",  };
static void * exp_data[] = { (void *)exp,  (void *)exp,  (void *)c_exp,  (void *)c_exp,  (void *)"exp",  };
static void * log_data[] = { (void *)log,  (void *)log,  (void *)c_log,  (void *)c_log,  (void *)"log",  };
static void * log10_data[] = { (void *)log10,  (void *)log10,  (void *)c_log10,  (void *)c_log10,  (void *)"log10",  };
static void * sin_data[] = { (void *)sin,  (void *)sin,  (void *)c_sin,  (void *)c_sin,  (void *)"sin",  };
static void * sinh_data[] = { (void *)sinh,  (void *)sinh,  (void *)c_sinh,  (void *)c_sinh,  (void *)"sinh",  };
static void * sqrt_data[] = { (void *)sqrt,  (void *)sqrt,  (void *)c_sqrt,  (void *)c_sqrt,  (void *)"sqrt",  };
static void * tan_data[] = { (void *)tan,  (void *)tan,  (void *)c_tan,  (void *)c_tan,  (void *)"tan",  };
static void * tanh_data[] = { (void *)tanh,  (void *)tanh,  (void *)c_tanh,  (void *)c_tanh,  (void *)"tanh",  };
static void * ceil_data[] = { (void *)ceil,  (void *)ceil,  (void *)"ceil",  };
static void * fabs_data[] = { (void *)fabs,  (void *)fabs,  (void *)"fabs",  };
static void * floor_data[] = { (void *)floor,  (void *)floor,  (void *)"floor",  };
static void * arctan2_data[] = { (void *)atan2,  (void *)atan2,  (void *)"arctan2",  };
static void * fmod_data[] = { (void *)fmod,  (void *)fmod,  (void *)"fmod",  };
static void * hypot_data[] = { (void *)hypot,  (void *)hypot,  (void *)"hypot",  };

static char add_signatures[] = { PyArray_UBYTE,  PyArray_UBYTE,  PyArray_UBYTE,  PyArray_SBYTE,  PyArray_SBYTE,  PyArray_SBYTE,  PyArray_SHORT,  PyArray_SHORT,  PyArray_SHORT,  PyArray_USHORT,  PyArray_USHORT,  PyArray_USHORT,  PyArray_INT,  PyArray_INT,  PyArray_INT,  PyArray_UINT, PyArray_UINT, PyArray_UINT, PyArray_LONG,  PyArray_LONG,  PyArray_LONG,  PyArray_FLOAT,  PyArray_FLOAT,  PyArray_FLOAT,  PyArray_DOUBLE,  PyArray_DOUBLE,  PyArray_DOUBLE,  PyArray_CFLOAT,  PyArray_CFLOAT,  PyArray_CFLOAT,  PyArray_CDOUBLE,  PyArray_CDOUBLE,  PyArray_CDOUBLE,  PyArray_OBJECT,  PyArray_OBJECT,  PyArray_OBJECT,  };
static char floor_divide_signatures[] = { PyArray_UBYTE,  PyArray_UBYTE,  PyArray_UBYTE,  PyArray_SBYTE,  PyArray_SBYTE,  PyArray_SBYTE,  PyArray_SHORT,  PyArray_SHORT,  PyArray_SHORT,  PyArray_USHORT,  PyArray_USHORT,  PyArray_USHORT,  PyArray_INT,  PyArray_INT,  PyArray_INT, PyArray_UINT, PyArray_UINT, PyArray_UINT, PyArray_LONG,  PyArray_LONG,  PyArray_LONG,  PyArray_FLOAT,  PyArray_FLOAT,  PyArray_FLOAT,  PyArray_DOUBLE,  PyArray_DOUBLE,  PyArray_DOUBLE,  };
static char true_divide_signatures[] = { PyArray_UBYTE,  PyArray_UBYTE,  PyArray_FLOAT,  PyArray_SBYTE,  PyArray_SBYTE,  PyArray_FLOAT,  PyArray_SHORT,  PyArray_SHORT, PyArray_FLOAT,  PyArray_USHORT,  PyArray_USHORT,  PyArray_FLOAT,  PyArray_INT,  PyArray_INT,  PyArray_DOUBLE,  PyArray_UINT, PyArray_UINT, PyArray_FLOAT, PyArray_LONG,  PyArray_LONG,  PyArray_DOUBLE,  PyArray_FLOAT,  PyArray_FLOAT,  PyArray_FLOAT,  PyArray_DOUBLE,  PyArray_DOUBLE,  PyArray_DOUBLE,  PyArray_CFLOAT,  PyArray_CFLOAT,  PyArray_CFLOAT,  PyArray_CDOUBLE,  PyArray_CDOUBLE,  PyArray_CDOUBLE,  PyArray_OBJECT,  PyArray_OBJECT,  PyArray_OBJECT,  };
static char divide_safe_signatures[] = { PyArray_UBYTE,  PyArray_UBYTE,  PyArray_UBYTE,  PyArray_SBYTE,  PyArray_SBYTE,  PyArray_SBYTE,  PyArray_SHORT,  PyArray_SHORT,  PyArray_SHORT, PyArray_USHORT,  PyArray_USHORT,  PyArray_USHORT,  PyArray_INT,  PyArray_INT,  PyArray_INT,  PyArray_UINT, PyArray_UINT, PyArray_UINT,  PyArray_LONG,  PyArray_LONG,  PyArray_LONG,  PyArray_FLOAT,  PyArray_FLOAT,  PyArray_FLOAT,  PyArray_DOUBLE,  PyArray_DOUBLE,  PyArray_DOUBLE,  };
static char conjugate_signatures[] = { PyArray_UBYTE,  PyArray_UBYTE,  PyArray_SBYTE,  PyArray_SBYTE,  PyArray_SHORT,  PyArray_SHORT,  PyArray_USHORT,  PyArray_USHORT,  PyArray_INT,  PyArray_INT,  PyArray_UINT, PyArray_UINT, PyArray_LONG,  PyArray_LONG,  PyArray_FLOAT,  PyArray_FLOAT,  PyArray_DOUBLE,  PyArray_DOUBLE,  PyArray_CFLOAT,  PyArray_CFLOAT,  PyArray_CDOUBLE,  PyArray_CDOUBLE,  PyArray_OBJECT,  PyArray_OBJECT,  };
static char remainder_signatures[] = { PyArray_UBYTE,  PyArray_UBYTE,  PyArray_UBYTE,  PyArray_SBYTE,  PyArray_SBYTE,  PyArray_SBYTE,  PyArray_SHORT,  PyArray_SHORT,  PyArray_SHORT, PyArray_USHORT,  PyArray_USHORT,  PyArray_USHORT,  PyArray_INT,  PyArray_INT,  PyArray_INT,  PyArray_UINT, PyArray_UINT, PyArray_UINT,  PyArray_LONG,  PyArray_LONG,  PyArray_LONG,  PyArray_FLOAT,  PyArray_FLOAT,  PyArray_FLOAT,  PyArray_DOUBLE,  PyArray_DOUBLE,  PyArray_DOUBLE,  PyArray_OBJECT,  PyArray_OBJECT,  PyArray_OBJECT,  };
static char absolute_signatures[] = { PyArray_UBYTE, PyArray_UBYTE, PyArray_SBYTE,  PyArray_SBYTE,  PyArray_SHORT,  PyArray_SHORT,  PyArray_USHORT,  PyArray_USHORT,  PyArray_INT,  PyArray_INT,  PyArray_UINT, PyArray_UINT,  PyArray_LONG,  PyArray_LONG,  PyArray_FLOAT,  PyArray_FLOAT,  PyArray_DOUBLE,  PyArray_DOUBLE,  PyArray_CFLOAT,  PyArray_FLOAT,  PyArray_CDOUBLE,  PyArray_DOUBLE,  PyArray_OBJECT,  PyArray_OBJECT,  };
static char negative_signatures[] = { PyArray_UBYTE, PyArray_UBYTE, PyArray_SBYTE,  PyArray_SBYTE,  PyArray_SHORT,  PyArray_SHORT, PyArray_USHORT,  PyArray_USHORT,  PyArray_INT,  PyArray_INT,  PyArray_UINT, PyArray_UINT, PyArray_LONG,  PyArray_LONG,  PyArray_FLOAT,  PyArray_FLOAT,  PyArray_DOUBLE,  PyArray_DOUBLE,  PyArray_CFLOAT,  PyArray_CFLOAT,  PyArray_CDOUBLE,  PyArray_CDOUBLE,  PyArray_OBJECT,  PyArray_OBJECT,  };
static char equal_signatures[] = { PyArray_CHAR, PyArray_CHAR, PyArray_LONG, PyArray_UBYTE,  PyArray_UBYTE,  PyArray_LONG,  PyArray_SBYTE,  PyArray_SBYTE,  PyArray_LONG,  PyArray_SHORT,  PyArray_SHORT,  PyArray_LONG, PyArray_USHORT,  PyArray_USHORT,  PyArray_LONG,  PyArray_INT,  PyArray_INT,  PyArray_LONG, PyArray_UINT, PyArray_UINT, PyArray_LONG,  PyArray_LONG,  PyArray_LONG,  PyArray_LONG,  PyArray_FLOAT,  PyArray_FLOAT,  PyArray_LONG,  PyArray_DOUBLE,  PyArray_DOUBLE,  PyArray_LONG, PyArray_CFLOAT, PyArray_CFLOAT, PyArray_LONG, PyArray_CDOUBLE, PyArray_CDOUBLE, PyArray_LONG, PyArray_OBJECT, PyArray_OBJECT, PyArray_LONG,};
static char greater_signatures[] = { PyArray_UBYTE,  PyArray_UBYTE,  PyArray_LONG,  PyArray_SBYTE,  PyArray_SBYTE,  PyArray_LONG,  PyArray_SHORT,  PyArray_SHORT,  PyArray_LONG, PyArray_USHORT,  PyArray_USHORT,  PyArray_LONG,  PyArray_INT,  PyArray_INT,  PyArray_LONG, PyArray_UINT, PyArray_UINT, PyArray_LONG,  PyArray_LONG,  PyArray_LONG,  PyArray_LONG,  PyArray_FLOAT,  PyArray_FLOAT,  PyArray_LONG,  PyArray_DOUBLE,  PyArray_DOUBLE,  PyArray_LONG,  };
static char logical_not_signatures[] = { PyArray_UBYTE,  PyArray_UBYTE,  PyArray_SBYTE,  PyArray_SBYTE,  PyArray_SHORT,  PyArray_SHORT, PyArray_USHORT,  PyArray_USHORT,  PyArray_INT,  PyArray_INT,  PyArray_UINT, PyArray_UINT, PyArray_LONG,  PyArray_LONG,  PyArray_FLOAT,  PyArray_FLOAT,  PyArray_DOUBLE,  PyArray_DOUBLE,  };
static char bitwise_and_signatures[] = { PyArray_UBYTE,  PyArray_UBYTE,  PyArray_UBYTE,  PyArray_SBYTE,  PyArray_SBYTE,  PyArray_SBYTE,  PyArray_SHORT,  PyArray_SHORT,  PyArray_SHORT, PyArray_USHORT,  PyArray_USHORT,  PyArray_USHORT,  PyArray_INT,  PyArray_INT,  PyArray_INT,  PyArray_UINT, PyArray_UINT, PyArray_UINT, PyArray_LONG,  PyArray_LONG,  PyArray_LONG,  PyArray_OBJECT,  PyArray_OBJECT,  PyArray_OBJECT,  };
static char invert_signatures[] = { PyArray_UBYTE,  PyArray_UBYTE,  PyArray_SBYTE,  PyArray_SBYTE,  PyArray_SHORT,  PyArray_SHORT, PyArray_USHORT,  PyArray_USHORT,  PyArray_INT,  PyArray_INT,  PyArray_UINT, PyArray_UINT,  PyArray_LONG,  PyArray_LONG,  PyArray_OBJECT,  PyArray_OBJECT,  };

static char arccos_signatures[] = { PyArray_FLOAT,  PyArray_FLOAT,  PyArray_DOUBLE,  PyArray_DOUBLE,  PyArray_CFLOAT,  PyArray_CFLOAT,  PyArray_CDOUBLE,  PyArray_CDOUBLE,  PyArray_OBJECT,  PyArray_OBJECT,  };
static char ceil_signatures[] = { PyArray_FLOAT,  PyArray_FLOAT,  PyArray_DOUBLE,  PyArray_DOUBLE,  PyArray_OBJECT,  PyArray_OBJECT,  };
static char arctan2_signatures[] = { PyArray_FLOAT,  PyArray_FLOAT,  PyArray_FLOAT,  PyArray_DOUBLE,  PyArray_DOUBLE,  PyArray_DOUBLE,  PyArray_OBJECT,  PyArray_OBJECT,  };


static void InitOperators(PyObject *dictionary)
{
    PyObject *f;

    add_data[11] =(void *)PyNumber_Add;
    subtract_data[11] = (void *)PyNumber_Subtract;
    multiply_data[9] = (void *)c_prod;
    multiply_data[10] = (void *)c_prod;
    multiply_data[11] = (void *)PyNumber_Multiply;
    divide_data[9] = (void *)c_quot;
    divide_data[10] = (void *)c_quot;
    divide_data[11] = (void *)PyNumber_Divide;

    floor_divide_data[9] = (void *)c_quot;
    floor_divide_data[10] = (void *)c_quot;
    floor_divide_data[11] = (void *)PyNumber_Divide;
    true_divide_data[9] = (void *)c_quot;
    true_divide_data[10] = (void *)c_quot;
    true_divide_data[11] = (void *)PyNumber_Divide;

    conjugate_data[11] = (void *)"conjugate";
    remainder_data[7] = (void *)fmod;
    remainder_data[8] = (void *)fmod; 
    remainder_data[9] = (void *)PyNumber_Remainder;
    power_data[7] = (void *)pow;
    power_data[8] = (void *)pow;
    power_data[9] = (void *)c_pow;
    power_data[10] = (void *)c_pow;
    power_data[11] = (void *)PyNumber_Power;
    absolute_data[11] = (void *)PyNumber_Absolute;
    negative_data[11] = (void *)PyNumber_Negative;
    bitwise_and_data[7] = (void *)PyNumber_And;
    bitwise_or_data[7] = (void *)PyNumber_Or;
    bitwise_xor_data[7] = (void *)PyNumber_Xor;
    invert_data[7] = (void *)PyNumber_Invert;
    left_shift_data[7] = (void *)PyNumber_Lshift;
    right_shift_data[7] = (void *)PyNumber_Rshift;


    add_functions[11] = PyUFunc_OO_O;
    subtract_functions[11] = PyUFunc_OO_O;
    multiply_functions[9] = PyUFunc_FF_F_As_DD_D;
    multiply_functions[10] = PyUFunc_DD_D;
    multiply_functions[11] = PyUFunc_OO_O;
    divide_functions[9] = PyUFunc_FF_F_As_DD_D;
    divide_functions[10] = PyUFunc_DD_D;
    divide_functions[11] = PyUFunc_OO_O;

    true_divide_functions[9] = PyUFunc_FF_F_As_DD_D;
    true_divide_functions[10] = PyUFunc_DD_D;
    true_divide_functions[11] = PyUFunc_OO_O;

    conjugate_functions[11] = PyUFunc_O_O_method;
    remainder_functions[7] = PyUFunc_ff_f_As_dd_d;
    remainder_functions[8] = PyUFunc_dd_d;
    remainder_functions[9] = PyUFunc_OO_O;
    power_functions[7] = PyUFunc_ff_f_As_dd_d;
    power_functions[8] = PyUFunc_dd_d;
    power_functions[9] = PyUFunc_FF_F_As_DD_D;
    power_functions[10] = PyUFunc_DD_D;
    power_functions[11] = PyUFunc_OO_O;
    absolute_functions[11] = PyUFunc_O_O;
    negative_functions[11] = PyUFunc_O_O;
    bitwise_and_functions[7] = PyUFunc_OO_O;
    bitwise_or_functions[7] = PyUFunc_OO_O;
    bitwise_xor_functions[7] = PyUFunc_OO_O;
    invert_functions[7] = PyUFunc_O_O;
    left_shift_functions[7] = PyUFunc_OO_O;
    right_shift_functions[7] = PyUFunc_OO_O;
    arccos_functions[0] = PyUFunc_f_f_As_d_d;
    arccos_functions[1] = PyUFunc_d_d;
    arccos_functions[2] = PyUFunc_F_F_As_D_D;
    arccos_functions[3] = PyUFunc_D_D;
    arccos_functions[4] = PyUFunc_O_O_method;
    ceil_functions[0] = PyUFunc_f_f_As_d_d;
    ceil_functions[1] = PyUFunc_d_d;
    ceil_functions[2] = PyUFunc_O_O_method;
    arctan2_functions[0] = PyUFunc_ff_f_As_dd_d;
    arctan2_functions[1] = PyUFunc_dd_d;
    arctan2_functions[2] = PyUFunc_O_O_method;

    f = PyUFunc_FromFuncAndData(add_functions, add_data, add_signatures, 12, 
				2, 1, PyUFunc_Zero, "add", 
				"Add the arguments elementwise.", 1);
    PyDict_SetItemString(dictionary, "add", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(subtract_functions, subtract_data, add_signatures, 
				12, 2, 1, PyUFunc_Zero, "subtract", 
				"Subtract the arguments elementwise.", 1);
    PyDict_SetItemString(dictionary, "subtract", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(multiply_functions, multiply_data, add_signatures, 
				12, 2, 1, PyUFunc_One, "multiply", 
				"Multiply the arguments elementwise.", 1);
    PyDict_SetItemString(dictionary, "multiply", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(divide_functions, divide_data, add_signatures, 
				12, 2, 1, PyUFunc_One, "divide", 
				"Divide the arguments elementwise.", 1);
    PyDict_SetItemString(dictionary, "divide", f);
    Py_DECREF(f);

    f = PyUFunc_FromFuncAndData(floor_divide_functions, floor_divide_data, floor_divide_signatures, 
				12, 2, 1, PyUFunc_One, "floor_divide", 
				"Floor divide the arguments elementwise.", 1);
    PyDict_SetItemString(dictionary, "floor_divide", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(true_divide_functions, true_divide_data, true_divide_signatures, 
				12, 2, 1, PyUFunc_One, "true_divide", 
				"True divide the arguments elementwise.", 1);
    PyDict_SetItemString(dictionary, "true_divide", f);
    Py_DECREF(f);

    f = PyUFunc_FromFuncAndData(divide_safe_functions, divide_safe_data, divide_safe_signatures, 
				9, 2, 1, PyUFunc_One, "divide_safe", 
				"Divide elementwise, ZeroDivision exception thrown if necessary.", 1);
    PyDict_SetItemString(dictionary, "divide_safe", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(conjugate_functions, conjugate_data, conjugate_signatures, 
				12, 1, 1, PyUFunc_None, "conjugate", 
				"returns conjugate of each element", 1);
    PyDict_SetItemString(dictionary, "conjugate", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(remainder_functions, remainder_data, remainder_signatures, 
				10, 2, 1, PyUFunc_Zero, "remainder", 
				"returns remainder of division elementwise", 1);
    PyDict_SetItemString(dictionary, "remainder", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(power_functions, power_data, add_signatures, 
				12, 2, 1, PyUFunc_One, "power", 
				"power(x,y) = x**y elementwise.", 1);
    PyDict_SetItemString(dictionary, "power", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(absolute_functions, absolute_data, absolute_signatures, 
				12, 1, 1, PyUFunc_None, "absolute", 
				"returns absolute value of each element", 1);
    PyDict_SetItemString(dictionary, "absolute", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(negative_functions, negative_data, negative_signatures, 
				12, 1, 1, PyUFunc_None, "negative", 
				"negative(x) == -x elementwise.", 1);
    PyDict_SetItemString(dictionary, "negative", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(greater_functions, divide_safe_data, greater_signatures, 
				9, 2, 1, PyUFunc_None, "greater", 
				"greater(x,y) is array of 1's where x > y, 0 otherwise.",1);
    PyDict_SetItemString(dictionary, "greater", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(greater_equal_functions, divide_safe_data, greater_signatures, 
				9, 2, 1, PyUFunc_None, "greater_equal", 
				"greater_equal(x,y) is array of 1's where x >=y, 0 otherwise.", 1);
    PyDict_SetItemString(dictionary, "greater_equal", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(less_functions, divide_safe_data, greater_signatures, 
				9, 2, 1, PyUFunc_None, "less", 
				"less(x,y) is array of 1's where x < y, 0 otherwise.", 1);
    PyDict_SetItemString(dictionary, "less", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(less_equal_functions, divide_safe_data, greater_signatures, 
				9, 2, 1, PyUFunc_None, "less_equal", 
				"less_equal(x,y) is array of 1's where x <= y, 0 otherwise.", 1);
    PyDict_SetItemString(dictionary, "less_equal", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(equal_functions, equal_data, equal_signatures, 
				13, 2, 1, PyUFunc_One, "equal", 
				"equal(x,y) is array of 1's where x == y, 0 otherwise.", 1);
    PyDict_SetItemString(dictionary, "equal", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(not_equal_functions, equal_data, equal_signatures, 
				13, 2, 1, PyUFunc_None, "not_equal", 
				"not_equal(x,y) is array of 0's where x == y, 1 otherwise.", 1);
    PyDict_SetItemString(dictionary, "not_equal", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(logical_and_functions, divide_safe_data, divide_safe_signatures, 
				9, 2, 1, PyUFunc_One, "logical_and", 
				"logical_and(x,y) returns array of 1's where x and y both true.", 1);
    PyDict_SetItemString(dictionary, "logical_and", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(logical_or_functions, divide_safe_data, divide_safe_signatures, 
				9, 2, 1, PyUFunc_Zero, "logical_or", 
				"logical_or(x,y) returns array of 1's where x or y or both are true.", 1);
    PyDict_SetItemString(dictionary, "logical_or", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(logical_xor_functions, divide_safe_data, divide_safe_signatures, 
				9, 2, 1, PyUFunc_None, "logical_xor", 
				"logical_xor(x,y) returns array of 1's where exactly one of x or y is true.", 1);
    PyDict_SetItemString(dictionary, "logical_xor", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(logical_not_functions, divide_safe_data, logical_not_signatures, 
				9, 1, 1, PyUFunc_None, "logical_not", 
				"logical_not(x) returns array of 1's where x is false, 0 otherwise.", 1);
    PyDict_SetItemString(dictionary, "logical_not", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(maximum_functions, divide_safe_data, divide_safe_signatures, 
				9, 2, 1, PyUFunc_None, "maximum", 
				"maximum(x,y) returns maximum of x and y taken elementwise.", 1);
    PyDict_SetItemString(dictionary, "maximum", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(minimum_functions, divide_safe_data, divide_safe_signatures,
				9, 2, 1, PyUFunc_None, "minimum", 
				"minimum(x,y) returns minimum of x and y taken elementwise.", 1);
    PyDict_SetItemString(dictionary, "minimum", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(bitwise_and_functions, bitwise_and_data, bitwise_and_signatures, 
				8, 2, 1, PyUFunc_One, "bitwise_and", 
				"bitwise_and(x,y) returns array of bitwise-and of respective elements.", 1);
    PyDict_SetItemString(dictionary, "bitwise_and", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(bitwise_or_functions, bitwise_or_data, bitwise_and_signatures, 
				8, 2, 1, PyUFunc_Zero, "bitwise_or", 
				"bitwise_or(x,y) returns array of bitwise-or of respective elements.", 1);
    PyDict_SetItemString(dictionary, "bitwise_or", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(bitwise_xor_functions, bitwise_xor_data, bitwise_and_signatures, 
				8, 2, 1, PyUFunc_None, "bitwise_xor", 
				"bitwise_xor(x,y) returns array of bitwise exclusive or of respective elements.", 1);
    PyDict_SetItemString(dictionary, "bitwise_xor", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(invert_functions, invert_data, invert_signatures, 
				8, 1, 1, PyUFunc_None, "invert", 
				"invert(n) returns array of bit inversion elementwise.", 1);
    PyDict_SetItemString(dictionary, "invert", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(left_shift_functions, left_shift_data, bitwise_and_signatures, 
				8, 2, 1, PyUFunc_None, "left_shift", 
				"left_shift(n, m) is n << m elementwise.", 1);
    PyDict_SetItemString(dictionary, "left_shift", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(right_shift_functions, right_shift_data, bitwise_and_signatures, 
				8, 2, 1, PyUFunc_None, "right_shift", 
				"right_shift(n, m) is n >> m elementwise.", 1);
    PyDict_SetItemString(dictionary, "right_shift", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(arccos_functions, arccos_data, arccos_signatures, 
				5, 1, 1, PyUFunc_None, "arccos", 
				"arccos(x) returns array of elementwise inverse cosines.", 1);
    PyDict_SetItemString(dictionary, "arccos", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(arccos_functions, arcsin_data, arccos_signatures, 
				5, 1, 1, PyUFunc_None, "arcsin", 
				"arcsin(x) returns array of elementwise inverse sines.", 1);
    PyDict_SetItemString(dictionary, "arcsin", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(arccos_functions, arctan_data, arccos_signatures, 
				5, 1, 1, PyUFunc_None, "arctan", 
				"arctan(x) returns array of elementwise inverse tangents.", 1);
    PyDict_SetItemString(dictionary, "arctan", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(arccos_functions, arctanh_data, arccos_signatures, 
				5, 1, 1, PyUFunc_None, "arctanh",
				"arctanh(x) returns array of elementwise inverse hyperbolic tangents.", 1);
    PyDict_SetItemString(dictionary, "arctanh", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(arccos_functions, arccosh_data, arccos_signatures, 
				5, 1, 1, PyUFunc_None, "arccosh",
				"arccosh(x) returns array of elementwise inverse hyperbolic cosines.", 1);
    PyDict_SetItemString(dictionary, "arccosh", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(arccos_functions, arcsinh_data, arccos_signatures, 
				5, 1, 1, PyUFunc_None, "arcsinh",
				"arcsinh(x) returns array of elementwise inverse hyperbolic sines.", 1);
    PyDict_SetItemString(dictionary, "arcsinh", f);
    Py_DECREF(f);   
    f = PyUFunc_FromFuncAndData(arccos_functions, cos_data, arccos_signatures, 
				5, 1, 1, PyUFunc_None, "cos", 
				"cos(x) returns array of elementwise cosines.", 1);
    PyDict_SetItemString(dictionary, "cos", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(arccos_functions, cosh_data, arccos_signatures, 
				5, 1, 1, PyUFunc_None, "cosh", 
				"cosh(x) returns array of elementwise hyberbolic cosines.", 1);
    PyDict_SetItemString(dictionary, "cosh", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(arccos_functions, exp_data, arccos_signatures, 
				5, 1, 1, PyUFunc_None, "exp", 
				"exp(x) returns array of elementwise e**x.", 1);
    PyDict_SetItemString(dictionary, "exp", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(arccos_functions, log_data, arccos_signatures, 
				5, 1, 1, PyUFunc_None, "log", 
				"log(x) returns array of elementwise natural logarithms.", 1);
    PyDict_SetItemString(dictionary, "log", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(arccos_functions, log10_data, arccos_signatures, 
				5, 1, 1, PyUFunc_None, "log10", 
				"log10(x) returns array of elementwise base-10 logarithms.", 1);
    PyDict_SetItemString(dictionary, "log10", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(arccos_functions, sin_data, arccos_signatures, 
				5, 1, 1, PyUFunc_None, "sin", 
				"sin(x) returns array of elementwise sines.", 1);
    PyDict_SetItemString(dictionary, "sin", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(arccos_functions, sinh_data, arccos_signatures, 
				5, 1, 1, PyUFunc_None, "sinh", 
				"sinh(x) returns array of elementwise hyperbolic sines.", 1);
    PyDict_SetItemString(dictionary, "sinh", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(arccos_functions, sqrt_data, arccos_signatures, 
				5, 1, 1, PyUFunc_None, "sqrt", 
				"sqrt(x) returns array of elementwise square roots.", 1);
    PyDict_SetItemString(dictionary, "sqrt", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(arccos_functions, tan_data, arccos_signatures, 
				5, 1, 1, PyUFunc_None, "tan", 
				"tan(x) returns array of elementwise tangents.", 1);
    PyDict_SetItemString(dictionary, "tan", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(arccos_functions, tanh_data, arccos_signatures, 
				5, 1, 1, PyUFunc_None, "tanh", 
				"tanh(x) returns array of elementwise hyperbolic tangents.", 1);
    PyDict_SetItemString(dictionary, "tanh", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(ceil_functions, ceil_data, ceil_signatures, 
				3, 1, 1, PyUFunc_None, "ceil", 
				"ceil(x) returns array of elementwise least whole number >= x.", 1);
    PyDict_SetItemString(dictionary, "ceil", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(ceil_functions, fabs_data, ceil_signatures, 
				3, 1, 1, PyUFunc_None, "fabs", 
				"fabs(x) returns array of elementwise absolute values, 32 bit if x is.", 1);

    PyDict_SetItemString(dictionary, "fabs", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(ceil_functions, floor_data, ceil_signatures, 
				3, 1, 1, PyUFunc_None, "floor", 
				"floor(x) returns array of elementwise least whole number <= x.", 1);
    PyDict_SetItemString(dictionary, "floor", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(arctan2_functions, arctan2_data, arctan2_signatures, 
				3, 2, 1, PyUFunc_None, "arctan2", 
				"arctan2(x,y) is a safe and correct tan(x/y).", 1);
    PyDict_SetItemString(dictionary, "arctan2", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(arctan2_functions, fmod_data, arctan2_signatures, 
				3, 2, 1, PyUFunc_None, "fmod", 
				"fmod(x,y) is remainder(x,y)", 1);
    PyDict_SetItemString(dictionary, "fmod", f);
    Py_DECREF(f);
    f = PyUFunc_FromFuncAndData(arctan2_functions, hypot_data, arctan2_signatures, 
				3, 2, 1, PyUFunc_None, "hypot", 
				"hypot(x,y) = sqrt(x**2 + y**2), elementwise.", 1);
    PyDict_SetItemString(dictionary, "hypot", f);
    Py_DECREF(f);

}


/* Initialization function for the module (*must* be called initArray) */

static struct PyMethodDef methods[] = {
    {NULL,		NULL, 0}		/* sentinel */
};

DL_EXPORT(void) initumath(void) {
    PyObject *m, *d, *s;
  
    /* Create the module and add the functions */
    m = Py_InitModule("umath", methods); 

    /* Import the array and ufunc objects */
    import_array();
    import_ufunc();

    /* Add some symbolic constants to the module */
    d = PyModule_GetDict(m);

    s = PyString_FromString("1.0");
    PyDict_SetItemString(d, "__version__", s);
    Py_DECREF(s);

    /* Load the ufunc operators into the array module's namespace */
    InitOperators(d); 

    PyDict_SetItemString(d, "pi", s = PyFloat_FromDouble(M_PI));
    Py_DECREF(s);
    PyDict_SetItemString(d, "e", s = PyFloat_FromDouble(exp(1.0)));
    Py_DECREF(s);

    /* Setup the array object's numerical structures */
    PyArray_SetNumericOps(d);
  
    /* Check for errors */
    if (PyErr_Occurred())
	Py_FatalError("can't initialize module umath");
}
