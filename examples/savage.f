( -------------------------------
  Savage floating point benchmark
  ------------------------------- )

REQUIRES float
REQUIRES facility

2500 VALUE MaxLoop

: (SAVAGE)	\ F: <> --- <r>
	1.E
	MaxLoop 1- 0	( Do 2499 times for result of 2500 )
	 ?DO
	    FDUP F* FSQRT 	\ 93 ms, zero error
	    FLN   FEXP
	    FATAN FTAN 		\ most errors are in here !!
	    1.E F+
	LOOP ;


: SAVAGE
	CR ." Testing . . " TIC
	0.E  100 0 DO FDROP (SAVAGE) LOOP FDROP
        TOC .ELAPSED ."  for 100 iterations" ;


\ CR .( Type SAVAGE to test the floating point speed )

CR SAVAGE
