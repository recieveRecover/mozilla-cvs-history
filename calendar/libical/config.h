/* config.h.  Generated by configure.  */
/* config.h.in.  Generated from configure.in by autoheader.  */

/* Define to 1 if you have the <assert.h> header file. */
#define HAVE_ASSERT_H 1

/* Define to 1 if you have the `gmtime_r' function. */
#define HAVE_GMTIME_R 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define if we have pthread. */
/* #undef HAVE_PTHREAD */

/* Define to 1 if you have the <pthread.h> header file. */
/* #undef HAVE_PTHREAD_H 1 */

/* Define to 1 if you have the `snprintf' function. */
#define HAVE_SNPRINTF 1

/* Define to 1 if you have the <stdint.h> header file. */
#ifndef XP_WIN32
#define HAVE_STDINT_H 1
#endif

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strdup' function. */
#define HAVE_STRDUP 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <time.h> header file. */
#define HAVE_TIME_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#ifdef XP_WIN32
#define NO_YY_UNISTD_H 1
#else
#define HAVE_UNISTD_H 1
#endif

/* Define to make icalerror_* calls abort instead of internally signalling an
   error */
/* #undef ICAL_ERRORS_ARE_FATAL */

/* Define if we want _REENTRANT */
/* #undef ICAL_REENTRANT */

/* Define to terminate lines with "
" instead of "
" */
#define ICAL_UNIX_NEWLINE 1

/* Define to 1 if you DO NOT WANT to see deprecated messages */
#define NO_WARN_DEPRECATED 1

/* Define to 1 if you DO NO WANT to see the warning messages related to
   ICAL_MALFORMEDDATA_ERROR and parsing .ics zoneinfo files */
#define NO_WARN_ICAL_MALFORMEDDATA_ERROR_HACK 1

/* Name of package */
#define PACKAGE "libical"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME ""

/* Define to the full name and version of this package. */
#define PACKAGE_STRING ""

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME ""

/* Define to the version of this package. */
#define PACKAGE_VERSION ""

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define to 1 if your <sys/time.h> declares `struct tm'. */
/* #undef TM_IN_SYS_TIME */

/* Version number of package */
#define VERSION "0.24"

/* Define to 1 if `lex' declares `yytext' as a `char *' by default, not a
   `char[]'. */
#define YYTEXT_POINTER 1

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `int' if <sys/types.h> does not define. */
/* #undef mode_t */

/* Define to `unsigned' if <sys/types.h> does not define. */
/* #undef size_t */
