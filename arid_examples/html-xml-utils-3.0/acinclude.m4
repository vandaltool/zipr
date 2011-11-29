# AC_FLEX_OPTIMIZE
# --------------------------------------------------------------
# Check whether we can use option -Cfe to optimize the lexer
AC_DEFUN([AC_FLEX_OPTIMIZE],
[case "$ac_cv_prog_LEX" in
  *flex) lex_opt_flags=-Cfe;;
esac])

# AC_MAN2HTML
# --------------------------------------------------------------
# Set $man2html to the path of the man2html program, or to ":"
AC_DEFUN([AC_PROG_MAN2HTML],
[AC_ARG_VAR(man2html, "Full path of man2html program")
AC_PATH_PROG(man2html, man2html, ":")])
