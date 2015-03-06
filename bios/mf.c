/* ===========================================================================
   MF:
      The virtual MinForth machine in ANSI-C, executes the image specified
      in the command line.
   Command line:
      mf [/i imagefile] [Forth command line]
   Files:
      <image.i>   compiled MinForth binary image file

      If imagefile is not specified the default image file mf.i is used.
      If you rename mf to xy the default imagefiel xy.i is used.

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

#include <stdlib.h>             /* standard libraries */
#include <stdio.h>

#include "mfcomp.h"		/* compiler-specific flags and libraries */
#include "mfptoken.h"

#if _OSTYPE <= 2
#include <sys\stat.h>
#include <io.h>
#include <conio.h>
#endif

#if _OSTYPE == 2
#define _CONSOLE
#include <windows.h>
#endif

#if _OSTYPE == 3
#include <sys/time.h>
#endif

#if _OSTYPE >= 3
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <termios.h>
#endif

#include <setjmp.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <math.h>


/* ---------------------------------------------------------------------------
   Declarations
*/

jmp_buf fvm_buf;		/* buffer to jump to FVM after exceptions */
void	ExecuteToken(); 	/* prototypes as forward declaration */

void	Throw(int i);		/* declaration of exception handler */
int	returncode=0;		/* return value to calling OS */

FILE*	imagefile;		/* MinForth imagefile control block */
char	fname[128]="";		/* MinForth commandline and filenames */
char	imagename[32];		/* imagefile name */

#define Cell	signed long	/* 32 bit cell */
#define Addr	unsigned long	/* 32 bit address */
#define Int	unsigned int	/* 16 bit address or offset */
#define Char	unsigned char	/* 8 bit character */
#define Float	double		/* 64 bit float */

#if MATH_64 == 0
struct	ds { Addr Lo; Addr Hi; };
typedef struct ds DAddr;
#endif

Addr	W;			/* MinForth Working Register */
Addr	IP=0;			/* MinForth Instruction Pointer */

Char Mfar* Forthspace;		/* MinForth image space */

#define NAMES	  16		/* Namespace address */
#define HEAP	  20		/* Heapspace address */
#define LIMIT	  24		/* last Forthspace adress */
#define THROW_CFA 40		/* Hilevel-Throw-CFA */
#define DEBUG_CFA 44		/* Debug-Trace-CFA */
#define HL_CLOCK  48		/* Hilevel clock for multitasking */
#define CODE_DP   52		/* Hilevel HERE */
#define NAME_DP   56		/* Hilevel HERE-N */

int	debugging=0;		/* flag, check debug state when on */
int	tracing=0;		/* flag, show tracing dialog when on */
Cell*	trcrst;			/* actual returnstack level to trace */
int	vectnesting=0;		/* used to detect cycling through vectors */
int	xccode=0;		/* holds exccode after exceptions */

Cell*	Datastack;		/* Datastack array */
Float*	Floatstack;		/* Floatstack array */
Cell*	Returnstack;		/* Returnstack array */

Char Mfar* Codespace;		/* codespace start */
Char Mfar* Namespace;		/* namespace start */
Char Mfar* Heapspace;		/* heapspace start */

Addr	codespacesize;		/* codespace size in bytes */
Addr	namespacesize;		/* namespace size in bytes */
Addr	heapspacesize;		/* heapspace size in bytes */
Addr	totalsize;		/* total forthspace size */

Int	stackcells;		/* max. depth of datastack */
Cell*	stk;			/* currrent stackpointer */
Cell*	stk_max;		/* lowest datastack position */
Cell*	stk_min;		/* highest datastack position */

Int	floatstackcells;	/* max. depth of floatstack */
Float*	flt;			/* current floatstackpointer */
Float*	flt_max;		/* lowest floatstack position */
Float*	flt_min;		/* highest floatstack position */

Int	returnstackcells;	/* max. depth of returnstack */
Cell*	rst;			/* current returnstackpointer */
Cell*	rst_max;		/* lowest returnstack position */
Cell*	rst_min;		/* highest returnstack position */

#define MTRUE	-1		/* MinForth's boolean flags */
#define MFALSE	 0

int	tasking=0;		/* tasking flag */
Addr	lastclock;		/* remember latest ticker time */
Addr	period;			/* msecs per ticker period */


/* ---------------------------------------------------------------------------
   Macros without any parameter checks
*/

#define NEXTW		W+4
#define IP_NEXT 	IP+=4
#define SETIP(x)	IP=(x)

#define PTR(x)		Forthspace+(x)
#define CAT(x)		*(PTR(x))
#define AT(x)		*(Cell Mfar*)(PTR(x))

#define DROP		stk++

#define POP()		*stk++
#define PUSH(x) 	*--stk=(x)
#define FPOP()		*flt++
#define FPUSH(x)	*--flt=(x)
#define RPOP()		*rst++
#define RPUSH(x)	*--rst=(x)

#define TOS		*stk
#define SECOND		stk[1]
#define THIRD		stk[2]
#define TOFS		*flt
#define TORS		*rst


/* ---------------------------------------------------------------------------
   Some useful subroutines
*/
Addr Milliseconds()
{  return((Addr)(1000*clock()/CLOCKS_PER_SEC)); }


/* ---------------------------------------------------------------------------
   Terminal raw mode setup
*/
#if _OSTYPE >= 3
struct termios newtset, oldtset; /* terminal settings */
static int pending = -1;	 /* holds character */
#endif

void SaveTerminal()
{
#if _OSTYPE >= 3
   tcgetattr(STDIN_FILENO, &oldtset);
#endif
}

void SetTerminal()
{
#if _OSTYPE >= 3
   newtset = oldtset;
   newtset.c_lflag    &= ~(ECHO | ICANON);
   newtset.c_iflag    &= ~ICRNL;
   newtset.c_cc[VTIME] = 0;
   newtset.c_cc[VMIN]  = 0;
   tcsetattr(STDIN_FILENO, TCSANOW, &newtset);
#else
   setmode(STDIN_FILENO, O_BINARY);
#endif
}

void RestoreTerminal()
{
#if _OSTYPE >= 3
   tcsetattr(STDIN_FILENO, TCSANOW, &oldtset);
#else
   setmode(STDIN_FILENO, O_TEXT);
#endif
}

int WaitKey(void) /* return true when keyboard event is in queue */
{
#if _OSTYPE >= 3
    char key;
    if (pending == -1)
    { if (0 == read(STDIN_FILENO, &key, 1)) return(0);
      if (key == 127) key = '\b';
      pending = key; }
    return(-1);
#else
    return(kbhit());
#endif
}

char GetKey(void) /* get or wait for next keyboard event */
{
#if _OSTYPE >= 3
    char key;
    if (pending == -1)
       do
          usleep(50000); /* for Linux ok, but for Minix? */
       while (WaitKey() == 0);
    key = pending, pending = -1;
    return(key);
#else
    return((char)getch());
#endif
}    


/* ---------------------------------------------------------------------------
   Release allocated memory before leaving MinForth
*/
void FreeAllocatedMemory()
{  if (Forthspace  != NULL) Mfree(Forthspace);
   if (Datastack   != NULL) free(Datastack);
   if (Floatstack  != NULL) free(Floatstack);
   if (Returnstack != NULL) free(Returnstack);
}

/* ---------------------------------------------------------------------------
   Immediate system stop
*/
void Terminate(char *msg)
{  fflush(stdout);
   RestoreTerminal();
   FreeAllocatedMemory();
   fprintf(stderr,"\nMinForth terminated: %s\007",msg);
   fprintf(stderr,"\n(press Return)");
   getchar();
   exit(-1);
}

