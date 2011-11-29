/* A Bison parser, made from /0/w3c/bbos/Work/HTML/html.y, by GNU bison 1.75.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

#ifndef BISON_Y_TAB_H
# define BISON_Y_TAB_H

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     TEXT = 258,
     COMMENT = 259,
     START = 260,
     END = 261,
     NAME = 262,
     STRING = 263,
     PROCINS = 264,
     EMPTYEND = 265,
     DOCTYPE = 266,
     ENDINCL = 267
   };
#endif
#define TEXT 258
#define COMMENT 259
#define START 260
#define END 261
#define NAME 262
#define STRING 263
#define PROCINS 264
#define EMPTYEND 265
#define DOCTYPE 266
#define ENDINCL 267




#ifndef YYSTYPE
#line 98 "html.y"
typedef union {
    string s;
    pairlist p;
} yystype;
/* Line 1281 of /usr/local/bison/share/bison/yacc.c.  */
#line 69 "html.h"
# define YYSTYPE yystype
#endif

extern YYSTYPE yylval;


#endif /* not BISON_Y_TAB_H */

