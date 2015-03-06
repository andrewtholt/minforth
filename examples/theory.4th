\ Program by Prof. Ting

: the           ." the " ;
: that          cr ." That " ;
: this          cr ." This is " the ;
: jack          ." that Jack Builds" ;
: summary       ." Summary" ;
: flaw          ." Flaw" ;
: mummery       ." Mummery" ;
: k             ." Constant K" ;
: haze          ." Krudite Verbal Haze" ;
: phrase        ." Turn of a Plausible Phrase" ;
: bluff         ." Chaotic Confusion and Bluff" ;
: stuff         ." Cybernatics and Stuff" ;
: theory        ." Theory " jack ;
: button        ." Button to Start the Machine" ;
: child         ." Space Child with Brow Serene" ;
: cybernatics   ." Cybernatics and Stuff" ;
: hiding        cr ." Hiding " the flaw ;
: lay           that ." Lay in " the theory ;
: based         cr ." Based on " the mummery ;
: saved         that ." Saved " the summary ;
: cloak         cr ." Cloaking " k ;
: thick         if that else cr ." And " then
                ." Thickened " the haze ;
: hung          that ." Hung on " the phrase ;
: cover         if that ." Covered "
                else cr ." To Cover "
                then bluff ;
: make          cr ." To Make with " the cybernatics ;
: pushed        cr ." Who Pushed " button ;
: without       cr ." Without Confusion, Exposing the Bluff" ;

: rest                                  ( pause for user interaction )
        ." . "                          ( print a period )
        10 SPACES                       ( followed by 10 spaces )
        QUERY                           ( wait the user to press a key )
        CR CR ;

: recite                                ( recite the poem )
        cr this theory rest
        this flaw lay rest
        this mummery hiding lay rest
        this summary based hiding lay rest
        this k saved based hiding lay rest
        this haze cloak saved based hiding lay rest
        this bluff hung 1 thick cloak
                saved based hiding lay rest
        this stuff 1 cover hung 0 thick cloak
                saved based hiding lay rest
        this button make 0 cover hung 0 thick cloak
                saved based hiding lay rest
        this child pushed
                cr ." That Made with " cybernatics without hung
                cr ." And, Shredding " the haze cloak
                cr ." Wrecked " the summary based hiding
                cr ." And Demolished " theory rest
        bye ;
recite
