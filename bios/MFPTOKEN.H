/* ======= MinForth Execution Tokens for C-coded Primitives =======

   This file is included by
     mfpnames.h   for the kernel-metacompiler and decompiler
     mfpfunc.h	  for the MinForth virtual machine.

   Token values are indices into arrays. For efficiency
   they should be kept consecutive.


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


#define PRIMNUMBER    120  /* total number of execution tokens */

#define EXECNUMBER	7  /* number of tokens in hilevel cfa's */

#define xtPOTHOLE	0

#define xtDOCONST	1
#define xtDOVALUE	2
#define xtDOVAR 	3
#define xtDOUSER	4
#define xtDOVECT	5
#define xtNEST		6

/* only the following tokens can appear within hilevel words */

#define xtUNNEST	7
#define xtEXECUTE	8

#define xtTRACE 	9
 
#define xtLIT		10
#define xtSLIT		11
#define xtTICK		12

#define xtJMP		13
#define xtJMPZ		14
#define xtJMPV		15

#define xtAT		16
#define xtSTORE 	17
#define xtCAT		18
#define xtCSTORE	19
#define xtFILL		20
#define xtMOVE		21

#define xtRDEPTH	22
#define xtRPSTORE	23
#define xtTOR		24
#define xtRFROM 	25
#define xtRPICK 	26

#define xtDEPTH 	27
#define xtSPSTORE	28

#define xtDROP		29
#define xtSWAP		30
#define xtROT		31
#define xtROLL		32
#define xtDUP		33
#define xtOVER		34
#define xtPICK		35

#define xtAND		36
#define xtOR		37
#define xtXOR		38
#define xtLSHIFT	39
#define xtRSHIFT	40
#define xtLESS		41
#define xtEQUAL 	42
#define xtULESS 	43

#define xtPLUS		44
#define xtMINUS 	45
#define xtSTAR		46
#define xtDIVMOD	47
#define xtDPLUS 	48
#define xtDNEGATE	49
#define xtMUSTAR	50
#define xtMUDIVMOD	51

#define xtCOMPARE	52
#define xtSCAN		53
#define xtTRIM		54
#define xtUPPER 	55

#define xtEMITQ 	56
#define xtTYPE		57

#define xtRAWKEYQ	58
#define xtRAWKEY	59

#define xtMSECS 	60
#define xtTIMEDATE	61

#define xtNOPEN 	62
#define xtNRENAME	63
#define xtNDELETE	64
#define xtNSTAT 	65
#define xtHCLOSE	66
#define xtHSEEK 	67
#define xtHTELL 	68
#define xtHSIZE 	69
#define xtHCHSIZE	70
#define xtHREAD 	71
#define xtHWRITE	72

#define xtDTOF		73
#define xtFTOD		74
#define xtFLOOR 	75
#define xtFPSTORE	76
#define xtFDEPTH	77
#define xtFPICK 	78
#define xtFROLL 	79
#define xtFPLUS 	80
#define xtFMINUS	81
#define xtFSTAR 	82
#define xtFDIV		83

#define xtREPRESENT	84
#define xtTOFLOAT	85

#define xtFSTORE	86
#define xtFAT		87
#define xtSFSTORE	88
#define xtSFAT		89
#define xtFLIT		90

#define xtFZLESS	91
#define xtFZEQUAL	92

#define xtSQRT		93
#define xtEXP		94
#define xtLOG		95
#define xtSIN		96
#define xtASIN		97

#define xtPRIMTOXT	98
#define xtXTTOPRIM	99
#define xtTCTOERRMSG	100

#define xtSEARCHTHREAD	101
#define xtSEARCHNAMES	102

#define xtOSCOMMAND	103
#define xtOSRETURN	104
#define xtGETENV	105
#define xtPUTENV	106
#define xtOSTYPE	107

#define xtRESIZEFORTH	108

#define xtTICKER	109

/* BIOS extension for DOS PCs */

#define	xt_FAR		110
#define	xt_BAT		111
#define	xt_WAT		112
#define	xt_BSTORE	113
#define	xt_WSTORE	114
#define	xt_BINP		115
#define	xt_WINP		116
#define	xt_BOUTP	117
#define	xt_WOUTP	118
#define	xt_INTR		119