/* ---------------------------------------------------------------------------
   Read next cell from imagefile
*/
Cell ReadCell()
{  Cell value;

   fread(&value,4,1,imagefile);
   return(value);
}

/* ---------------------------------------------------------------------------
   Read imagefile cells and set up forthspace and stacks
*/
void ReadImageFile()
{  Addr codeimagesize, nameimagesize;

   codeimagesize = ReadCell();
   nameimagesize = ReadCell();
   fseek(imagefile,(12+16),SEEK_SET);
   codespacesize = ReadCell(); /* read NAMES, HEAP, LIMIT */
   namespacesize = ReadCell() - codespacesize;
   heapspacesize = ReadCell() - codespacesize - namespacesize;
   totalsize = codespacesize + namespacesize + heapspacesize;
   stackcells = (Int)ReadCell();
   floatstackcells = (Int)ReadCell();
   returnstackcells = (Int)ReadCell();

   Forthspace  = (Char Mfar*)Mcalloc(totalsize+4,sizeof(Char));
   Datastack   = (Cell*)calloc(stackcells+2,sizeof(Cell));
   Floatstack  = (Float*)calloc(floatstackcells+2,sizeof(Float));
   Returnstack = (Cell*)calloc(returnstackcells+2,sizeof(Cell));
   if (Forthspace==NULL)
      Terminate("Can't allocate Forthspace");
   if ((Datastack==NULL)||(Floatstack==NULL)||(Returnstack==NULL))
      Terminate("Can't allocate stacks");
   stk_max = Datastack+1,   stk_min = Datastack+stackcells;
   stk = stk_min;
   flt_max = Floatstack+1,  flt_min = Floatstack+floatstackcells;
   flt = flt_min;
   rst_max = Returnstack+1, rst_min = Returnstack+returnstackcells;
   rst = rst_min;

   Codespace = Forthspace;
   Namespace = Codespace + codespacesize;
   Heapspace = Namespace + namespacesize;

   fseek(imagefile,12,SEEK_SET); /* set fptr to start of codeimage */
   if (codeimagesize != fread(Codespace,1,codeimagesize,imagefile))
      Terminate("Can't read codeimage");
   if (nameimagesize != fread(Namespace,1,nameimagesize,imagefile))
      Terminate("Can't read nameimage");
}

/* ---------------------------------------------------------------------------
   Open and read MinForth imagefile and commandline
*/

void OpenReadImageFile(int argc, char **argv)
{  int i,st; Addr adr; char *np1, *np2;

   if ((argc >=3)&&(argv[1][0]=='/')&&(toupper(argv[1][1])=='I'))
   {  strcpy(imagename,argv[2]);
      if (NULL == strchr(imagename,'.')) strcat(imagename,".i");
      st = 3;
   }
   else
   {  np1 = strrchr(argv[0],'/');  if (np1 == NULL) np1 = argv[0];
      np2 = strrchr(argv[0],'\\'); if (np2 == NULL) np2 = argv[0];
      if (np2 > np1) np1 = np2;
      if ((*np1 == '/')||(*np1 == '\\')) np1++; /* fname with ext isolated */
      np2 = strchr(np1,'.'); if (np2 == NULL) np2 = argv[0]+strlen(argv[0]);
      strcpy(imagename,np1);
      imagename[np2-np1] = 0;
      strcat(imagename,".i");
      st = 1;
   }
   strcpy(fname,imagename);
#if _OSTYPE <= 2
   imagefile = fopen(imagename,"rb");
#else
   imagefile = fopen(imagename,"r");
#endif
   if (imagefile == NULL)
   {  strcat(fname," imagefile open failed");
      Terminate(fname); }
   if ((Cell)0xe8f4b4bd != ReadCell()) /* check magic number */
      strcat(fname," is no MinForth imagefile"), Terminate(fname);
   ReadImageFile();
   fname[0]='\0';
   for (i=st; i<argc; i++) {
       strcat(fname,argv[i]); strcat(fname," "); }
   adr = AT(CODE_DP);
   CAT(adr) = (Char)strlen(fname); /* copy commandline to hilevel HERE */
   strcpy((char Mfar*)PTR(adr+1),fname);
}

/* ---------------------------------------------------------------------------
   Close imagefile before starting MinForth interpreter
*/
void CloseImageFile()
{  if (fclose(imagefile) == EOF)
      Terminate("Can't close imagefile");
}

/* ---------------------------------------------------------------------------
   Find the name belonging to a given address
*/
Addr SearchNames(Addr ca) /* find the word whose code includes address a */
{  Addr hdr, hdrmax, cfa, clen;

   hdr	  = AT(NAMES) + 4, hdrmax = AT(NAMES) + AT(NAME_DP);
   while (hdr < hdrmax)
   {  cfa = (Addr)AT(hdr+4),  clen = 0xffff & (AT(hdr+8));
      if (((clen==0)&&(cfa==ca))||((ca>=cfa-4)&&(ca<cfa-4+clen)))
	 return(hdr);
      hdr += ((0x1f & (CAT(hdr+12))) + 17) & -4l; }
   return(0);
}

/* ---------------------------------------------------------------------------
   Abort MinForth with stack dump
*/
void Abort(char *msg)
{  Int stkd, fltd, rstd, i; Addr r, hdr; Char Mfar* na;

   fflush(stdout); RestoreTerminal();
   fprintf(stderr,"\nMinForth VM exception %d at %lX: %s",(int)W,IP,msg);
   stkd = stk_min - stk, fltd = flt_min - flt, rstd = rst_min - rst;

   fprintf(stderr,"\nDatastack [%d cell(s)]",stkd);
   if (stkd != 0 ) fprintf(stderr,"\n");
   if (stkd > 8) { stkd = 8; fprintf(stderr,"<< "); }
   while (stkd-- > 0) fprintf(stderr,"%ld ",stk[stkd]);

   fprintf(stderr,"\nFloatstack [%d float(s)]",fltd);
   if (fltd != 0 ) fprintf(stderr,"\n");
   if (fltd > 8) { fltd = 8; fprintf(stderr,"<< "); }
   while (fltd-- > 0) fprintf(stderr,"%G ",flt[fltd]);

   fprintf(stderr,"\nReturnstack [%d cell(s)]",rstd);
   if (rstd > 8) rstd = 8;
   for (i=0; i<rstd; i++)
   {  r = rst[i];
      fprintf(stderr,"\n$%lX = %ld",r,(Cell)r);
      hdr = SearchNames(r);
      if (hdr)
      {  na = PTR(hdr+13);
	 if (*na) fprintf(stderr," --> %s + %ld",na,r-AT(hdr+4));
	 else fprintf(stderr," --> XT%ld + %ld",AT(hdr+4),r-AT(hdr+4));
	 hdr = SearchNames(AT(r-4));
	 na = PTR(hdr+13);
	 if (*na) fprintf(stderr," --> %s",na);
	 else fprintf(stderr," --> XT%ld",AT(hdr+4)); } }
   fprintf(stderr,"\n(press Return)");
   getchar();
   returncode = W; longjmp(fvm_buf,1);
}

/* ---------------------------------------------------------------------------
   Signal handling  /  Attention: Can cause problems with Win32 and WinNT
*/

#if _OSTYPE == 2

