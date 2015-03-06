/* ========================================================================
   METACOMP:
      metacompiles the MinForth kernel image from the inputfile kernel.mfc
      which contains definitions of primitives and basic MinForth words
   Command line:
      METACOMP (no parameters)
   Files:
      kernel.mfc  in	text file with the first kernel definitions
      kernel.i	  out	binary file of the MinForth kernel image


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
    
   ========================================================================
*/

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

/* ========================================================================
   Forth primitive execution tokens < 256
*/
#include "mfcomp.h"
#include "mfpnames.c"

/* ------------------------------------------------------------------------
   32 bit variable types
*/
typedef long		cell;
typedef unsigned long	address;

/* ------------------------------------------------------------------------
   Global definitions
*/
#define CODESPLEN  35000	/* bytelength of codespace array */
#define NAMESPLEN  25000	/* bytelength of namespace array */
#define MAXLINE    256		/* max input file line length */
#define MAXWORD    64		/* max header name length */
#define CFSSIZE    16		/* control flow stack cell number */

#define MTRUE	   -1		/* True flag */
#define MFALSE	   0		/* False flag */

#define uint	  unsigned int

int	eof_reached=MFALSE;	/* flag end of inputfile */

FILE	*inputfile;		/* fcb for kernel.mfc */
FILE	*outputfile;		/* fcb for kernel.i */

char	line[MAXLINE+1]="";	/* linebuffer */
int	ln_no=0;		/* line number */
char	token[MAXWORD+1]="";	/* wordbuffer */
uint	ln_idx=0;		/* running index in line[] */
uint	tk_idx=0;		/* index to 1st token char in line[] */
char	name[MAXWORD+1]="";	/* holds name of current definition */

char	codespace[CODESPLEN];	/* start of code space */
address code_dp;		/* code space dictionary pointer */

char	namespace[NAMESPLEN];	/* start of namespace containing headers */
address name_dp;		/* header space dictionary pointer */

address last=0; 		/* header dp of last definition */
address current=0;		/* header dp of current definition */
address last_cfa=0;		/* cfa of last definition */

address cfs[CFSSIZE];		/* control flow stack */
int	cfsptr=0;		/* control flow stack pointer */

/* ------------------------------------------------------------------------
   Useful macros
*/
#define iscmd(x)  (strcmp(command,x)==0) /* outer compiler command? */
#define isctrl(x) (strcmp(name,x)==0)	 /* inner compiler command? */

/* ========================================================================
   FUNCTIONS & PROCEDURES
*/
char *strupper(char *str)
{  char *cp;
   for (cp = str; *cp; ++cp) *cp = (char)toupper(*cp);
   return(cp);
}

/*   Terminate program by brute force with error message
*/
void Terminate(char *message)
{
   fflush(stdout);
   fprintf(stderr,"\n?: %s\n%s",message,"(press Return)");
   getchar();
   fprintf(stderr,"\n");
   exit(EXIT_FAILURE); /* close all open files and quit */
}

void LineTerminate(char *message)
{
   uint i;			/* offset into current line */

   fflush(stdout);
   fprintf(stderr,"\nError in line %d:",ln_no);
   fprintf(stderr,"\n%s\n",line);
   for (i=0; i < tk_idx; i++)
       fputc('-',stderr);
   fputc('^',stderr);
   Terminate(message);
}

void EarlyTerminate()
{
   LineTerminate("Abrupt file end within definition");
}

void InitTerminate()
{
   printf("\n%s not found",name);
   Terminate("Initialisation failure");
}

/* ------------------------------------------------------------------------
   Open and close kernel input files
*/
void OpenKernelFile()
{
   inputfile = fopen("kernel.mfc","rb");
   if (inputfile == NULL)
      Terminate("MinForth kernel source file kernel.mfc not found");
}

void CloseKernelFile()
{
   if (fclose(inputfile) == EOF)
      Terminate("Source file close error");
}

