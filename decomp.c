/* ========================================================================
   DECOMP:
      Decompiles MinForth kernel image files for diagnostics scanning the
      header list beginning from namespace start. Output is written to a
      logfile and optionally to stdout.
   Command line:
      DECOMP [-v]  switch -v for verbose output to screen
   Files:
      kernel.i	  in   binary compiled Forth image file
      kernel.d	  out  ascii file with addresses, execution tokens and
			header information


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
#include <string.h>
#include <ctype.h>

/* ========================================================================
   Forth primitive execution tokens < 256
*/
#include "mfcomp.h"
#include "mfpnames.c"

/* ------------------------------------------------------------------------
   Definitions
*/
typedef long		cell;
typedef unsigned long	address;

#define uint		unsigned int

#define TRUE	-1
#define FALSE	0

int	verbose = FALSE;	/* verbose flag */

FILE	*imgfile;		/* Forth image input file */
FILE	*outfile;		/* ascii dump output file */

char	name[64];		/* holds imagefile name */
char	line[80];		/* holds dump lines */

char	*codespace;		/* pointer to codespace array */
address code_ofs=256;		/* index into codespace */
address codesize;		/* codespace size as read from image */

char	*namespace;		/* pointer to namespace array */
address name_ofs=4;		/* index into namespace */
address namesize;		/* namespace size as read from image */

address codelen;		/* actual code length */

/* ========================================================================
   FUNCTIONS & PROCEDURES

   Terminate program by brute force with error message
*/
void Terminate(char *message)
{
   fprintf(stderr,"\n?: %s\n%s",message,"Aborted.");
   if (codespace != NULL)
      free(codespace);
   exit(EXIT_FAILURE); /* close all open files and quit */
}

/* ------------------------------------------------------------------------
   Create and close dump file
*/
void CreateDumpFile()
{
   strcpy(line,name);
   strcat(line,".d");
   outfile = fopen(line,"wt");
   if (outfile == NULL)
      Terminate("Error opening dumpfile");
   printf("\nDumpfile opened.");
   fflush(stdout);
   fprintf(outfile,"------------------------------\n");
   fprintf(outfile,"  MinForth kernel-image dump\n");
   fprintf(outfile,"------------------------------");
}

void CloseDumpFile()
{
   if (fclose(outfile) == EOF)
      Terminate("Error closing dumpfile");
   printf("\nDumpfile closed.");
   fflush(stdout);
}

/* ------------------------------------------------------------------------
   Get an address from the image
*/
address GetAddress()
{
   address value;

   fread(&value,sizeof(address),1,imgfile);
   return(value);
}

/* ------------------------------------------------------------------------
   Read a cell from codespace or namespace
*/
cell ReadCode(address cindex)
   { return(*(cell*)(codespace+cindex)); }

address ReadHead(address nindex)
   { return(*(address*)(namespace+nindex)); }

/* ------------------------------------------------------------------------
   Output dump to dumpfile and stdout, if verbose
*/
void DumpStr(char *format, char *text)
{
   if (verbose == TRUE)
   {
      printf(format,text);
      fflush(stdout);
   }
   if (fprintf(outfile,format,text) == EOF)
      Terminate("Dumpfile write error");
}

void DumpNum(char *format, cell number)
{
   static char outstring[48];

   sprintf(outstring,format,number);
   DumpStr("%s",outstring);
}

/* ------------------------------------------------------------------------
   Dump a string
*/
void StrDump(char *text)
   { DumpStr("%s",text); }

/* ------------------------------------------------------------------------
   Dump a cell
*/
void CellDump(cell value)
  { DumpNum("%ld",value); }

void HexCellDump(cell value)
  { DumpNum("%2X",value); }

/* ------------------------------------------------------------------------
   Dump a cell in long hex format
*/
void HexDump(cell value)
{
   DumpNum("%04X",(value>>16)&0xffff);
   DumpNum(".%04X",value&0xffff);
}

/* ------------------------------------------------------------------------
   Some useful dumps
*/
void Separator()
   { StrDump("\n------------------------------"); }

void TabDump()
   { StrDump("\t"); }

void PreCDump()
   { StrDump("\nC: "); }