BOOL WINAPI WinBreakHandler(DWORD type)
{  if ((CTRL_C_EVENT == type)||(CTRL_BREAK_EVENT == type))
   {  xccode = -259, debugging = MTRUE;
      return(TRUE); }
   return(FALSE);
}

#else

void BreakSignal()
{  signal(SIGINT, BreakSignal);
   xccode = -259, debugging = MTRUE;
}

#endif

void FPESignal()
{  signal(SIGFPE, FPESignal);
   xccode = -55, debugging = MTRUE;
}

void SegVSignal()
{  signal(SIGSEGV, SegVSignal);
   xccode = -9, debugging = MTRUE;
}

#if _OSTYPE >= 3

void QuitSignal()
{  signal(SIGQUIT, QuitSignal);
   xccode = -1, debugging = MTRUE;
}

void TStopSignal()
{  xccode = 1, debugging = MTRUE;
}

void ContSignal()
{  signal(SIGCONT, ContSignal);
   xccode = 0, debugging = MTRUE;
}

#endif

/* ---------------------------------------------------------------------------
   Throw mechanism
*/
#define THROWCODES 61

struct errasgn { int xc; char* msg; };
struct errasgn throwcode[THROWCODES] = {
/* ANS Throw code assignments */
   {-3, "Stack overflow"},
   {-4, "Stack underflow"},
   {-5, "Return stack overflow"},
   {-6, "Return stack underflow"},
   {-7, "Stack overflow"},
   {-8, "Dictionary overflow"},
   {-9, "Invalid memory address"},
   {-10, "Division by zero"},
   {-11, "Result out of range"},
   {-12, "Argument type mismatch"},
   {-13, "Undefined word"},
   {-14, "Interpreting a compile-only word"},
   {-15, "Invalid FORGET"},
   {-16, "Attempt to use zero-length string as a name"},
   {-17, "Pictured numeric output string overflow"},
   {-18, "Parsed string overflow"},
   {-19, "Definition name too long"},
   {-20, "Write to read-only location"},
   {-21, "Unsupported operation"},
   {-22, "Control structure mismatch"},
   {-23, "Address alignment exception"},
   {-24, "Invalid numeric argument"},
   {-25, "Return stack imbalance"},
   {-26, "Loop parameter unavailable"},
   {-27, "Invalid recursion"},
   {-28, "User interrupt"},
   {-29, "Compiler nesting"},
   {-30, "Obsolescent feature"},
   {-31, ">BODY used on non-CREATEd definition"},
   {-32, "Invalid name argument"},
   {-33, "Block read exception"},
   {-34, "Block write exception"},
   {-35, "Invalid block number"},
   {-36, "Invalid file position"},
   {-37, "File I/O exception"},
   {-38, "Non-existent file"},
   {-39, "Unexpected end of file"},
   {-40, "Invalid base for floating-point conversion"},
   {-41, "Loss of precision"},
   {-42, "Floating-point divide by zero"},
   {-43, "Floating-point result out of range"},
   {-44, "Floating-point stack overflow"},
   {-45, "Floating-point stack underflow"},
   {-46, "Floating-point invalid argument"},
   {-47, "Compilation word list deleted"},
   {-48, "Invalid POSTPONE"},
   {-49, "Search-oder overflow"},
   {-50, "Search-oder underflow"},
   {-51, "Compilation word list changed"},
   {-52, "Control-flow stack overflow"},
   {-53, "Exception stack overflow"},
   {-54, "Floating-point underflow"},
   {-55, "Floating-point unidentified fault"},
   {-57, "Exception in sending or receiving a character"},
   {-58, "[IF], [ELSE] or [THEN] exception"},
/* MinForth's special error messages */
   {-256, "Unsupported execution token"},
   {-257, "Invalid code field address"},
   {-258, "Unreferred execution vector"},
   {-259, "Terminal break"},
   {-260, "Memory allocation problem"},
   {-261, "external reference not found"}
};

char* GetErrorMessage(int xc)
{  int i;
   for (i=0; i<THROWCODES; i++)
      if (xc == throwcode[i].xc) return(throwcode[i].msg);
   return(NULL);
}

void Throw(int code)
{  Addr A; char* errmsg;
   tracing = vectnesting = 0;
   A = AT(THROW_CFA), W = code; /* store code for Abort() */
   if (stk-stk_max < 4) stk = stk_max+4;
   if (rst-rst_max < 4) rst = rst_max+4;
   if ((A >= 256) && (A < totalsize)) {
      PUSH(code), W = A;
      AT(THROW_CFA)=0;
      ExecuteToken();
      AT(THROW_CFA)=A;
#if _OSTYPE == 3
      siglongjmp(fvm_buf,code); }
#else
      longjmp(fvm_buf,code); }
#endif
   errmsg = GetErrorMessage(code);
   if (errmsg != NULL) Abort(errmsg);
   else Abort("THROW with unknown exception code");
}

/* ------------------------------------------------------------------------
   Memory check and support functions
*/

void SetIP(Addr a)
{  if (a >= totalsize) Throw(-8);
   if (a & 3) Throw(-23);
   IP = a;
}
void Indepth(Int d)
{  if (stk+d > stk_min) Throw(-4);
}

void FIndepth(Int d)
{  if (flt+d > flt_min) Throw(-45);
}

void Inrange(Addr a, Int u)
{  if ((a >= totalsize)||((a+u) >= totalsize)) Throw(-9);
}

Cell At(Addr a)
{  if (a >= totalsize) Throw(-9);
   return(AT(a));
}

Char ChAt(Addr a)
{  if (a >= totalsize) Throw(-9);
   return(CAT(a));
}

Cell Pop()
{  if (stk >= stk_min) Throw(-4);
   return(POP());
}

void Push(Cell x)
{  if (stk < stk_max) Throw(-3);
   PUSH(x);
}

Cell RPop()
{  if (rst >= rst_min) Throw(-6);
   return(RPOP());
}

void RPush(Cell x)
{  if (rst < rst_max) Throw(-5);
   RPUSH(x);
}

void FPush(Float x)
{  if (flt < flt_max) Throw(-44);
   FPUSH(x);
}

Float FPop()
{  if (flt >= flt_min) Throw(-45);
   return(FPOP());
}

#if MATH_64
#define DLo(x) (Addr)(x)
#define DHi(x) (Addr)((x)>>32)
#else
#define DLo(x) (x).Lo
#define DHi(x) (x).Hi
#endif

void DStore(DAddr *dvar, Addr hi, Addr lo)
{
#if MATH_64
   *dvar = ((DAddr)hi << 32) + lo;
#else
   dvar->Lo = lo, dvar->Hi = hi;
#endif
}

void DNegate(DAddr* dv)
{
#if MATH_64
   *dv = 0 - (*dv);
#else
   dv->Lo = 0 - (dv->Lo); if (dv->Lo) dv->Hi += 1;
   dv->Hi = 0 - (dv->Hi);
#endif
}

/* ------------------------------------------------------------------------
   Hilevel program flow affecting functions
*/
void pPOTHOLE() /* ( -- ) abort whenever IP hits an empty cell */
{  fprintf(stderr," ? Bumped at IP=0x%lX",IP);
   Throw(-1);
}

void pDOCONST() /* ( -- x ) push a constant */
{  Push(AT(NEXTW));
}

void pDOVALUE() /* ( -- x ) push a value */
{  Push(At(NEXTW));
}

void pDOVAR() /* ( -- adr ) push variable address */
{  Push(NEXTW);
}

void pDOUSER() /* ( -- adr ) push user variable address */
{  Addr A = At(NEXTW);
   if ((A < 16)||(A >= 256)) Throw(-9);
   Push(A);
}

