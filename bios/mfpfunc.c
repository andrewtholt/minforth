/* ======== MinForth Functions for C-coded Primitives ========

   This file is used by the C-loader MF.C

   !!!	The order and number of the (primfunc[])()-array
   !!!	   must be kept synchronous to MFPTOKEN.H 


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

void (*primfunc[PRIMNUMBER])() =
{
    pPOTHOLE,		/* 0  */

    pDOCONST,		/* 1  */
    pDOVALUE,		/* 2  */
    pDOVAR,		/* 3  */
    pDOUSER,		/* 4  */
    pDOVECT,		/* 5  */
    pNEST,		/* 6  */

    pUNNEST,		/* 7  */
    pEXECUTE,		/* 8  */

    pTRACE,		/* 9  */

    pLIT,		/* 10 */
    pSLIT,		/* 11 */
    pTICK,		/* 12 */

    pJMP,		/* 13 */
    pJMPZ,		/* 14 */
    pJMPV,		/* 15 */

    pAT,		/* 16 */
    pSTORE,		/* 17 */
    pCAT,		/* 18 */
    pCSTORE,		/* 19 */
    pFILL,		/* 20 */
    pMOVE,		/* 21 */

    pRDEPTH,		/* 22 */
    pRPSTORE,		/* 23 */
    pTOR,		/* 24 */
    pRFROM,		/* 25 */
    pRPICK,		/* 26 */

    pDEPTH,		/* 27 */
    pSPSTORE,		/* 28 */

    pDROP,		/* 29 */
    pSWAP,		/* 30 */
    pROT,		/* 31 */
    pROLL,		/* 32 */
    pDUP,		/* 33 */
    pOVER,		/* 34 */
    pPICK,		/* 35 */

    pAND,		/* 36 */
    pOR,		/* 37 */
    pXOR,		/* 38 */
    pLSHIFT,		/* 39 */
    pRSHIFT,		/* 40 */
    pLESS,		/* 41 */
    pEQUAL,		/* 42 */
    pULESS,		/* 43 */

    pPLUS,		/* 44 */
    pMINUS,		/* 45 */
    pSTAR,		/* 46 */
    pDIVMOD,		/* 47 */
    pDPLUS,		/* 48 */
    pDNEGATE,		/* 49 */
    pMUSTAR,		/* 50 */
    pMUDIVMOD,		/* 51 */

    pCOMPARE,		/* 52 */
    pSCAN,		/* 53 */
    pTRIM,		/* 54 */
    pUPPER,		/* 55 */

    pEMITQ,		/* 56 */
    pTYPE,		/* 57 */

    pRAWKEYQ,		/* 58 */
    pRAWKEY,		/* 59 */

    pMSECS,		/* 60 */
    pTIMEDATE,		/* 61 */

    pNOPEN,		/* 62 */
    pNRENAME,		/* 63 */
    pNDELETE,		/* 64 */
    pNSTAT,		/* 65 */
    pHCLOSE,		/* 66 */
    pHSEEK,		/* 67 */
    pHTELL,		/* 68 */
    pHSIZE,		/* 69 */
    pHCHSIZE,		/* 70 */
    pHREAD,		/* 71 */
    pHWRITE,		/* 72 */

    pDTOF,		/* 73 */
    pFTOD,		/* 74 */
    pFLOOR,		/* 75 */
    pFPSTORE,		/* 76 */    
    pFDEPTH,		/* 77 */
    pFPICK,		/* 78 */
    pFROLL,		/* 79 */
    pFPLUS,		/* 80 */
    pFMINUS,		/* 81 */
    pFSTAR,		/* 82 */
    pFDIV,		/* 83 */
    
    pREPRESENT, 	/* 84 */
    pTOFLOAT,		/* 85 */
    
    pFSTORE,		/* 86 */
    pFAT,		/* 87 */
    pSFSTORE,		/* 88 */
    pSFAT,		/* 89 */
    pFLIT,		/* 90 */

    pFZLESS,		/* 91 */
    pFZEQUAL,		/* 92 */

    pFSQRT,		/* 93 */
    pFEXP,		/* 94 */
    pFLOG,		/* 95 */
    pFSIN,		/* 96 */
    pFASIN,		/* 97 */

    pPRIMTOXT,		/* 98 */
    pXTTOPRIM,		/* 99 */
    pTCTOERRMSG,	/* 100 */

    pSEARCHTHREAD,	/* 101 */
    pSEARCHNAMES,	/* 102 */

    pOSCOMMAND, 	/* 103 */
    pOSRETURN,		/* 104 */
    pGETENV,		/* 105 */
    pPUTENV,		/* 106 */
    pOSTYPE,		/* 107 */

    pRESIZEFORTH, 	/* 108 */

    pTICKER,		/* 109 */
    
/*  --------------------------
    BIOS extension for DOS PCs
    -------------------------- */

    p_FAR,		/* 110 */
    p_BAT,		/* 111 */
    p_WAT,		/* 112 */
    p_BSTORE,		/* 113 */
    p_WSTORE,		/* 114 */
    p_BINP,		/* 115 */
    p_WINP,		/* 116 */
    p_BOUTP,		/* 117 */
    p_WOUTP,		/* 118 */
    p_INTR		/* 119 */
};
