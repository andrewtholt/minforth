/* ======== MinForth Names for C-coded Primitives ========

   This file is used by all MinForth programs

   !!!	The order and number of the primtable[]-array
   !!!	   must match the defines in MFPTOKEN.H 


   Copyright (C) 2003  Andreas Kochenburger (kochenburger@gmx.de)

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 
*/


#ifndef  PRIMNUMBER
#include "mfptoken.h"
#endif

char	*primtable[PRIMNUMBER] =
{
	"_POTHOLE",		/* 0  */

	"_DOCONST",		/* 1  */
	"_DOVALUE",		/* 2  */
	"_DOVAR",		/* 3  */
	"_DOUSER",		/* 4  */
	"_DOVECT",		/* 5  */
	"_NEST",		/* 6  */

	"_UNNEST",		/* 7  */
	"_EXECUTE",		/* 8  */

	"_TRACE",		/* 9  */

	"_LIT", 		/* 10 */
	"_SLIT",		/* 11 */
	"_TICK",		/* 12 */

	"_JMP", 		/* 13 */
	"_JMPZ",		/* 14 */
	"_JMPV",		/* 15 */

	"_AT",			/* 16 */
	"_STORE",		/* 17 */
	"_CAT", 		/* 18 */
	"_CSTORE",		/* 19 */
	"_FILL",		/* 20 */
	"_MOVE",		/* 21 */

	"_RDEPTH",		/* 22 */
	"_RPSTORE",		/* 23 */
	"_TOR", 		/* 24 */
	"_RFROM",		/* 25 */
	"_RPICK",		/* 26 */

	"_DEPTH",		/* 27 */
	"_SPSTORE",		/* 28 */

	"_DROP",		/* 29 */
	"_SWAP",		/* 30 */
	"_ROT", 		/* 31 */
	"_ROLL",		/* 32 */
	"_DUP", 		/* 33 */
	"_OVER",		/* 34 */
	"_PICK",		/* 35 */

	"_AND", 		/* 36 */
	"_OR",			/* 37 */
	"_XOR", 		/* 38 */
	"_LSHIFT",		/* 39 */
	"_RSHIFT",		/* 40 */
	"_LESS",		/* 41 */
	"_EQUAL",		/* 42 */
	"_ULESS",		/* 43 */

	"_PLUS",		/* 44 */
	"_MINUS",		/* 45 */
	"_STAR",		/* 46 */
	"_DIVMOD",		/* 47 */
	"_DPLUS",		/* 48 */
	"_DNEGATE",		/* 49 */
	"_MUSTAR",		/* 50 */
	"_MUDIVMOD",		/* 51 */

	"_COMPARE",		/* 52 */
	"_SCAN",		/* 53 */
	"_TRIM",		/* 54 */
	"_UPPER",		/* 55 */

	"_EMITQ",		/* 56 */
	"_TYPE",		/* 57 */

	"_RAWKEYQ",		/* 58 */
	"_RAWKEY",		/* 59 */

	"_MSECS",		/* 60 */
	"_TIMEDATE",		/* 61 */

	"_NOPEN",		/* 62 */
	"_NRENAME",		/* 63 */
	"_NDELETE",		/* 64 */
	"_NSTAT",		/* 65 */
	"_HCLOSE",		/* 66 */
	"_HSEEK",		/* 67 */
	"_HTELL",		/* 68 */
	"_HSIZE",		/* 69 */
	"_HCHSIZE",		/* 70 */
	"_HREAD",		/* 71 */
	"_HWRITE",		/* 72 */

	"_DTOF",		/* 73 */
	"_FTOD",		/* 74 */
	"_FLOOR",		/* 75 */
	"_FPSTORE",		/* 76 */
	"_FDEPTH",		/* 77 */
	"_FPICK",		/* 78 */
	"_FROLL",		/* 79 */
	"_FPLUS",		/* 80 */
	"_FMINUS",		/* 81 */
	"_FSTAR",		/* 82 */
	"_FDIV",		/* 83 */
	
	"_REPRESENT",		/* 84 */
	"_TOFLOAT",		/* 85 */

	"_FSTORE",		/* 86 */
	"_FAT", 		/* 87 */
	"_SFSTORE",		/* 88 */
	"_SFAT",		/* 89 */
	"_FLIT",		/* 90 */

	"_FZLESS",		/* 91 */
	"_FZEQUAL",		/* 92 */

	"_FSQRT",		/* 93 */
	"_FEXP",		/* 94 */
	"_FLOG",		/* 95 */
	"_FSIN",		/* 96 */
	"_FASIN",		/* 97 */

	"_PRIM>XT",		/* 98 */
	"_XT>PRIM",		/* 99 */
	"_TC>ERRMSG",		/* 100 */

	"_SEARCHTHREAD",	/* 101 */
	"_SEARCHNAMES", 	/* 102 */
	
	"_OSCOMMAND",		/* 103 */
	"_OSRETURN",		/* 104 */
	"_GETENV",		/* 105 */
	"_PUTENV",		/* 106 */
	"_OSTYPE",		/* 107 */

	"_RESIZEFORTH",		/* 108 */
	"_TICKER",		/* 109 */
    
/*  ----------------------------------
    	BIOS extension for DOS PCs
    ---------------------------------- */

    	"_FAR",			/* 110 */
    	"_BAT",			/* 111 */
    	"_WAT",			/* 112 */
    	"_BSTORE",		/* 113 */
    	"_WSTORE",		/* 114 */
    	"_BINP",		/* 115 */
    	"_WINP",		/* 116 */
    	"_BOUTP",		/* 117 */
    	"_WOUTP",		/* 118 */
    	"_INTR"			/* 119 */
};