/* ------------------------------------------------------------------------
   Read tokens to token[] from input file until it's empty, if necessary
      refill line[] from input file; handle comments by \ and \S
   returns TRUE if eof reached
*/
int GetNextToken()
{
   uint i,tok_len;			/* token length */

Label1:
   if (ln_idx == strlen(line))	/* end of current line reached? */
   {
      if (fgets(line,MAXLINE,inputfile) == NULL) /* refill line */
	 return(MTRUE);
      for (i=0;i<strlen(line);i++) /* blank out control chars */
	 if (line[i] < ' ') line[i]=' ';
      line[strlen(line)-1] = '\0'; /* zero \n at line end */
      strcpy(token,""); 	/* update variables if line refill ok */
      ln_idx = 0;
      tk_idx = 0;
      ln_no++;
   /* verbose: printf("\n%s",line);
	       fflush(stdout);	    */
   }
   while (line[ln_idx]==' ') ln_idx++; /* advance until token begins */
   if (line[ln_idx] == '\0')	/* did we get an empty line here? */
      goto Label1;
   tk_idx = ln_idx;		/* remember token start */
   while (line[ln_idx] > ' ')
      ln_idx++; 		/* advance to char following token end */
   tok_len = ln_idx - tk_idx;
   if (tok_len > MAXWORD)
      tok_len = MAXWORD;
   memcpy(token,&line[tk_idx],tok_len); /* copy isolated token */
   token[tok_len] = '\0';
   if (strcmp(token,"\\") == 0) /* comment till end of line? */
   {
      ln_idx = strlen(line);
      goto Label1;
   }
   if ((strcmp(token,"\\s")==0)||(strcmp(token,"\\S")==0))
      return(MTRUE);
   /* prepend Forth comment words with underscore */
   if (strcmp(token,"_\\")==0)
      strcpy(token,"\\");
   if ((strcmp(token,"_\\s")==0)||(strcmp(token,"_\\S")==0))
      strcpy(token,"\\S");
   return(MFALSE);
}

/* ------------------------------------------------------------------------
   Get next word to name[]
*/
void GetLoWord()
{
   if (GetNextToken() == MTRUE)
      EarlyTerminate();
   strcpy(name,token);
}

void GetWord()
{
   GetLoWord();
   strupper(name);
}

/* ------------------------------------------------------------------------
   Try to convert token in name[] to the passed number
   Returns true if successful, otherwise false
*/
int IsNumber(cell *number)
{
   char *s;			/* pointer required by strtol */
   cell num;			/* conversion field */

   num = strtol(name,&s,0);
   if (*s == '\0')
   {
      *number = num;
      return(MTRUE);
   }
   *number = 0;
   return(MFALSE);
}

/* ------------------------------------------------------------------------
   Get next token and convert it to the passed number
   Returns true if successful, otherwise false
*/
int GetNumber(cell *number)
{
   char *s;			/* see IsNumber() before */
   cell num;

   if (GetNextToken() == MTRUE)
      EarlyTerminate();
   num = strtol(token,&s,0);
   if (*s == '\0')
   {
      *number = num;
      return(MTRUE);
   }
   *number = 0;
   return(MFALSE);
}

/* ------------------------------------------------------------------------
   Align both dictionary pointers to 32 bit boundary
*/
void Align()
{
   code_dp = (code_dp + 3) & -4l;
   name_dp = (name_dp + 3) & -4l;
}

/* ------------------------------------------------------------------------
   Read a 32 bit word from given index in code space
*/
cell Read(address index)
{
   return(*(cell*)(&codespace[index]));
}

/* ------------------------------------------------------------------------
   Write a 32 bit word at given index into code space
*/
void Write(address index, cell value)
{
   cell *cellptr;		/* long pointer to indexed position */

   cellptr = (cell*)&codespace[index];
   *cellptr = value;
}

/* ------------------------------------------------------------------------
   Compile a 32 bit word into code space
*/
void Comp(cell value)
{
   Write(code_dp,value);
   code_dp += 4;
}

/* ------------------------------------------------------------------------
   Compile a 32 bit word into header in namespace
*/
void HComp(address value)
{
   address *headptr;		/* unsigned long pointer to header-here */

   headptr = (address*)&namespace[name_dp];
   *headptr = value;
   name_dp += 4;
}