/* is this ok? */
void pDOVECT() /* ( -- ) defer execution to address in body */
{  Addr A = At(NEXTW);
   if ((A == 0)||(vectnesting > 10)) Throw(-258);
   vectnesting++, W = A, ExecuteToken(); /* nest vector executions */
   vectnesting--;
}

void pNEST() /* (R -- ip ) nest to hilevel cfa in colon definition */
{  RPush(IP);
   SetIP(NEXTW);
}

/* check nesting level up down */
void pUNNEST() /* (R ip -- ) unnest back to hilevel caller */
{  if (tracing)
      if (rst == trcrst) tracing = 0;
   SetIP(RPop());
}

void pEXECUTE() /* ( xt -- ) execute xt on TOS */
{  Addr XT = Pop();
   if (XT == (Addr)-1)
#if _OSTYPE == 3
      siglongjmp(fvm_buf,1);
#else
      longjmp(fvm_buf,1); /* end MinForth */
#endif
   if (XT >= totalsize) Throw(-256);
   W = XT, ExecuteToken();
}

void pTRACE() /* ( -- ) start lolevel tracer/debugger */
{  debugging = -1;
   if (AT(DEBUG_CFA) == 0l)
      trcrst = rst, tracing = -1;
}

/* ------------------------------------------------------------------------
   Inline literals
*/
void pLIT() /* ( -- n ) push inline value onto data stack */
{  Push(AT(IP)), IP_NEXT;
}

void pFLIT() /* ( f: -- r ) push unline float onto float stack */
{  FPush(*(Float Mfar*)(PTR(IP))), IP += 8;
}

void pSLIT() /* ( -- adr len ) push inline string address and length */
{  Char Len = CAT(IP);
   Push(IP+1), Push(Len);
   SETIP((IP+Len+5) & -4l);
}

void pTICK() /* ( -- xt ) push inline cfa */
{  Addr XT = AT(IP);
   if (XT >= totalsize) Throw(-257);
   Push(XT), IP_NEXT;
}

/* ------------------------------------------------------------------------
   Branching and looping
*/
void pJMP() /* ( -- ) unconditional absolute jump */
{  SetIP(AT(IP));
}

void pJMPZ() /* ( flag -- ) jump absolute if flag is zero */
{  if (Pop()) IP_NEXT; else SetIP(AT(IP));
}

void pJMPV() /* ( incr | i lim adr) add incr to i, jump when crossing */
{  int flag; Cell Incr = Pop();
   if (rst-rst_max < 3) Throw(-5);
   flag = (TORS < 0), TORS += Incr;
   if (flag ^ (TORS < 0)) SetIP(AT(IP)); else IP_NEXT;
}

/* ------------------------------------------------------------------------
   Memory access
*/
void pAT() /* ( adr -- n ) read value n from address adr */
{  Indepth(1);
   TOS = At(TOS);
}

void pSTORE() /* ( n adr -- ) store value n at address adr */
{  Addr A; Cell n;
   Indepth(2); A = POP(), n = POP();
   if (A >= totalsize) Throw(-9);
   AT(A) = n;
}

void pCAT() /* ( adr -- c ) read char c from address adr */
{  Indepth(1);
   TOS = ChAt(TOS);
}

void pCSTORE() /* ( c adr -- ) store char c at address adr */
{  Addr A; Char c;
   Indepth(2); A = (Addr)POP(), c = (Char)POP();
   if (A >= totalsize) Throw(-9);
   CAT(A) = c;
}

void pFILL() /* ( adr u c -- ) fill u chars c beginning at adr */
{  Addr A; Int u; Char c;
   Indepth(3); c = (Char)POP(), u = POP(), A = POP();
   Inrange(A,u);
   memset(PTR(A),c,u);
}

void pMOVE() /* ( from to u -- ) move u chars */
{  Addr From, To; Int u;
   Indepth(3); u = POP(), To = POP(), From = POP();
   Inrange(From,u); Inrange(To,u);
   memmove(PTR(To),PTR(From),u);
}

void pFSTORE() /* ( a -- f: r -- ) store float */
{  Addr A; Float Mfar* FPtr;
   A=Pop(); if (A >= totalsize) Throw(-9);
   FPtr = (Float Mfar*)(PTR(A));
   *FPtr = FPop();
}

void pFAT() /* ( a -- f: -- r ) read float from adr */
{  Addr A; Float Mfar* FPtr;
   A = Pop(); if (A >= totalsize) Throw(-9);
   FPtr = (Float Mfar*)(PTR(A));
   FPush(*FPtr);
}

void pSFSTORE()
{  Addr A; float Mfar* FPtr;
   A=Pop(); if (A >= totalsize) Throw(-9);
   FPtr = (float Mfar*)(PTR(A));
   *FPtr = (float)FPop();
}

void pSFAT()
{  Addr A; float Mfar* FPtr;
   A = Pop(); if (A >= totalsize) Throw(-9);
   FPtr = (float Mfar*)(PTR(A));
   FPush((Float)(*FPtr));
}

/* ------------------------------------------------------------------------
   Returnstack operations
*/

void pRDEPTH() /* ( -- u ) number of elements on returnstack */
{  Push(rst_min - rst);
}

void pRPSTORE() /* ( u -- ) set new returnstack depth */
{  Int u = Pop();
   if (u > returnstackcells) Throw(-5);
   rst = rst_min - u;
   if (tracing) trcrst = rst;
}

void pTOR() /* ( x -- |R -- x ) move TOS to returnstack */
{  RPush(Pop());
   if (tracing) trcrst-- ;
}

void pRFROM() /* ( -- x |R x -- ) move TORS to datastack */
{  Push(RPop());
   if (tracing) trcrst++;
}

void pRPICK() /* ( i -- ri ) copy i-th element onto returnstack */
{  Int i;
   Indepth(1); i = TOS;
   if (rst+i >= rst_min) Throw(-6);
   TOS = rst[i];
}

/* ------------------------------------------------------------------------
   Datastack operations
*/
void pDEPTH() /* ( -- u ) number of elements on datastack */
{  Push(stk_min - stk);
}

void pSPSTORE() /* ( u -- ) set new datastack depth */
{  Int u = Pop();
   if (u > stackcells) Throw(-3);
   stk = stk_min - u;
}

void pDROP() /* ( n -- ) drop the TOS */
{  Indepth(1); DROP;
}

void pSWAP() /* ( a b -- b a ) swap the top 2 stack elements */
{  Cell X;
   Indepth(2); X = TOS, TOS = SECOND, SECOND = X;
}

void pROT() /* ( a b c -- b c a ) rotate the top 3 stack elements */
{  Cell X;
   Indepth(3); X = THIRD, THIRD = SECOND, SECOND = TOS, TOS = X;
}

void pROLL() /* ( ni..n0 i -- ni-1..n0 ni ) */
{  Cell ni; Int i = Pop();
   Indepth(i+1); ni = stk[i];
   memmove(stk+1,stk,4*i); TOS = ni;
}

void pDUP() /* ( n -- n n ) push a TOS copy */
{  Indepth(1);
   Push(TOS);
}

void pOVER() /* ( a b -- a b a ) push a SECOND copy */
{  Indepth(2);
   Push(SECOND);
}

void pPICK() /* ( ni..n0 i -- ni..n0 ni ) copy i-th element onto stack */
{  Int i;
   Indepth(1); i = TOS; Indepth(i+2);
   TOS = stk[i+1];
}

