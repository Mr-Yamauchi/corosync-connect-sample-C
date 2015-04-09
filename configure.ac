#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([darkside], [0.0.1], [yamauchi@matrixjpn.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AM_PROG_CC_C_O
AC_PROG_RANLIB
AC_PROG_LIBTOOL

# Checks for libraries.
AC_CHECK_LIB([cpg], [main])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T

# Checks for library functions.

AC_CONFIG_FILES([Makefile
                 src/Makefile
		 src/lib/Makefile])
AC_OUTPUT
