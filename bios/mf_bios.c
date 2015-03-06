/* ===========================================================================
   MF_BIOS:
      included by mf.c
      provides functions for DOS system programming in high-level Forth 

      Attention:
      works only with the Turbo C compiler for DOS

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
    
   ===========================================================================
*/

#if !defined(__TURBOC__) || !defined(__MSDOS__)
#error Turbo C for DOS required for BIOS functions
#endif

#define Word	unsigned short


/* ---------------------------------------------------------------------------
   External memory access
*/

Char far* SysBase = 0;

void p_FAR() /* ( mfadr -- faradr ) convert internal to external address */
{  Addr A;
   Indepth(1); A = TOS;
   if (totalsize < A) Throw(-9);
   TOS = (Addr)(Forthspace + A);
}

void p_BAT() /* ( faradr -- byte ) read external byte */
{  Char far* A;
   Indepth(1); A = (Char far*)TOS;
   TOS = *A;
}

void p_WAT() /* ( faradr -- word ) read external 16-bit word */
{  Word far* A;
   Indepth(1); A = (Word far*)TOS;
   TOS = *A;
}

void p_BSTORE() /* ( byte faradr -- ) write external byte */
{  Char far* A; Char b;
   b = (Char)Pop(); A = (Char far*)Pop();
   *A = b;
}

void p_WSTORE() /* ( word faradr -- ) write external 16-bit word */
{  Word far* A; Word w;
   w = Pop(); A = (Word far*)Pop();
   *A = w;
}


/* ---------------------------------------------------------------------------
   Port I/O
*/

void p_BINP() /* ( port -- byte ) read one byte from port */
{  Indepth(1);
   TOS = (Cell)inportb((unsigned int)TOS);
}

void p_WINP() /* ( port -- word ) read one word from port */
{  Indepth(1);
   TOS = (Word)inportb((unsigned int)TOS);
}

void p_BOUTP() /* ( byte port -- ) write one byte to port */
{  Char b; unsigned int p;
   p = Pop(), b = Pop();
   outportb(p,b); 
}

void p_WOUTP() /* ( word port -- ) write one word to port */
{  Word w; unsigned int p;
   p = Pop(), w = Pop();
   outport(p,w); 
}


/* ---------------------------------------------------------------------------
   Interrupts
*/

void p_INTR() /* ( regarray intnum -- ) Pascal-like interrupt */
{  int i; Addr A;
   i = Pop(); A = Pop();
   if (totalsize < A) Throw(-9);  
   intr(i, (struct REGPACK*)(Forthspace + A)); 
}