void pFDEPTH() /* ( -- d ) number of elements in floatstack */
{  Push(flt_min - flt);
}

void pFPSTORE() /* ( u -- ) set new floatstack depth */
{  int u = Pop();
   if (u < 0 ) Throw(-45);
   if (u > (int)floatstackcells) Throw(-44);
   flt = flt_min - u;
}

void pFPICK() /* ( d: i -- f: fi..f0 -- fi..f0 fi ) pick ith float */
{  Int i;
   i = Pop(); FIndepth(i+1);
   FPush(flt[i]);
}

void pFROLL() /* ( d: i -- f: fi..f0 -- fi-1..f0 fi ) roll i floats */
{  Int i; Float fi;
   i = Pop(); FIndepth(i+1);
   fi = flt[i];
   memmove(flt+1,flt,8*i); TOFS = fi;
}

/* ------------------------------------------------------------------------
   Bit operations
*/
void pAND() /* ( a b -- a&b ) */
{  Cell i;
   Indepth(2); i = POP();
   TOS &= i;
}

void pOR() /* ( a b -- a | b ) */
{  Cell i;
   Indepth(2); i = POP();
   TOS |= i;
}

void pXOR() /* ( a b -- a^b ) */
{  Cell i;
   Indepth(2); i = POP();
   TOS ^= i;
}

void pLSHIFT() /* ( a b -- a<<b ) */
{  Int i;
   Indepth(2); i = (Int)POP();
   TOS <<= i;
}

void pRSHIFT() /* ( a b -- a>>b ) unsigned */
{  Int i;
   Indepth(2); i = (Int)POP();
   TOS = (Addr)TOS >> i;
}

/* ------------------------------------------------------------------------
   Logic operations
*/
void pLESS() /* ( a b -- flag ) */
{  Cell i;
   Indepth(2); i = POP();
   TOS = (TOS < i ? MTRUE : MFALSE);
}

void pEQUAL() /* ( a b -- flag ) */
{  Cell i;
   Indepth(2); i = POP();
   TOS = (TOS == i ? MTRUE : MFALSE);
}

void pULESS() /* ( a b -- flag ) unsigned */
{  Addr i;
   Indepth(2); i = (Addr)POP();
   TOS = ((Addr)TOS < i ? MTRUE : MFALSE);
}

void pFZLESS() /* ( f: r -- d: -- flag ) float < zero ? */
{  Push(FPop() < 0.0 ? MTRUE : MFALSE );
}

void pFZEQUAL() /* ( f: r -- d: -- flag ) float = zero ? */
{  Push(FPop() == 0.0 ? MTRUE : MFALSE );
}

/* ------------------------------------------------------------------------
   Arithmetics
*/
void pPLUS() /* ( a b -- sum ) */
{  Cell i;
   Indepth(2); i = POP();
   TOS += i;
}

void pMINUS() /* ( a b -- diff ) */
{  Cell i;
   Indepth(2); i = POP();
   TOS -= i;
}

void pSTAR() /* ( a b -- prod ) */
{  Cell i;
   Indepth(2); i = POP();
   TOS *= i;
}
	 
void pDIVMOD() /* ( a b -- rem quot ) */
{  Cell a,b;
   Indepth(2); a = SECOND, b = TOS;
   if (b == 0) Throw(-10);
   SECOND = a % b, TOS = a / b;
}

/* ------------------------------------------------------------------------
   Double and mixed arithmetics (for compilers with/out 64bit math)
*/

void pDPLUS() /* ( d1 d2 -- dsum ) */
{  DAddr D1,D2,DRes;
   Indepth(4); DStore(&D1,TOS,SECOND); stk+=2; DStore(&D2,TOS,SECOND);
#if MATH_64
   DRes = D1 + D2;
#else
   DRes.Lo = D2.Lo + D1.Lo; if (DRes.Lo < D1.Lo) D1.Hi++;
   DRes.Hi = D2.Hi + D1.Hi;
#endif
   SECOND = DLo(DRes), TOS = DHi(DRes);
}

void pDNEGATE() /* ( d -- -d ) */
{  DAddr D1;
   Indepth(2); DStore(&D1,TOS,SECOND);
   DNegate(&D1);
   SECOND = DLo(D1), TOS = DHi(D1);
}

void pMUSTAR() /* ( ud1 u2 -- udprod ) unsigned */
{
#if MATH_64
   Addr u; DAddr D1,DRes;
   Indepth(3); u=POP(); DStore(&D1,TOS,SECOND);
   DRes = D1 * u;
   SECOND = DLo(DRes), TOS = DHi(DRes);
#else
   Addr u, ul, uh, al, ah, a, b;
   Indepth(3); u = POP(), a = SECOND, b = TOS;
   ah = a >> 16, al = a & 0xffff, uh = u >> 16, ul = u & 0xffff;
   SECOND = a * u, TOS = b * u + ah * uh;
   a = uh * al, b = a + ul * ah + ((ul * al)>>16),
   u = b >> 16; if (a > b) u |= 0x10000l;
   TOS += u;
#endif
}

void pMUDIVMOD() /* ( ud1 u2 -- urem udquot ) unsigned */
{
#if MATH_64
   Addr u; DAddr D1, DRes;
   Indepth(3); u = TOS; DStore(&D1,SECOND,THIRD);
   if (u == 0) Throw(-10);
   DRes = D1 / u;  u = D1 % u;
   THIRD = u, SECOND = DLo(DRes), TOS = DHi(DRes);
#else
   Addr u, a, b, c, sa; int i;
   Indepth(3); u = TOS, c = SECOND, b = THIRD;
   if (u == 0) Throw(-10);
   a = c / u, c = c % u;
   for (i=0; i<32; i++)
   {   sa =	       c >> 31;
       c = (c << 1) | (b >> 31);
       b = (b << 1) | (a >> 31);
       a =  a << 1;
       if (sa | (c >= u)) c -= u, a += 1; }
   SECOND = a, TOS = b, THIRD = c;
#endif
}

/* ------------------------------------------------------------------------
   Floats
*/

void pDTOF()  /* ( d -- f: -- f ) convert double to float */
{  DAddr D1; Float F;
   Indepth(2); DStore(&D1,TOS,SECOND);
#if MATH_64
   F = (Float)(signed)D1;
#else
   if (TOS < 0) DNegate(&D1);
   F = ldexp((Float)D1.Hi,32) + (Float)D1.Lo;
   if (TOS < 0) F = -F;
#endif
   stk += 2;
   FPush(F);
}

void pFTOD()  /* ( f: f -- d: -- d ) convert float to double */
{
#if MATH_64
   long long Df; DAddr D1; Float F1;
   F1 = FPop();
   Df = F1;
   D1 = Df;
#else
   DAddr D1; Float F1, F2;
   F1 = FPop(); F2 = floor(fabs(F1));
   D1.Hi = (Addr)ldexp(F2,-32);
   D1.Lo = (Addr)(F2-ldexp((Float)D1.Hi,32));
   if (F1 < 0) DNegate(&D1);
#endif
   Push(DLo(D1)), Push(DHi(D1));
}

void pFLOOR() /* (f: f -- floor ) round towards -infinity */
{  FIndepth(1);
   TOFS = floor(TOFS);
}

void pFPLUS() /* ( f: f1 f2 -- fsum ) add floats */
{  Float i;
   FIndepth(2); i = FPOP();
   TOFS += i;
}

