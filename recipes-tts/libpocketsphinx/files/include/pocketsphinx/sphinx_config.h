/* sphinx_config.h: Externally visible configuration parameters */

/* Define to use fixed-point computation */
/* #undef FIXED_POINT */

/* Default radix point for fixed-point */
#define DEFAULT_RADIX 12

/* Define if you have the <stdint.h> header file. */
#ifndef HAVE_STDINT_H
#define HAVE_STDINT_H
#endif

/* The size of `long', as computed by sizeof. */
#define SIZEOF_LONG 8

/* Define if the system has the type `long long'. */
#ifndef HAVE_LONG_LONG /* sometimes it is already defined */
#define HAVE_LONG_LONG
#endif

/* The size of `long long', as computed by sizeof. */
#define SIZEOF_LONG_LONG 8

/* Enable debugging output */
/* #undef SPHINX_DEBUG */
