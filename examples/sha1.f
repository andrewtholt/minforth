\ ---------------------------------------------------------------------
\ ANS Forth implementation of the Secure Hash Algorithm SHA-1
\ See FIPS PUB 180-1 -- www.itl.nist.gov/fipspubs/fip180-1.htm
\ SHA-1 required for Digital Signature Algorithm (DSA) and the
\ Digital Signature Standard (DSS) and secure federal hash applications.
\ Assumes Little Endian, byte addressable CPU, e.g. Pentium class
\ DEPENDENCIES: CORE EXT WORDSET
\ Permission for use of this code is free
\ subject to acknowledgment of copyright.
\ Copyright (c) 2000 Jabari Zakiya -- jzakiya@mail.com  7/1/2000

REQUIRES facility

  DECIMAL
  32 CONSTANT CELLSIZE    \ CPU bitsize


  CREATE MCount  2 CELLS ALLOT   \ Holds message bits count 0 - <2^64
  CREATE SHAval  5 CELLS ALLOT   \ Holds hash value after each message
  CREATE SHAsh  85 CELLS ALLOT   \ Fully extended hash array
  CREATE W      80 CELLS ALLOT   \ Fully extended message array

  HEX

    5a827999  CONSTANT  K1   \ Constant for rounds  0 - 19
    6ed9eba1  CONSTANT  K2   \ Constant for rounds 20 - 39
    8f1bbcdc  CONSTANT  K3   \ Constant for rounds 40 - 59
    ca62c1d6  CONSTANT  K4   \ Constant for rounds 60 - 79

  DECIMAL

\ Create words and phrases to produce optimized inline compiled code
  0 VALUE  litval
: ]L  S" TO litval litval ] LITERAL " EVALUATE ; IMMEDIATE

  0 VALUE H[E]  \ Pointer to array addr of hash value E for each round