void pFMINUS() /* ( f: f1 f2 -- fdif ) subtract floats */
{  Float i;
   FIndepth(2); i = FPOP();
   TOFS -= i;
}

void pFSTAR() /* ( f: f1 f2 -- fmult ) multiply floats */
{  Float i;
   FIndepth(2); i = FPOP();
   TOFS *= i;
}

void pFDIV() /* ( f: f1 f2 -- fdiv ) divide floats */
{  Float i;
   FIndepth(2); i = FPOP();
   if (i == 0.0) Throw(-42);
   TOFS /= i;
}

void pFSQRT() /* ( f: f -- sqrt(F) ) square root */
{  Float i;
   FIndepth(1); i = TOFS;
   if (i < 0.0) Throw(-46);
   TOFS = sqrt(i);
}

void pFEXP() /* ( f: f -- exp(f) ) exponential function */
{  FIndepth(1);
   TOFS = exp(TOFS);
}

void pFLOG() /* ( f: f -- log(f) ) logarithmic function */
{  Float i;
   FIndepth(1); i = TOFS;
   if (i <= 0.0) Throw(-46);
   TOFS = log(i);
}

void pFSIN() /* ( f: f -- sin(f) ) sinus function */
{  FIndepth(1);
   TOFS = sin(TOFS);
}

void pFASIN() /* ( f: f -- asin(f) ) arcus sinus function */
{  Float i;
   FIndepth(1); i = TOFS;
   if (fabs(i) > 1.0) Throw(-46);
   TOFS = asin(i);
}

/* ---------------------------------------------------------------------------
   Floating-point number conversion
*/
void pTOFLOAT() /* ( a u -- f, if true: r: -- r ) convert string to float */
{  Int u; Float F; char* ep;
   Indepth(2); u = POP(); Inrange(TOS,u);
   memcpy(fname,PTR(TOS),u); fname[u]='\0';
   F = strtod(fname,&ep);
   if (ep-fname == (int)u) goto float_ok;
   if ('D' == toupper(*ep)) *ep = 'E';
   if ('E' == toupper(fname[u-1])) fname[u++]='0', fname[u]='\0';
   F = strtod(fname,&ep);
   if (ep-fname == (int)u) goto float_ok;
   while (u > 0) { if (fname[u-1] != ' ') break; u--; }
   if (u == 0) { F = 0.0; goto float_ok; }
   TOS = MFALSE; return;
float_ok:
   TOS = MTRUE; FPUSH(F);
}

void pREPRESENT() /* ( d: a u f: i -- d: e f1 f2 ) convert to string */
{  Float i; char *buf; int dpl, sig;
   Indepth(2);
   if ((0==TOS)||((Int)TOS>16)) Throw(-24);
   Inrange(SECOND,TOS);
   i = FPop();
   buf = (char*)ecvt(i,TOS,&dpl,&sig); /* missing in Minix 2.0.2 */
   memcpy(PTR(SECOND),buf,TOS);
   SECOND = dpl;
   TOS = (sig ? MTRUE : MFALSE);
   Push((isdigit(buf[0])&&isdigit(buf[1])) ? MTRUE : MFALSE);
}


/* ------------------------------------------------------------------------
   Strings
*/
void pCOMPARE() /* ( adr1 u1 adr2 u2 -- flag ) compare two strings */
{  Addr a1, u1, a2, u2;
   Indepth(4); u2=POP(), a2=POP(), u1=POP(), a1=TOS;
   Inrange(a1,u1); Inrange(a2,u2);
   a1 = memcmp(PTR(a1),PTR(a2),u1);
   if ((a1 == 0L)&&(u1 < u2)) a1 = (Addr)(-1L);
   TOS = a1;
}

void pSCAN() /* ( adr u c -- adr' u' ) scan string for first char c  */
{  Char c; Addr a, u;
   Indepth(3); c = (Char)POP(), u = TOS, a = SECOND;
   Inrange(a,u);
   if (c == ' ')
   {  while(u) { if (c >= CAT(a)) break; a++, u--; } }
   else
   {  while(u) { if (c == CAT(a)) break; a++, u--; } }
   SECOND=a, TOS=u;
}

void pTRIM() /* ( adr u c dir -- adr' u' ) trim leading/trailing chars */
{  Char c; Addr a, u; int dir;
   Indepth(4); dir=(int)POP(), c=(Char)POP(), u=TOS, a=SECOND;
   Inrange(a,u);
   if (dir >= 0) /* trim leading chars */
   {  if (c == ' ')
      {  while(u) { if (c  < CAT(a)) break; a++, u--; } }
      else
      {  while(u) { if (c != CAT(a)) break; a++, u--; } } }
   else /* trim trailing chars */
   {  if (c == ' ')
      {  while(u) { if (c  < CAT(a+u-1)) break; u--; } }
      else
      {  while(u) { if (c != CAT(a+u-1)) break; u--; } } }
   SECOND=a, TOS=u;
}

void pUPPER() /* ( adr u -- adr u ) convert string to upper chars */
{  Addr a, u;
   Indepth(2); u = TOS, a = SECOND;
   Inrange(a,u);
   while (u--)
      CAT(a) = (Char)toupper(CAT(a)), a++;
}

/* ------------------------------------------------------------------------
   Primitive terminal I/O
*/

void pEMITQ() /* ( -- flag ) true when terminal ready for output */
{  struct stat buf;
   Push(0l == fstat(STDOUT_FILENO,&buf) ? MTRUE : MFALSE);
}

void pTYPE() /* ( adr u -- ) emit character string to terminal */
{  Addr a; int u;
   Indepth(2), u = POP(), a = POP();
   Inrange(a,u);
   while (0 < u--) putchar(CAT(a++));
   fflush(stdout);
}

void pRAWKEYQ() /* ( -- flag ) test if keyboard signal is waiting */
{  Push(WaitKey() ? MTRUE : MFALSE);
}

void pRAWKEY() /* ( -- keycode ) fetch raw keyboard event code */
{  Push(GetKey());
}

/* ------------------------------------------------------------------------
   Time and date
*/
void pMSECS() /* ( -- msecs-since-start ) */
{  Push(Milliseconds());
}

void pTIMEDATE() /* ( -- second minute hour day month year ) */
{  struct tm *ts; time_t t;
   if (stk < stk_max+6) Throw(-3);
   ts = localtime(&t);
   PUSH(ts->tm_sec),  PUSH(ts->tm_min),   PUSH(ts->tm_hour);
   PUSH(ts->tm_mday), PUSH(1+ts->tm_mon), PUSH(1900+ts->tm_year);
}

void pTICKER() /* ( msecs-to-task -- ) set up lolevel ticker */
{  period = Pop();
   lastclock = Milliseconds();
   tasking = period ? MTRUE : MFALSE;
   AT(HL_CLOCK) = 0;
}