/* ------------------------------------------------------------------------
   Dump next line in namespace, advance index to next cell
*/
void NameCellDump()
{
   StrDump("\nN: ");
   HexDump(name_ofs);
   TabDump();
   name_ofs += 4;
}

/* ------------------------------------------------------------------------
   Dump next line in codespace, advance index to next cell
*/
void CodeCellDump()
{
   StrDump("\nC: ");
   HexDump(code_ofs);
   TabDump();
   code_ofs += 4;
}

cell GetCodeCellDump()
{
   cell value;

   value = ReadCode(code_ofs);
   CodeCellDump();
   HexDump(value);
   return(value);
}

/* ------------------------------------------------------------------------
   Dump the body of a CONSTANT
*/
void DumpConstant()
{
   StrDump("_DOCONST");
   DumpNum("\t%ld",GetCodeCellDump());
}

/* ------------------------------------------------------------------------
   Dump the body of a USER variable
*/
void DumpUser()
{
   cell content;

   StrDump("_USER");
   content = ReadCode(GetCodeCellDump());
   TabDump();
   HexDump(content);
   DumpNum(" = %ld",content);
}

/* ------------------------------------------------------------------------
   Dump the body of a VARIABLE
*/
void DumpVariable()
{
   StrDump("_DOVAR");
   DumpNum("\t%ld",GetCodeCellDump());
}

/* ------------------------------------------------------------------------
   Dump the body of a DEFERed execution vector
*/
void DumpVector()
{
   address whereto, lfv;		 /* cfa of referred target */
   char* n;

   StrDump("_DOVECT");
   whereto = GetCodeCellDump();
   lfv = ReadCode(whereto-4);
   StrDump("  -->  ");
   if (whereto < 256)
     StrDump("unreferred");
   else
   { n = namespace+lfv+13;
     if ('\0' == *n) StrDump(":NONAME"); else StrDump(n);
   }
}

/* ------------------------------------------------------------------------
   Special dumps for control flow affecting tokens within definitions
*/
void DumpInlines(cell token)
{
   address value;

   switch(token)
   {
      case xtLIT:

	 value = GetCodeCellDump();
	 DumpNum("\t%ld",value);
	 break;

      case xtJMP:
      case xtJMPZ:
      case xtJMPV:

	 value = GetCodeCellDump();
	 if (value < code_ofs) StrDump("  ^--");
	 else		       StrDump("  v--");
	 break;

      case xtSLIT:

	 CodeCellDump();
	 value = (address)*(codespace+code_ofs-4);
	 StrDump("     ");
	 CellDump(value);
	 DumpStr("\t\t%s",codespace+code_ofs-3);
	 code_ofs += (value-2);
	 while ((code_ofs & 3) != 0) code_ofs++;
	 break;

      case xtTICK:

	 value = GetCodeCellDump();
	 TabDump();
	 if (value >= 256)
	    StrDump(namespace+ReadCode(value-4)+1);
	 else if (value < PRIMNUMBER)
	    StrDump(primtable[value]);
	 else
	    Terminate("Bad ticked cfa");
	 break;
   }
}

/* ------------------------------------------------------------------------
   Dump the compiled word sequence of a hilevel Forth definition
*/
void DumpHilevel()
{
   cell token; address codeend;

   StrDump("_NEST");
   codeend = code_ofs + codelen - 12;

   do
   {
      token = GetCodeCellDump();
      TabDump();
      if (token < 256)
      {
	 if (token < PRIMNUMBER)
	 {
	    StrDump(primtable[token]);
	    DumpInlines(token);
	 }
	 else
	 {
	    StrDump("  <---  bad code token");
	    Terminate("Forth image file error");
	 }
      }
      else

	 StrDump(namespace+ReadCode(token-4)+13);

   } while (code_ofs <= codeend);
}

/* ------------------------------------------------------------------------
   Analyze and dump the codespace of Forth definitions
   Increment code_ofs on the fly
*/
void CodeWalk()
{
   cell content;

   (void)GetCodeCellDump();
   StrDump("  nfv");

   content = GetCodeCellDump();
   TabDump();

   switch(content)
   {
      case xtNEST:	DumpHilevel();	break;

      case xtDOUSER:	DumpUser();	break;

      case xtDOVAR:	DumpVariable(); break;

      case xtDOCONST:	DumpConstant(); break;

      case xtDOVECT:	DumpVector();	break;

      default:
	 StrDump("  <--  bad code token");
	 Terminate("Forth image file error");
   }
}

