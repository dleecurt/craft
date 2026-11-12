AC_PREREQ([2.69])
AC_INIT([craft], [3.5.1], [https://github.com/dleecurt/craft/issues], [craft],
        [https://github.com/dleecurt/craft])
AC_CONFIG_SRCDIR([craft.cc])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign -Wall -Werror subdir-objects])

AC_PROG_CXX
AC_PROG_RANLIB
AM_PROG_AR
AC_LANG([C++])

dnl X11
AC_PATH_XTRA
AS_IF([test "x$no_x" = xyes], [AC_MSG_ERROR([Craft needs Xlib (libX11 and its headers)])])
save_LIBS=$LIBS
save_CPPFLAGS=$CPPFLAGS
CPPFLAGS="$CPPFLAGS $X_CFLAGS"
LIBS="$X_LIBS $LIBS"
AC_CHECK_HEADER([X11/Xlib.h], [], [AC_MSG_ERROR([X11/Xlib.h not found])])
AC_CHECK_LIB([X11], [XOpenDisplay], [X11_LIBS="$X_LIBS $X_PRE_LIBS -lX11 $X_EXTRA_LIBS"],
             [AC_MSG_ERROR([libX11 not found])])
LIBS=$save_LIBS
CPPFLAGS=$save_CPPFLAGS
AC_SUBST([X11_LIBS])

AC_CHECK_LIB([m], [sqrt])

dnl zlib, for compressed saved games
AC_CHECK_HEADER([zlib.h], [], [AC_MSG_ERROR([zlib.h not found (install zlib1g-dev or zlib-devel)])])
AC_CHECK_LIB([z], [gzopen], [], [AC_MSG_ERROR([libz not found (install zlib1g-dev or zlib-devel)])])

dnl Art tools (picture editor, morph editor, converters)
AC_ARG_ENABLE([tools],
  [AS_HELP_STRING([--disable-tools], [do not build the craft-* art tools])],
  [], [enable_tools=yes])
AM_CONDITIONAL([BUILD_TOOLS], [test "x$enable_tools" != xno])

dnl Compiler warnings (the code builds warning-free with gcc and clang)
AC_ARG_ENABLE([warnings],
  [AS_HELP_STRING([--disable-warnings], [do not compile with -Wall -Wextra])],
  [], [enable_warnings=yes])
AS_IF([test "x$enable_warnings" = xyes],
      [WARN_CXXFLAGS="-Wall -Wextra"], [WARN_CXXFLAGS=""])
AC_SUBST([WARN_CXXFLAGS])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

craft_show_datadir=`eval eval echo "$datadir/$PACKAGE" | sed "s|^NONE|$ac_default_prefix|"`
AS_IF([test "x$prefix" != xNONE],
      [craft_show_datadir=`prefix=$prefix; eval eval echo "$datadir/$PACKAGE"`])
AC_MSG_NOTICE([
  Craft $PACKAGE_VERSION
  prefix:      $prefix
  data files:  $craft_show_datadir  (override at run time with CRAFT_DATADIR)
  art tools:   $enable_tools
])