/* ---------------------------------------------------------------------------
   Basic file access with handles
*/
void pNOPEN() /* ( adr u fam -- hndl ior ) open or create file */
{  Addr a; Int u, f;
   Indepth(3); f = POP(), u = 127 & TOS, a = SECOND;
   Inrange(a,u);
   memcpy(fname,PTR(a),u); fname[u]='\0'; a=-1l, u=0;
   switch (f & 3)
   {  case 0: u  = O_RDONLY; break;
      case 1: u  = O_WRONLY; break;
      case 2: u  = O_RDWR;   break; }
#if _OSTYPE <= 2
   if (f&4)   u |= O_BINARY;
#endif
   if (f&8)   u |= (O_CREAT|O_TRUNC|O_RDWR);
   if (strcmp(fname,"stdin")==0)
      { if (u&(O_RDWR|O_RDONLY)) a = dup(STDIN_FILENO);
	else  errno = EACCES; }
   else if (strcmp(fname,"stdout")==0)
	   { if (u&(O_RDWR|O_WRONLY)) a = dup(STDOUT_FILENO);
	     else  errno = EACCES; }
   else if (strcmp(fname,"stderr")==0)
	   { if (u&(O_RDWR|O_WRONLY)) a = dup(STDERR_FILENO);
	     else  errno = EACCES; }
#if _OSTYPE == 4
   else a = open(fname,u);
#else
   else a = open(fname,u,S_IREAD|S_IWRITE);
#endif
   if (a == (Addr)-1) TOS = errno, SECOND = 0;
   else 	      TOS = 0, SECOND = a;
}

void pNRENAME() /* ( aold uold anew unew -- ior ) rename file */
{  char oname[64]=""; Addr ao, an; Int uo, un;
   Indepth(4); un = 127 & POP(), an = POP(), uo = 63 & POP(), ao = TOS;
   Inrange(ao,uo); Inrange(an,un);
   memcpy(fname,PTR(an),un); fname[un]='\0';
   memcpy(oname,PTR(ao),uo); oname[uo]='\0';
   if (rename(oname,fname)) TOS = errno; else TOS = 0;
}

void pNDELETE() /* ( adr u -- ior ) delete file */
{  Addr a; Int u;
   Indepth(2); u = 127 & POP(), a = TOS;
   Inrange(a,u);
   memcpy(fname,PTR(a),u); fname[u]='\0';
   if (remove(fname)) TOS = errno; else TOS = 0;
}

void pNSTAT() /* ( adr u -- stat ior ) get file access mode */
{  Addr a; Int u; struct stat sbuf;
   Indepth(2); a = SECOND, u = TOS;
   Inrange(a,u);
   memcpy(fname,PTR(a),u); fname[u]='\0';
   if (stat(fname,&sbuf))
      SECOND = errno, TOS = -1;
   else
      SECOND = sbuf.st_mode, TOS = 0;
}

void pHCLOSE() /* ( hndl -- ior ) close file */
{  Indepth(1);
   TOS = close(TOS);
}

void pHSEEK() /* ( pos hndl -- ior ) reposition file pointer */
{  Addr p, h;
   Indepth(2); h = POP(), p = TOS;
   if (p == (Addr)lseek(h,p,SEEK_SET)) TOS = 0; else TOS = errno;
}

void pHTELL() /* ( hndl -- pos ior ) get file pointer position */
{  Addr p;
   Indepth(1);
   p = lseek(TOS,0l,SEEK_CUR);
   if (p == (Addr)-1) TOS = 0, Push(errno); else TOS = p, Push(0);
}

void pHSIZE() /* ( hndl -- size ior ) get file size */
{  Addr s; struct stat buf;
   Indepth(1);
   s = fstat(TOS,&buf);
   if (s == (Addr)-1) TOS = 0, Push(errno); else TOS = buf.st_size, Push(0);
}

void pHCHSIZE() /* ( hndl unew -- ior ) change file size */
{  Addr u;
   Indepth(2); u = POP();
#if _OSTYPE >= 3
   if (ftruncate(TOS,u)) /* missing in Minix 2.0.2 */
#else
   if (chsize(TOS,u))
#endif
	TOS = errno;
   else TOS = 0;
}

void pHREAD() /* ( adr u1 hndl -- u2 ior ) read a byte sequence */
{  Addr a, u, h;
   Indepth(3); h = POP(), u = TOS, a = SECOND;
   Inrange(a,u);
   u = read(h,PTR(a),u);
   if (u == (Addr)-1) SECOND = 0, TOS = errno; else SECOND = u, TOS = 0;
}

void pHWRITE() /* ( adr u hndl -- ior ) write a byte sequence */
{  Addr a, u, h;
   Indepth(3); h = POP(), u = POP(), a = TOS;
   Inrange(a,u);
   u = write(h,PTR(a),u);
   if (u == (Addr)-1) TOS = errno; else TOS = 0;
}

/* ------------------------------------------------------------------------
   Identify primitives and fetch error message strings
*/
#include "mfpnames.c"	/* get array with primitive namnes */

void pPRIMTOXT() /* ( a u -- xt true | false ) get primitive exec. token */
{  Addr a; Int u, i;
   Indepth(2); u = POP(); a = TOS; Inrange(a,u);
   memcpy(fname,PTR(a),u); fname[u]='\0';
   for (i=0; i<u; i++) fname[i] = toupper(fname[i]);
   for (i=0; i<PRIMNUMBER; i++)
   { if (0 == strcmp(fname,primtable[i]))
	{ TOS = i; PUSH(MTRUE); return; } }
   TOS = MFALSE;
}

void pXTTOPRIM() /* ( xt bufadr -- flag ) get primitive name */
{  Addr a; Int xt, u;
   Indepth(2); a = POP(), xt = TOS;
   if (xt >= PRIMNUMBER) TOS = MFALSE;
   else
   { u = strlen(primtable[xt]); Inrange(a,u+1); CAT(a) = u;
     strncpy((char Mfar*)PTR(a+1),primtable[xt],u);
     TOS = MTRUE; }
}

void pTCTOERRMSG() /* ( xc bufadr -- flag ) get error message */
{  Addr buf; int xc; char* errmsg; Int u;
   Indepth(2); buf = POP(), xc = (int)TOS;
   errmsg = GetErrorMessage(xc);
   if (errmsg == NULL) TOS = MFALSE;
   else
   { u = strlen(errmsg); Inrange(buf,u+1); CAT(buf) = u;
     strncpy((char Mfar*)PTR(buf+1),errmsg,u);
     TOS = MTRUE; }
}

/* ------------------------------------------------------------------------
   Operating system functions
*/
void pOSCOMMAND() /* ( adr u -- retcode ) command host OS */
{  Addr a; Int u;
   Indepth(2); u = 127 & POP(), a = TOS;
   Inrange(a,u);
   memcpy(fname,PTR(a),u); fname[u]='\0';
   TOS = system(fname);
}

void pOSRETURN() /* ( retcode -- ) return to OS */
{  returncode = Pop();
   longjmp(fvm_buf,1);
}

void pGETENV() /* ( a u buf len -- flag ) get OS environment variable */
{  Addr buf; Int u,len; char* env;
   Indepth(4); len = POP(), buf = POP(), u = 127 & POP();
   Inrange(TOS,u); Inrange(buf,len);
   memcpy(fname,PTR(TOS),u); fname[u]='\0';
   env = getenv(fname);
   if (NULL == env) TOS = MFALSE;
   else { u = strlen(env); if (u>len) Throw(-18);
	  CAT(buf) = (Char)u; strcpy((char*)(buf+1),env);
	  TOS = MTRUE; }
}

void pPUTENV() /* ( a u -- flag ) set OS environment variable */
{  Addr buf; Int u;
   Indepth(2); u = 127 & POP(), buf = TOS;
   Inrange(buf,u);
   memcpy(fname,PTR(buf),u); fname[u]='\0';
   if (putenv(fname)) TOS=MFALSE; else TOS=MTRUE;
}

void pRESIZEFORTH() /* ( addon -- ior ) increase Forthspace */
{  Char Mfar* NewForthspace; Addr a;
   Indepth(1); a = (7 + TOS) & -4;
   NewForthspace = (Char Mfar*)Mrealloc(Forthspace,(totalsize+a));
   if (NewForthspace == NULL) TOS = MTRUE;
   else
   {  Forthspace = Codespace = NewForthspace;
      totalsize += a;
      AT(LIMIT) += a;
      TOS = 0; }
}

