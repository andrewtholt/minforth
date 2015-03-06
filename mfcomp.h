/* ---------------------------------------------------------------------------
   MinForth - compiler.h
   (included by mf.c)
   make compiler-specific directives
   ---------------------------------------------------------------------------

   Extend the C precompiler definitions for each combination of operating
   system and compiler combination.

   OSTYPE return the operating system id. Use
   1	for	DOS
   2	for	Windows
   3	for	Linux
   4	for	Minix


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

/* ---------------------------------------------------------------------------
   Turbo C for DOS (16-bit)
*/
#if defined(__TURBOC__) && defined(__MSDOS__)

#define _OSTYPE 	1

#if !defined(__HUGE__)
#error Wrong compiler model (you must select Huge).
#endif

#include <alloc.h>
#define Mcalloc 	farcalloc
#define Mfree		farfree
#define Mrealloc	farrealloc
#define Mfar		huge

#define STDIN_FILENO	0
#define STDOUT_FILENO	1
#define STDERR_FILENO	2

#define MATH_64 	0

#define CLOCKS_PER_SEC	18.2


/* ---------------------------------------------------------------------------
   DJGPP for DOS (32-bit with DPMI memory extender)
*/
#elif defined(__DJGPP__)

#define _OSTYPE 	1

#include <unistd.h>

#define MATH_64 1
#define DAddr		unsigned long long

#define Mcalloc 	calloc
#define Mfree		free
#define Mrealloc	realloc
#define Mfar


/* ---------------------------------------------------------------------------
   GCC-MingW32 for Windows
*/
#elif defined(__GNUC__) && defined(__WIN32__)

#define _OSTYPE 	2

#define MATH_64 1
#define DAddr		unsigned long long

#define Mcalloc 	calloc
#define Mfree		free
#define Mrealloc	realloc
#define Mfar

#define STDIN_FILENO	0
#define STDOUT_FILENO	1
#define STDERR_FILENO	2


/* ---------------------------------------------------------------------------
   Borland C++ for Windows 95/98/NT
*/
#elif defined(__BORLANDC__) && defined(_WIN32)

/* #error BCC32 compiles but does not run p_RUNPROC :-( */

#define _OSTYPE 	2

#define MATH_64		0	/* bcc misreads longlong function arguments */

#define Mcalloc 	calloc
#define Mfree		free
#define Mrealloc	realloc
#define Mfar

#define STDIN_FILENO	0
#define STDOUT_FILENO	1
#define STDERR_FILENO	2


/* ---------------------------------------------------------------------------
   LCC-Win32 for Windows 95/98/NT
*/
#elif defined(__LCC__) && defined(_WIN32)

#define _OSTYPE 	2

#define MATH_64		1	/* some lcc-versions have buggy 64-bit math */
#define DAddr		unsigned long long

#define Mcalloc 	calloc
#define Mfree		free
#define Mrealloc	realloc
#define Mfar

/* #define ecvt		_ecvt 	some lcc-versions have buggy ecvt */

#define STDIN_FILENO	0
#define STDOUT_FILENO	1
#define STDERR_FILENO	2


/* ---------------------------------------------------------------------------
   MS Visual C for Windows (not tested)
*/
#elif defined(_MSC_VER) && defined(_WIN32)

#define _OSTYPE 	2

#define MATH_64 	1
#define DAddr		unsigned _int64

#define Mcalloc 	calloc
#define Mfree		free
#define Mrealloc	realloc
#define Mfar

#define STDIN_FILENO	0
#define STDOUT_FILENO	1
#define STDERR_FILENO	2


/* ---------------------------------------------------------------------------
   GCC for Linux
*/
#elif defined(__GNUC__) && defined(__linux__)

#define _OSTYPE 	3

#define MATH_64 	1
#define DAddr		unsigned long long

#define Mcalloc 	calloc
#define Mfree		free
#define Mrealloc	realloc
#define Mfar

#define STDIN_FILENO	0
#define STDOUT_FILENO	1
#define STDERR_FILENO	2


/* ---------------------------------------------------------------------------
   ACK for Minix
   mf.c not ready yet

#elif defined(__ACK__) && defined(__minix)

#define _OSTYPE		4

#define MATH_64		0

#define Mcalloc		calloc
#define Mfree		free
#define Mrealloc	realloc
#define Mfar

#define STDIN_FILENO	0
#define STDOUT_FILENO	1
#define STDERR_FILENO	2
*/

/* ---------------------------------------------------------------------------
   Unknown Compiler
*/
#else

#error Compiler unknown!

#endif

