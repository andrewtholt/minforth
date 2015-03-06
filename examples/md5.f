0 [IF]

MD5 routine in ANS FORTH  by Fredrick W Warren  02Nov2000
---------------------------------------------------------

Hi, I posted earlier this week a request for MD5 written in
ANS Forth. I have written an implementation myself, with several
improvements over the C version. By making this post, I hope to
accomplish several things, one of them being this source code being
archived in DEJANEWS. The other, is a chance for the Forth community
to critique my work. I am by no means a Forth Guru and am always
looking for a way to improve my coding style.

USAGE:
If you have the data you wish to get an MD5 hash for, be it a string,
or a file, completely loaded in memory, use MD5Full

     S" Hash this sentance" MD5Full  MD5String TYPE

If you are reading from a file or keyboard, you can send chuncks of
data to MD5Update and then call MD5Final after sending the last chunk.
The improvement here is that the chuncks can be any size desired, they
do not have to be multiples of 64. Also MD5Int is set to run after
MD5String has been called to retrive the hash.

     S" Hash thi" MD5Update
     S" s Sent" MD5Update
     S" ance" MD5Final  MD5String TYPE

The finaly goodie is MD5Test generates the test vectors to verify that
the hasing is working fine.

May someone else find this code useful.

[THEN]

\ : .S ( -- ) \ Display stack contents
\   DEPTH IF DEPTH 0 DO I PICK U. LOOP THEN ;

VARIABLE A       VARIABLE AA
VARIABLE B       VARIABLE BB
VARIABLE C       VARIABLE CC
VARIABLE D       VARIABLE DD

VARIABLE MD5LEN

CREATE BUF[] 64 ALLOT
CREATE PART[] 64 ALLOT
CREATE MD5PAD 64 ALLOT   MD5PAD 64 0 FILL  128 MD5PAD C!

: LROLL ( n1 s1 -- res ) \ roll left with c/o to bit 0
  2DUP 32 SWAP - RSHIFT  ROT ROT LSHIFT OR ;

: F() ( n1 n2 n3 -- n4 )
  ROT DUP NOT ROT AND ROT ROT AND OR ;

: G() ( n1 n2 n3 -- n4 )
  SWAP OVER NOT AND ROT ROT AND OR ;

: H() ( n1 n2 n3 -- n4 )
  XOR XOR ;

: I() ( n1 n2 n3 -- n4 )
  NOT ROT OR XOR ;

: FF() ( a b c d ac x s -- res )
  >R  CELLS BUF[] + @ + >R
  2OVER NIP >R F() + R> SWAP R> + R> LROLL + ;

: GG() ( a b c d ac x s -- res )
  >R  CELLS BUF[] + @ +  >R
  2OVER NIP >R  G() + R> SWAP R> + R> LROLL + ;

: HH() ( a b c d ac x s -- res )
  >R  CELLS BUF[] + @ + >R
  2OVER NIP >R  H() + R> SWAP R> + R> LROLL + ;

: II() ( a b c d ac x s -- res )
  >R  CELLS BUF[] + @ + >R
  2OVER NIP >R  I() + R> SWAP R> + R> LROLL + ;


HEX
: ROUND1 ( -- )
  a @ b @ c @ d @ 0d76aa478 00 07 FF() a ! \ 1
  d @ a @ b @ c @ 0e8c7b756 01 0C FF() d ! \ 2
  c @ d @ a @ b @ 0242070db 02 11 FF() c ! \ 3
  b @ c @ d @ a @ 0c1bdceee 03 16 FF() b ! \ 4
  a @ b @ c @ d @ 0f57c0faf 04 07 FF() a ! \ 5
  d @ a @ b @ c @ 04787c62a 05 0C FF() d ! \ 6
  c @ d @ a @ b @ 0a8304613 06 11 FF() c ! \ 7
  b @ c @ d @ a @ 0fd469501 07 16 FF() b ! \ 8
  a @ b @ c @ d @ 0698098d8 08 07 FF() a ! \ 9
  d @ a @ b @ c @ 08b44f7af 09 0C FF() d ! \ 10
  c @ d @ a @ b @ 0ffff5bb1 0A 11 FF() c ! \ 11
  b @ c @ d @ a @ 0895cd7be 0B 16 FF() b ! \ 12
  a @ b @ c @ d @ 06b901122 0C 07 FF() a ! \ 13
  d @ a @ b @ c @ 0fd987193 0D 0C FF() d ! \ 14
  c @ d @ a @ b @ 0a679438e 0E 11 FF() c ! \ 15
  b @ c @ d @ a @ 049b40821 0F 16 FF() b ! \ 16
  ;