/* ------------------------------------------------------------------------
   Get a primitive code token from the primtable[]-array in mfpnames.h
*/
address prim(char *primname)
{
   int i;			/* list index */

   for (i=0; i<PRIMNUMBER; i++)
   {
      if (strcmp(primtable[i],primname) == 0)
	 return((address)i);
   }
   LineTerminate("Unrecognized primitive definition");
   return(0);
}

/* ------------------------------------------------------------------------
   Search linked header list for headername like name[]
   Return cfa if found, else FALSE
*/
address FindWord()
{
   address lfa_i;		/* index to current header's lfa */
   address *lfv;		/* pointer to preceding lfa */
   address *cfv;		/* pointer to code field vector */

   lfa_i = last;
   while (lfa_i != 0)		/* first header starts at 4 */
   {
      if (strcmp(name,&namespace[lfa_i+13]) == 0)
      {
	 cfv = (address*)&namespace[lfa_i+4];
	 return(*cfv);
      }
      lfv = (address*)&namespace[lfa_i];
      lfa_i = *lfv;
   }
   return(MFALSE);
}

/* ------------------------------------------------------------------------
   Create a header from name[]
*/
void Create()			/* Codespace:	      Namespace:	 */
{
   int i=0;			/* name char counter			 */

   if (MFALSE != FindWord())
      LineTerminate("Duplicate definition");

   if (name[0]=='_')
      LineTerminate("Definitions must not begin with _underscore");

   current = name_dp;		/*	 lfv	----> lfa: link   --^	  */
   Comp(current);		/* cfa:  doxt	<----	   cfv		 */
   HComp(last); 		/* body: ...		   clen+sln	 */
   HComp(code_dp);		/*		      nfa  ct+fl+namez	 */
   HComp(((address)ln_no)<<16);
   last_cfa = code_dp;
   namespace[name_dp++] = 0x80 | (char)strlen(name);
   do				/* compile name field			 */
   {  namespace[name_dp++] = 0x7f & name[i];
   }  while (name[i++] != '\0');
   Align();
}

/* ------------------------------------------------------------------------
   Finish up and link the current definition
*/
void Reveal()
{
   unsigned int codelen; address* lnptr;

   codelen = code_dp - last_cfa + 4;
   lnptr = (address*)&namespace[current+8];
   *lnptr = (*lnptr) | codelen;
   last = current;		/* now the last word is find-able */
}

/* ------------------------------------------------------------------------
   Compile a constant after _CONST <name> <value>
*/
void DoConst()
{
   static cell value;		/* holds constant value */

   GetWord();			/* fetch name to name[] */
   if (GetNumber(&value) == MFALSE)
      LineTerminate("Bad CONSTANT value");
   Create();
   Comp(xtDOCONST);
   Comp(value);
   Reveal();
}

/* ------------------------------------------------------------------------
   Compile a variable after _VARIABLE <name>
*/
void DoVariable()
{
   GetWord();			/* fetch name to name[] */
   Create();
   Comp(xtDOVAR);
   Comp(0);			/* initialise with 0 */
   Reveal();
}

/* ------------------------------------------------------------------------
   Compile a user variable after _USER <codespace-address>
*/
void DoUser()
{
   static address varaddr;	/* holds variable effective address */
   static int adrflag[64]={0};	/* check for possible collisions */

   GetWord();			/* fetch name to name[] */
   if ((!GetNumber((cell*)&varaddr))||(varaddr > 255)||(varaddr&3))
      LineTerminate("Bad USER variable address");
   if (adrflag[varaddr/4])
      LineTerminate("USER variable address already in use");
   Create();
   Comp(xtDOUSER);
   Comp((cell)varaddr);
   adrflag[varaddr/4] = MTRUE;
   Reveal();
}

/* ------------------------------------------------------------------------
   Compile an execution vector after _DEFER <name>
*/
void DoDefer()
{
   GetWord();
   Create();
   Comp(xtDOVECT);
   Comp(0);
   Reveal();
}

/* ------------------------------------------------------------------------
   Set the Immediate bit in the last definition's vfv after _IMMEDIATE
*/
void DoImmediate()
{
   namespace[last+12] |= 0x40;
}