: H[C] ( - adr[C] )  S" H[E]  [ 2 CELLS ]L  +"  EVALUATE  ;  IMMEDIATE
: H[B] ( - adr[B] )  S" H[E]  [ 3 CELLS ]L  +"  EVALUATE  ;  IMMEDIATE
: rol  ( n #  -  u)  2DUP  LSHIFT  -ROT CELLSIZE  -  NEGATE RSHIFT OR ;


: InitHash ( -)  \ Load initial hash values H0 - H4
  [ HEX ] 67452301 ( H0)  efcdab89 ( H1)  98badcfe ( H2)
          10325476 ( H3)  c3d2e1f0 ( H4)  [ DECIMAL ]
  SHAsh  4 0 DO TUCK ! CELL+ LOOP !  \ Store initial hash in SHAsh array
  SHAsh  SHAval  5 CELLS  MOVE       \ Store copy in SHAval array
  SHAsh  TO  H[E]                    \ Init pointer to last hash value
;

: UpDateHash ( -)  \ Compute hash, update hash arrays with new values
  SHAsh  SHAval  H[E]                 \ Place array addresses on stack
  5 0 DO DUP >R @ SWAP DUP >R @ + DUP \ Compute updated hash subvalue
  R@ ! OVER ! CELL+ R> CELL+ R> CELL+ \ Store updated hash subvalue
  LOOP  3DROP                         \ Clear stack when done
  SHAsh  TO  H[E]                     \ Init pointer to last subvalue
;

: Wexpand ( -)  \ Create fully expanded subkey array
  [ W 16 CELLS + ]L                           \ Start at W[16]
  64 0 DO DUP >R [  3 CELLS ]L - @            \ W[t-3] R:W[t]
          R@     [  8 CELLS ]L - @ XOR        \ W[t-3]^W[t-8]
          R@     [ 14 CELLS ]L - @ XOR        \ W'^W[t-14]
          R@     [ 16 CELLS ]L - @ XOR 1 rol  \ (W"^W[t-16]<<1)
          R>  TUCK  !  CELL+                  \ W[t+1] R:-- ;W[t]=Wnew
  LOOP  DROP                                  \ --
;

: F2 ( - n)  \ n = B XOR C XOR D
  H[C] DUP  CELL-  @ ( D)  SWAP  2@ ( B C)  XOR  XOR  \ B^C^D
;

: F1 ( - n)  \ n = (B AND C) OR (~B AND D)
  H[C]  DUP  CELL-  SWAP  2@    \ H[D] B C
  OVER  AND  SWAP  INVERT       \ H[D] (B*C) ~B
  ROT  @  AND  OR               \ (B*C)|(~B*D)
;

: F3 ( - n)  \ n = (B AND C) OR (B AND D) OR (C AND D)
  H[C]  DUP  2@  OVER  >R  AND  \ H[C]  (B*C)           R:B
  SWAP  CELL-  2@  DUP >R  AND  \ (B*C) (C*D)           R:B D
  2R>  AND  OR  OR              \ (B*C)|(C*D)|(B*D)     R:--
;

: HashAdjust ( A' -)  \ Adjust hash array for next round
  H[B]  DUP  @  30 rol  SWAP  ! \ A'     ;B = S(B,30) = rol(B,30)
  H[E]  CELL+  DUP  TO  H[E]    \ A' [E] ;H[E] points to former H[D]
  [ 4 CELLS ]L  +  !            \ --     ;A in new top of hash address
;

: rnds1 ( [Wi] -  [Wi]')  \ Perform hash algorithm for rounds  0 - 19
  20 0 DO  DUP  @                       \ [Wi] Wi
           H[E]  DUP  @                 \ [Wi] Wi (E) E
           SWAP  [ 4 CELLS ]L  +  @     \ [Wi] Wi E A
           5 rol  +  +  K1  +  F1  +    \ [Wi] Wi+E+S(A,5)+K1+F1
           HashAdjust                   \ [Wi]
           CELL+                        \ [Wi]'
  LOOP
;

: rnds2 ( [Wi] -  [Wi]')  \ Perform hash algorithm for rounds 20 - 39
  20 0 DO  DUP  @                       \ [Wi] Wi
           H[E]  DUP  @                 \ [Wi] Wi (E) E
           SWAP  [ 4 CELLS ]L  +  @     \ [Wi] Wi E A
           5 rol  +  +  K2  +  F2  +    \ [Wi] Wi+E+S(A,5)+K2+F2
           HashAdjust                   \ [Wi]
           CELL+                        \ [Wi]'
  LOOP
;

: rnds3 ( [Wi] -  [Wi]')  \ Perform hash algorithm for rounds 40 - 59
  20 0 DO  DUP  @                       \ [Wi] Wi
           H[E]  DUP  @                 \ [Wi] Wi (E) E
           SWAP  [ 4 CELLS ]L  +  @     \ [Wi] Wi E A
           5 rol  +  +  K3  +  F3  +    \ [Wi] Wi+E+S(A,5)+K3+F3
           HashAdjust                   \ [Wi]
           CELL+                        \ [Wi]'
  LOOP
;

: rnds4 ( [Wi] -  [Wi]')  \ Perform hash algorithm for rounds 60 - 79
  20 0 DO  DUP  @                       \ [Wi] Wi
           H[E]  DUP  @                 \ [Wi] Wi (E) E
           SWAP  [ 4 CELLS ]L  +  @     \ [Wi] Wi E A
           5 rol  +  +  K4  +  F2  +    \ [Wi] Wi+E+S(A,5)+K4+F2
           HashAdjust                   \ [Wi]
           CELL+                        \ [Wi]'
  LOOP
;

: SHA1 ( -)  \ Create 160-bit SHA-1 hash of 512-bit message in W array
  Wexpand                         \ Expand message currently in W array
  W rnds1 rnds2 rnds3 rnds4 DROP  \ Do SHA-1 alg, create H[A]--H[E]
  UpDateHash                      \ Compute and update hash values
;

: InitCount  ( -)  0  0  MCount  2!  ;  \ Set message bit count to zero

: CountUpDate ( d -)  MCount  2@  D+  MCount  2!  ;

: CountStore  ( -)  MCount  2@  [ W  14 CELLS  + ]L  2!  ;

\ Display SHA-1 hash value in hex ( A B C D E)
: HASH. CR  HEX
  [ SHAval 4 CELLS + ]L  5 0 DO  DUP  @  U.  CELL-  LOOP  DROP  DECIMAL

;

\ Load W array with data on stack
: WLoad ( d0..d15 - ) [ W 15 CELLS + ]L 16 0 DO TUCK ! CELL- LOOP DROP ;

\ ---------------------------------------------------------------------
\ EXAMPLE 1: from FIPS PUB 180-1
\ Message: ASCII string 'abc'
\ Hash = A9993E36  4706816A  BA3E2571  7850C26C  9CD0D89D

\ Load message block with ASCII string 'abc' (616263h)
: EX1a  [ HEX ]  61626380 0 0 0 0 0 0 0 0 0 0 0 0 0 0 18  WLoad ;

\ Compute/display hash for ASCII string 'abc'
: EX1  InitHash  EX1a  SHA1  HASH.  ;
\ ---------------------------------------------------------------------
\ EXAMPLE 2: from FIPS PUB 180-1
\ Message: 'abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq'
\ Hash = 84983E44  1C3BD26E  BAAE4AA1  F95129E5  E54670F1

: EX2a \ Load 'abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq'
  [ HEX ]   61626364  62636465  63646566  64656667  65666768  66676869
  6768696A  68696A6B  696A6B6C  6A6B6C6D  6B6C6D6E  6C6D6E6F  6D6E6F70
  6E6F7071  80000000  0  WLoad
;

: EX2b  \ Load second block for message string
  [ HEX ]  0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1C0  WLoad
;

\ Do hash for 'abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq'
: EX2   InitHash  EX2a  SHA1  EX2b  SHA1  HASH.  ;
\ ---------------------------------------------------------------------
\ EXAMPLE 3: from FIPS PUB 180-1
\ Message: String of 1 million copies of 'a' (61h), (8 million bits)
\ Hash = 34AA973C  D4C4DAA4  F61EEB2B  DBAD2731  6534016F

: EX3a \ Load message block with all 'a' (61h), must hash 15,625 times
  [ HEX ] 61616161 ( 'aaaa') [ DECIMAL ] W 16 0 DO 2DUP ! CELL+ LOOP
  2DROP
;

: EX3b  \ Load last block: first bit to '1' and bit-count to 8 million
  [ HEX ]  80000000 0 0 0 0 0 0 0 0 0 0 0 0 0 0 7A1200  WLoad
;

\ Compute/display hash for 1 million copies of ASCII 'a' (61h)
: EX3 CR ." Hashing 'a' 1,000,000 times, go get a beer..."   
  [ DECIMAL ] InitHash  EX3a  15625  0 DO  SHA1  LOOP
  EX3b  SHA1  HASH.
;

CR .( --- TESTING SECURE HASH ALGORITHM SHA-1 --- )
EX1
EX2
TIC EX3TOC
CR .( This took ) .ELAPSED