: ROUND2 ( -- )
  a @ b @ c @ d @ 0f61e2562 01 05 GG() a ! \ 1
  d @ a @ b @ c @ 0c040b340 06 09 GG() d ! \ 2
  c @ d @ a @ b @ 0265E5A51 0B 0E GG() c ! \ 3
  b @ c @ d @ a @ 0e9b6c7aa 00 14 GG() b ! \ 4
  a @ b @ c @ d @ 0d62f105d 05 05 GG() a ! \ 5
  d @ a @ b @ c @  02441453 0A 09 GG() d ! \ 6
  c @ d @ a @ b @ 0d8a1e681 0F 0E GG() c ! \ 7
  b @ c @ d @ a @ 0e7d3fbc8 04 14 GG() b ! \ 8
  a @ b @ c @ d @ 021e1cde6 09 05 GG() a ! \ 9
  d @ a @ b @ c @ 0c33707d6 0E 09 GG() d ! \ 10
  c @ d @ a @ b @ 0f4d50d87 03 0E GG() c ! \ 11
  b @ c @ d @ a @ 0455a14ed 08 14 GG() b ! \ 12
  a @ b @ c @ d @ 0a9e3e905 0D 05 GG() a ! \ 13
  d @ a @ b @ c @ 0fcefa3f8 02 09 GG() d ! \ 14
  c @ d @ a @ b @ 0676f02d9 07 0E GG() c ! \ 15
  b @ c @ d @ a @ 08d2a4c8a 0C 14 GG() b ! \ 16
  ;

: ROUND3 ( -- )
  a @ b @ c @ d @ 0fffa3942 05 04 HH() a ! \ 1
  d @ a @ b @ c @ 08771f681 08 0B HH() d ! \ 2
  c @ d @ a @ b @ 06d9d6122 0B 10 HH() c ! \ 3
  b @ c @ d @ a @ 0fde5380c 0E 17 HH() b ! \ 4
  a @ b @ c @ d @ 0a4beea44 01 04 HH() a ! \ 5
  d @ a @ b @ c @ 04bdecfa9 04 0B HH() d ! \ 6
  c @ d @ a @ b @ 0f6bb4b60 07 10 HH() c ! \ 7
  b @ c @ d @ a @ 0bebfbc70 0A 17 HH() b ! \ 8
  a @ b @ c @ d @ 0289b7ec6 0D 04 HH() a ! \ 9
  d @ a @ b @ c @ 0eaa127fa 00 0B HH() d ! \ 10
  c @ d @ a @ b @ 0d4ef3085 03 10 HH() c ! \ 11
  b @ c @ d @ a @  04881d05 06 17 HH() b ! \ 12
  a @ b @ c @ d @ 0d9d4d039 09 04 HH() a ! \ 13
  d @ a @ b @ c @ 0e6db99e5 0C 0B HH() d ! \ 14
  c @ d @ a @ b @ 01fa27cf8 0F 10 HH() c ! \ 15
  b @ c @ d @ a @ 0c4ac5665 02 17 HH() b ! \ 16
  ;

: ROUND4 ( -- )
  a @ b @ c @ d @ 0f4292244 00 06 II() a ! \ 1
  d @ a @ b @ c @ 0432aff97 07 0A II() d ! \ 2
  c @ d @ a @ b @ 0ab9423a7 0E 0F II() c ! \ 3
  b @ c @ d @ a @ 0fc93a039 05 15 II() b ! \ 4
  a @ b @ c @ d @ 0655b59c3 0C 06 II() a ! \ 5
  d @ a @ b @ c @ 08f0ccc92 03 0A II() d ! \ 6
  c @ d @ a @ b @ 0ffeff47d 0A 0F II() c ! \ 7
  b @ c @ d @ a @ 085845dd1 01 15 II() b ! \ 8
  a @ b @ c @ d @ 06fa87e4f 08 06 II() a ! \ 9
  d @ a @ b @ c @ 0fe2ce6e0 0F 0A II() d ! \ 10
  c @ d @ a @ b @ 0a3014314 06 0F II() c ! \ 11
  b @ c @ d @ a @ 04e0811a1 0D 15 II() b ! \ 12
  a @ b @ c @ d @ 0f7537e82 04 06 II() a ! \ 13
  d @ a @ b @ c @ 0bd3af235 0B 0A II() d ! \ 14
  c @ d @ a @ b @ 02ad7d2bb 02 0F II() c ! \ 15
  b @ c @ d @ a @ 0eb86d391 09 15 II() b ! \ 16
  ;
DECIMAL

: Transform ( -- )
  a @ b @   c @ d @  ROUND1 ROUND2 ROUND3 ROUND4
  d @ + d !  c @ + c !    b @ + b !  a @ + a !  ;

HEX
: MD5INT ( -- )
  067452301 a !   0efcdab89 b !
  098badcfe c !   010325476 d ! 
  0 MD5LEN !  ;