/* ------------------------------------------------------------------------
   Set the Compile-only bit after _COMPILE-ONLY
*/
void DoCompOnly()
{
   namespace[last+12] |= 0x20;
}
/* ------------------------------------------------------------------------
   Store a value at codespace-address after _STORE <addr> <value>
*/
void DoStore()
{
   static address addr; 	/* where to store */
   static cell value;		/* what to store */

   if (GetNumber((cell*)&addr) == MFALSE)
      LineTerminate("Bad STORE address");
   if (GetNumber(&value) == MFALSE)
      LineTerminate("Bad value to STORE");
   Write(addr,value);
}

/* ------------------------------------------------------------------------
   Link vector with hi-level definition after _REFERS <vectorname>
*/
void DoRefers()
{
   address foundcfa;		/* holds found cfa */
   address *fcfptr;		/* pointer to found word's cfa */
   address *lcfptr;		/* pointer to last cfa */

   GetWord();			/* fetch referred definition to name[] */
   foundcfa = FindWord();
   if (foundcfa == MFALSE)
      LineTerminate("Referred definition not found");
   fcfptr = (address*)&codespace[foundcfa];
   lcfptr = (address*)&codespace[last_cfa];

   if ((last_cfa >= 256) && (*lcfptr == xtDOVECT))
   {				/* link a vector to a previous word */
      lcfptr++;
      *lcfptr = foundcfa;
   }
   else if ((foundcfa >= 256) && (*fcfptr == xtDOVECT))
   {				/* link a word to a previous vector */
      fcfptr++;
      *fcfptr = last_cfa;
   }
   else
      LineTerminate("No vector found to REFER");
}

/* ------------------------------------------------------------------------
   Advance dictionary pointer by n bytes after _ALLOT n
*/
void DoAllot()
{
   static cell value;		/* holds constant value */

   if (GetNumber(&value) == MFALSE)
      LineTerminate("Bad ALLOT value");
   if ((value < 0) || ((value + code_dp) >= CODESPLEN))
      LineTerminate("ALLOT size out of range");
   code_dp += value;
   Align();
}

/* ------------------------------------------------------------------------
   Push HERE on control flow stack
*/
void cfpush(address addr)
{
   if (cfsptr == CFSSIZE)
      LineTerminate("Control flow stack overflow");
   cfs[cfsptr++] = addr;
}

/* ------------------------------------------------------------------------
   Pop address from control flow stack
*/
address cfpop()
{
   if (cfsptr == 0)
      LineTerminate("Control flow stack underflow");
   return(cfs[--cfsptr]);
}

/* ------------------------------------------------------------------------
   Set up a forward jump
*/
void JmpForward()
{
   cfpush(code_dp - 4); 	/* remember address for next ToForward */
}

/* ------------------------------------------------------------------------
   Resolve a forward jump
*/
void ToForward()
{
   Write(cfpop(),(cell)code_dp);
}

/* ------------------------------------------------------------------------
   Set up a backward jump
*/
void ToBackward()
{
   cfpush(code_dp);		/* remember target address */
}

/* ------------------------------------------------------------------------
   Resolve a backward jump
*/
void JmpBackward()
{
   Write(code_dp-4,(cell)cfpop());
}

/* ------------------------------------------------------------------------
   Immediate execution of control words in definitions
*/
void DoControlWord()
{
   address i = 0;		/* literal inline string char counter */

   if	   isctrl("_IF")
   {
      Comp(xtJMPZ);
      Comp(0);
      JmpForward();
   }
   else if isctrl("_ELSE")
   {
      Comp(xtJMP);
      Comp(0);
      ToForward();
      JmpForward();
   }
   else if isctrl("_THEN")

      ToForward();

   else if isctrl("_BEGIN")

      ToBackward();

   else if isctrl("_WHILE")
   {
      Comp(xtJMPZ);
      Comp(0);
      i = cfpop();		/* leave begin marker on top */
      JmpForward();
      cfpush(i);
   }
   else if isctrl("_REPEAT")
   {
      Comp(xtJMP);
      Comp(0);
      JmpBackward();
      ToForward();
   }
   else if isctrl("_UNTIL")
   {
      Comp(xtJMPZ);
      Comp(0);
      JmpBackward();
   }
   else if isctrl("_AGAIN")
   {
      Comp(xtJMP);
      Comp(0);
      JmpBackward();
   }
   else if isctrl("_\"")	/* metacompiler's special string handler */
   {
      if (GetNextToken() == MTRUE)
     EarlyTerminate();
      Comp(xtSLIT);
      codespace[code_dp++] = (char)strlen(token);
      do			/* copy and blank out underscores */
      {  if (token[i] == '_')
	codespace[code_dp++] = ' ';
     else
	codespace[code_dp++] = token[i];
      }  while (token[i++] != '\0');
      Align();
   }
   else if isctrl("_\'")	/* metacompiler's special tick handler */
   {
      GetWord();
      Comp(xtTICK);
      i = FindWord();
      if (i == MFALSE)
	 LineTerminate("Unknown definition");
      Comp((cell)i);
   }
   else if isctrl("_EXIT")
   {
      Comp(xtUNNEST);
   }
   else

      LineTerminate("Unrecognized control word");
}