/* ------------------------------------------------------------------------
   Analyze and dump the Forth definition pointed to by name_ofs
   Increment name_ofs to point to following definition
*/
void NameWalk()
{
   address lfv, cfv, vfv;
   cell allotted;
   unsigned char count; char* n;

   lfv = ReadHead(name_ofs);
   cfv = ReadHead(name_ofs+4);

   allotted = cfv-4-code_ofs;
   if (allotted>0 && cfv>=256)
   {
      Separator();
      StrDump("\n   :NONAME or ALLOTed ");
      CellDump(allotted);
      code_ofs += allotted;
   }

   Separator();
   NameCellDump();
   HexDump(lfv);
   StrDump("  lfv");

   NameCellDump();
   HexDump(cfv);
   StrDump("  cfv");
   if (cfv < 256)
   {
      DumpStr("  code %s ",primtable[cfv]);
      CellDump(cfv);
   }
   else if (cfv < code_ofs)
   {  
      StrDump("  alias ");
      StrDump(namespace+ReadCode(cfv-4)+13);
   }
   vfv = ReadHead(name_ofs);
   NameCellDump();
   HexDump(vfv);
   codelen = vfv & 0xffff;
   StrDump("\tcodelen ");
   CellDump(codelen);
   StrDump(" sline ");
   CellDump(vfv >> 16);
   
   count = *(namespace+name_ofs);
   NameCellDump();
   StrDump("	   ");
   HexCellDump(count);
   n = (char*)(namespace+name_ofs-3);
   if ('\0' == *n) DumpStr("\t%s",":NONAME"); else DumpStr("\t%s",n);

   if (count & 0x40)
      StrDump(" immediate");
   if (count & 0x20)
      StrDump(" compile-only");

   count &= 0x1f;
   name_ofs += (count - 2);
   while ((name_ofs & 3) != 0) name_ofs++;

   if (cfv >= code_ofs)
      CodeWalk();
}

/* ------------------------------------------------------------------------
   Dissect the image file and produce results to dumpfile or stdout, if
      verbose option is set
*/
void DissectImageFile()
{
   address totalsize;

   codesize = GetAddress();
    StrDump("\nCodespace size ");
    HexDump(codesize);
    DumpNum(" = %ld bytes",codesize);
   namesize = GetAddress();
    StrDump("\nNamespace size ");
    HexDump(namesize);
    DumpNum(" = %ld bytes",namesize);
   totalsize = namesize+codesize;

   codespace = (char *)malloc(totalsize);
   if (codespace == NULL)
      Terminate("Can't allocate Forthspace");
   namespace = codespace + codesize;

   fread(codespace,1,totalsize,imgfile);

   while(name_ofs < namesize)
      NameWalk();		/* dump all words */

   Separator();

   free(codespace);
}

/* ------------------------------------------------------------------------
   Open and close image file, evaluate command line parameters
*/
void OpenImageFile(int argc, char **argv)
{
   address tok;

   switch (argc)
   {
   case 1: break;

   case 2: if ((0==strcmp(argv[1],"-v"))||(0==strcmp(argv[1],"-V")))
	      verbose = TRUE;
	   break;

   default:
	   Terminate("Bad number of command line parameters");
   }
   strcpy(name,"kernel");

   imgfile = fopen("kernel.i","rb");
   if (imgfile == NULL)
   {
      fprintf(stderr,"\n? : Error opening imagefile %s\nAborted.\n",line);
      exit(EXIT_FAILURE);
   }
   fread(&tok,sizeof(long),1,imgfile);
   if (tok != 0xe8f4b4bdL)	/* check magic number */
      Terminate("Not a MinForth kernel image");
}

void CloseImageFile()
{
   if (fclose(imgfile) == EOF)
       Terminate("Error closing imagefile");
}

/* ------------------------------------------------------------------------
   Main
*/
int main(int argc, char **argv)
{
   printf("MinForth Kernel Image Dump Utility\n");
   printf("==================================");
   fflush(stdout);
   OpenImageFile(argc,argv);
    CreateDumpFile();
     DissectImageFile();
    CloseDumpFile();
   CloseImageFile();
   printf("\nImage dumped.\n");
   return(0);
}