/* ------------------------------------------------------------------------
   Search for names in MinForth dictionary (lolevel-coded for speed)
*/
void pSEARCHTHREAD() /* ( adr u nfa-ptr -- 0 | lfa pred-lfa ) */
{  Addr a, u, cslfa, pcslfa, cura;
   Indepth(3); pcslfa = POP(), u = POP(), a = TOS;
   Inrange(a,u);
   codespacesize = AT(NAMES);
   while (u)
   {  cura = At(pcslfa);
      if (0 == cura) break; /* thread exhausted */
      cslfa = codespacesize + cura;
      Inrange(cslfa,44);
      cura = 0x1f & (CAT(cslfa+12));
      if ((u==cura) && !memcmp(PTR(a),PTR(cslfa+13),u))
      { TOS = cslfa, PUSH(pcslfa); return; }
      pcslfa = cslfa; }
   TOS = 0;
}

void pOSTYPE() /* ( -- id ) inform about underlying OS */
{  Push(_OSTYPE);
}

void pSEARCHNAMES() /* ( codeadr -- 0 | headeradr ) */
{  Indepth(1);
   TOS = SearchNames(TOS);
}


/* ---------------------------------------------------------------------------
   BIOS interface example
*/
#include "mf_bios.c"

/* ---------------------------------------------------------------------------
   Jumptable to all MinForth primitives
*/
#include "mfpfunc.c"

/* --------------------------------------------------------------------------
   Lolevel Tracer / Debugger
*/
static int hexflag, rstflag, fltflag;

void Debug()
{  Cell tk; int xc;
   Cell* ptr; Float* fptr; char Mfar* na;

   if (xccode) /* signal exception handling */
      xc=xccode, debugging=xccode=0, Throw(xc);

   if (W == (Addr)AT(DEBUG_CFA))
      trcrst = rst, tracing = -1;
   else if (tracing == 0)
           return;
        else if (trcrst != rst)
             return;
Label1:
   printf("\n");
/* ptr = rst_min; while (ptr-- > rst) putchar('.'); */
   if (hexflag) printf("x%lX:",IP); else printf("%ld:",IP);
   if (rstflag) {
       ptr = rst_min; printf(" r"); while (ptr-- > rst)
       { if (hexflag) printf(" %lX",*ptr); else printf(" %ld",*ptr); } }
   if (fltflag) {
      fptr = flt_min; printf(" f"); while (fptr-- > flt) printf(" %G",*fptr); }
   ptr = stk_min; printf(" d"); while (ptr-- > stk)
   { if (hexflag) printf(" %lX",*ptr); else printf(" %ld",*ptr); }
   tk = AT(IP), na = (char Mfar*)(PTR(13+SearchNames(tk)));
   if (*na != '\0') printf(" > %s ",na);
   else if (hexflag) printf(" > XT%lX ",W); else printf(" > XT%ld ",W);
   if (tk == xtLIT) printf("%ld",AT(IP+4));
   else if (tk == xtSLIT) printf("%s",PTR(IP+5));

Label2:
   fflush(stdout);
   switch (toupper(GetKey())) {
   case ' ': case 13: break;
   case 'R': printf(" Returnstack ");
	     if (rstflag) { rstflag=0;	printf("off"); }
	     else {	    rstflag=-1; printf("on"); }
	     goto Label1;
   case 'F': printf(" Floating-point stack ");
	     if (fltflag) { fltflag=0;	printf("off"); }
	     else {	    fltflag=-1; printf("on"); }
	     goto Label1;
   case 'X': if (hexflag) { hexflag=0;	printf(" Decimal"); }
	     else {	    hexflag=-1; printf(" Hex"); }
	     goto Label1;
   case 'N': printf(" Nest");
	     if (trcrst > rst_max) trcrst--; break;
   case 'U': printf(" Unnest");
	     if (trcrst < rst_min) trcrst++; break;
   case 'C': case 27: printf(" Continue\n");
	     tracing = 0; break;
   case 'S': printf(" Stop debugger\n");
	     AT(DEBUG_CFA) = debugging = tracing = 0; break;
   case 'A': printf(" Abort and stop debugger");
	     AT(DEBUG_CFA) = debugging = tracing = 0; Throw(-1);
   case 'B': printf(" Bye? Really? [Y] ");
             if (toupper(GetKey()) == 'Y')
#if _OSTYPE == 3
		siglongjmp(fvm_buf,1);
#else
		longjmp(fvm_buf,1);
#endif
             break;
   case 'H': printf(" Help\nHelp RSt FSt heX Nest Unnest Cont Stop Abort Bye");
	     goto Label1;
   default:  goto Label2; }
   fflush(stdout);
}

/* ---------------------------------------------------------------------------
   Lolevel dispatcher
*/
void ExecuteToken()
{  Addr XT;

   if (W < PRIMNUMBER) {		/* inline primitives */
      (primfunc[W])(); return; }
   if ((W < 256)||(W >= totalsize))	/* unassigned */
      Throw(-256);
   XT = AT(W);
   if (XT < EXECNUMBER) {		/* indirect primitives */
      (primfunc[XT])(); return; }
   if ((XT < 256)||(XT >= totalsize))	/* out of range */
      Throw(-256);
   PUSH(NEXTW); RPUSH(IP); SETIP(XT);	/* do DOES> */
}

/* ---------------------------------------------------------------------------
   Preemptive task manager
*/
void Task()
{  Addr actualclock;
   actualclock = Milliseconds();
   if (period >= lastclock-actualclock) return;
   lastclock = actualclock;
   AT(HL_CLOCK) += 1;
}

/* ---------------------------------------------------------------------------
   MinForth Main Program and Virtual Machine
*/
int main(int argc, char **argv)
{
   OpenReadImageFile(argc,argv);
   CloseImageFile();
   SaveTerminal();
#if _OSTYPE == 2
   SetConsoleTitle("MinForth Version 1.5");
   SetConsoleCtrlHandler(WinBreakHandler,TRUE);
#else
   signal(SIGINT,  BreakSignal);
#endif
   signal(SIGFPE,  FPESignal);
   signal(SIGSEGV, SegVSignal);
#if _OSTYPE >= 3
   signal(SIGQUIT, QuitSignal);
   signal(SIGTSTP, TStopSignal);
   signal(SIGCONT, ContSignal);
#endif

#if _OSTYPE == 3
   if (1 == sigsetjmp(fvm_buf,-1))
#else
   if (1 == setjmp(fvm_buf))
#endif
      goto fvm_end;
   SetTerminal();

fvm_loop:
   if (tasking) Task();
   if (debugging) Debug();	/* deviate through debugger */
   W = AT(IP), IP_NEXT; 	/* classic threaded Forth */
   ExecuteToken();
   goto fvm_loop;

fvm_end:
   RestoreTerminal();

#if _OSTYPE != 2
   signal(SIGINT,  SIG_DFL);
#endif
   signal(SIGFPE,  SIG_DFL);
   signal(SIGSEGV, SIG_DFL);
#if _OSTYPE >= 3
   signal(SIGQUIT, SIG_DFL);
   signal(SIGTSTP, SIG_DFL);
   signal(SIGCONT, SIG_DFL);
#endif
   FreeAllocatedMemory();
   putchar('\n');
   return(returncode);
}