/* ------------------------------------------------------------------------
   Compile a header for a primitive after _CODE <name> <primtoken>
*/
void DoCode()
{
   address primtok;		/* holds primitive token value */
   cell *cptr;			/* pointer to current cfv */

   GetWord();			/* fetch name to name[] */
   Create();
   code_dp -= 4;		/* primitives don't use code space */
   GetWord();			/* fetch codename to name[] */
   primtok = prim(name);
   cptr = (cell*)&namespace[current+4];
   *cptr = (cell)primtok;	/* replace cfv with token */
   Reveal();
}

/* ------------------------------------------------------------------------
   Inner compile loop within a highlevel definition
*/
void CompileHilevel()
{
   address foundcfa;		/* holds cfa of already defined words */
   static cell number;		/* holds number if word is a number */

   Comp(xtNEST);		/* cfa of a colon definition */
   while (1)
   {
      GetWord();
      foundcfa = FindWord();
      if (foundcfa != MFALSE)

     Comp((cell)foundcfa);	/* compile cfa or primitive token */

      else if (IsNumber(&number) == MTRUE)
      {
     Comp(xtLIT);
     Comp(number);
      }
      else if isctrl("_;")
      {
     Comp(xtUNNEST);
     break;			/* exit loop only when _; encountered */
      }
      else if (name[0] == '_')	/* control words begin with underscore */

     DoControlWord();

      else

     LineTerminate("Unrecognized word in definition");
   }
   if (cfsptr != 0)
      LineTerminate("Unbalanced control flow in last definition");
   Reveal();
}

/* ------------------------------------------------------------------------
   Compile a highlevel Forth colon definition
*/
void DoColonDefinition()
{
   GetWord();
   Create();
   CompileHilevel();
}

/* ------------------------------------------------------------------------
   Compile a :NONAME definition
*/
void DoNoname()
{  address l;

   name[0] = '\0';
   l = last;
   Create();
   CompileHilevel();
   last = l;
}

/* ------------------------------------------------------------------------
   Compile an ALIAS header for an already existing definition
*/
void DoAlias()
{
   address foundcfa;
   cell *cptr;

   GetWord();
   Create();
   GetWord();
   foundcfa = FindWord();
   if (foundcfa != (address)MTRUE)
      {
	code_dp -= 4;
	cptr = (cell*)&namespace[current+4];
	*cptr = (cell)foundcfa; /* replace cfv with synonym cfv */
	last = current;
      }
   else
      LineTerminate("ALIAS word not found");
}

/* ------------------------------------------------------------------------
   Executes defining words and compiler commands
*/
void DoCommand(char *command)
{
	if iscmd("_:")		  DoColonDefinition();
   else if iscmd("_:NONAME")	  DoNoname();
   else if iscmd("_CODE")	  DoCode();
   else if iscmd("_CONST")	  DoConst();
   else if iscmd("_VARIABLE")	  DoVariable();
   else if iscmd("_USER")	  DoUser();
   else if iscmd("_ALIAS")	  DoAlias();
   else if iscmd("_DEFER")	  DoDefer();
   else if iscmd("_REFERS")	  DoRefers();
   else if iscmd("_IMMEDIATE")	  DoImmediate();
   else if iscmd("_COMPILE-ONLY") DoCompOnly();
   else if iscmd("_STORE")	  DoStore();
   else if iscmd("_ALLOT")	  DoAllot();
   else
      LineTerminate("Unrecognized metacompiler command");
}