DECIMAL

-1 VALUE MD5INT?
 
: SETLEN ( count -- )
  MD5LEN @ 8 M*  BUF[] 60 + ! BUF[] 56 + ! ;

\ Do all 64 byte blocks leaving remainder block
: DOFULLBLOCKS ( adr1 count1 --  adr2 count2 )
  BEGIN  DUP 63 >
  WHILE  64 - SWAP DUP BUF[] 64 MOVE
         64 + SWAP Transform
  REPEAT ;

: MOVEPARTIAL ( addr count -- )
  SWAP OVER BUF[] SWAP MOVE
  MD5PAD OVER BUF[] + ROT 64 SWAP - MOVE ;

: DOFINAL ( addr count -- )
  2DUP MOVEPARTIAL DUP 55 >  
  IF  Transform  BUF[] 64 0 FILL THEN
  2DROP SETLEN Transform  ;

\ compute MD5 from a counted buffer of text
: MD5Full ( addr count -- )
  MD5INT DUP MD5LEN +!  DOFULLBLOCKS DOFINAL ;

: SAVEPART ( adr count -- ) 
  MD5LEN @ 64 MOD IF  PART[] SWAP MOVE  ELSE  2DROP  THEN  ;

: MOVEPART ( adr1 count1 partindex -- adr2 count2 ) \ add to PART[]
  2DUP 64 SWAP - MIN >R  PART[] + >R OVER R> R@ MOVE
  SWAP R@ + SWAP R> - ;

: MD5Update ( adr count -- ) 
  MD5INT? IF MD5INT FALSE TO MD5INT? THEN
  MD5LEN @ 64 MOD OVER MD5LEN +! ( adr count partindex -- )
  DUP IF    2DUP + 63 >
            IF    MOVEPART PART[] 64 DOFULLBLOCKS  DOFULLBLOCKS
		  SAVEPART CR
            ELSE  MOVEPART 2DROP THEN
      ELSE  DROP DOFULLBLOCKS SAVEPART THEN ;


: MD5Final ( adr count -- ) 
  MD5INT? IF MD5INT FALSE TO MD5INT? THEN
  MD5LEN @ 64 MOD OVER MD5LEN +! ( adr count partindex -- )
  DUP IF    2DUP + 63 >
            IF    MOVEPART PART[] 64 DOFULLBLOCKS  DOFULLBLOCKS
		  DOFINAL
            ELSE  MOVEPART 2DROP PART[] MD5LEN @ 64 MOD DOFINAL THEN
      ELSE  DROP DOFULLBLOCKS DOFINAL THEN ;



\ Functions for creating output string
CREATE DIGIT$ 
  48 c, 49 c, 50 c, 51 c, 52 c, 53 c, 54 c, 55 c, 56 c, 57 c,
  97 c, 98 c, 99 c, 100 c, 101 c, 102 c,

: INTDIGITS ( -- )
  0 PAD ! ;
: SAVEDIGIT ( n -- ) \ output digit at pad
  PAD C@ 1+ DUP PAD C! PAD + C! ;
: BYTEDIGITS ( n1 -- )
  DUP 4 RSHIFT DIGIT$ + C@ SAVEDIGIT  15 AND DIGIT$ + C@ SAVEDIGIT ;
: CELLDIGITS ( a1 -- )
  DUP 4 + SWAP DO I C@ BYTEDIGITS LOOP ;
: MD5STRING ( -- adr count ) \ return address of counted MD5 string
  INTDIGITS a CELLDIGITS b CELLDIGITS c CELLDIGITS d CELLDIGITS PAD
  COUNT
  TRUE TO MD5INT? ;


\ Test Suite ------------------------------------------------------------------------

: QuoteString ( adr count -- )
  CR 34 EMIT TYPE 34 EMIT ;

: .MD5 ( adr count -- )
  CR CR 2DUP MD5Full MD5String TYPE SPACE QuoteString ;

: FOO ( -- )
  S" foo" r/o OPEN-FILE 0=
  IF  BEGIN  DUP PAD 1024 ROT READ-FILE DROP DUP 1024 =
      WHILE  PAD SWAP MD5Update 
      REPEAT PAD SWAP MD5Final 
      CLOSE-FILE DROP CR CR MD5String TYPE ."  foo"
  ELSE DROP
  THEN ;

: MD5Test ( -- )
  CR ." --- MD5 test suite results:  ---"
  S" "  .MD5
  S" a" .MD5
  S" abc" .MD5
  S" Hello world!" .MD5
  S" abcdefghijklmnopqrstuvwxyz" .MD5
  S" ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
     .MD5
\  S" 12345678901234567890123456789012345678901234567890123456789012345678901234567890"
\     .MD5 
  FOO CR CR ;

MD5TEST