/* ------------------------------------------------------------------------
   Outer compile loop
*/
void InterpretKernelFile()
{
   char command[MAXWORD+1];	/* holds metacompiler command */

   do
   {  eof_reached = GetNextToken();
      if (eof_reached == MTRUE)
     break;
      strcpy(command,token);
      strupper(command);
      DoCommand(command);
   } while (eof_reached == MFALSE);
}

/* ------------------------------------------------------------------------
   Set up and initialise Forth data spaces for code, headers and cf-stack
*/
void SetupImage()
{
   memset(codespace,0,sizeof(codespace));
   memset(namespace,0,sizeof(namespace));
   memset(cfs,0,sizeof(cfs));
   code_dp = 256,  name_dp = 0; /* start compilation here */
   HComp(-1L);			/* end-marker of wordlists */
}

/* ------------------------------------------------------------------------
   Find system variable cfa and pfa
*/
address FindVarCfa(char *varname)
{
   address foundcfa;

   strcpy(name,varname);
   foundcfa = FindWord();
   if (foundcfa == MFALSE)
      InitTerminate();
   return(foundcfa);
}

address FindVarPfa(char *varname)
{
   return((address)Read(4+FindVarCfa(varname)));
}

/* ------------------------------------------------------------------------
   Initialise Forth system variables after metacompilation
*/
void FinishImage()
{
   address wordlist;

   Write(0,(cell)FindVarCfa("(BOOT)"));
   Write(4,(cell)FindVarCfa("(MAIN)"));
   Write(8,(cell)FindVarCfa("(BYE)"));
   Write(12,-1); /* end MinForth if _BYE returns */

   /* DP, DP-N, LAST, LAST-N, ROOT-WORDLIST, CURRENT, CONTEXT
      LAST-LINK, VOC-LINK, FILE-LINK */

   wordlist = FindVarPfa("TEMP-WORDLIST");
   Write(wordlist,1);
   wordlist = FindVarPfa("ROOT-WORDLIST");
   Write(wordlist,1);
   Write(wordlist+4,last);
   Write(FindVarPfa("CURRENT"),wordlist);
   Write(FindVarPfa("CONTEXT"),wordlist);
   Write(FindVarPfa("VOC-LINK"),wordlist);
   Write(FindVarPfa("FILE-LINK"),(cell)code_dp+8);

   strcpy(name,"kernel.mfc"); /* make first file list entry */
   Create();
   Comp(xtDOVAR);
   Comp(0);
   Comp(0);
   Reveal();

   Write(FindVarPfa("LAST"),last_cfa);
   Write(FindVarPfa("LAST-N"),last);
   Write(FindVarPfa("LAST-LINK"),wordlist+4);
   Write(FindVarPfa("DP"),(cell)code_dp);
   Write(FindVarPfa("DP-N"),(cell)name_dp);
}

/* ------------------------------------------------------------------------
   Save Forth image to output file
*/
void SaveImage()
{  unsigned long magic=0xe8f4b4bdL; /* M4th with hibits set */

   outputfile = fopen("kernel.i","wb");
   if (outputfile == NULL)
      Terminate("Could not create output image file");
   fwrite(&magic,sizeof(cell),1,outputfile);
   fwrite(&code_dp,sizeof(cell),1,outputfile);
   fwrite(&name_dp,sizeof(cell),1,outputfile);
   fwrite(codespace,sizeof(char),code_dp,outputfile);
   fwrite(namespace,sizeof(char),name_dp,outputfile);
   if (fclose(outputfile) == EOF)
      Terminate("Output image file close error");
}

/* ========================================================================
   Main
*/
int main()
{
   printf("MinForth Extendable Kernel Metacompiler");
   printf("\n=======================================");
   fflush(stdout);
   SetupImage();
    OpenKernelFile();
     InterpretKernelFile();
    CloseKernelFile();
   FinishImage();
   SaveImage();
   printf("\nImagefile kernel.i saved.\n");
   fflush(stdout);
   return(0);
}


