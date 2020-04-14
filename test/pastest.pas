
program foo;
(* blah

*)

var 
    nuts(* should be gone *): integer = 12345(* should be gone *);
// a c++-style comment (* don't matter *)

    quork: string = (*this is how you declare a single ' ... *)''(*...*)''(* yeah, seriously. *);

    blah: string = (* should be gone *)'hurp de(* not a comment *) durp';
{ this (* shouldn't be a *) problem }
    honk: string = 'foop';

(* a pas comment ... { and braced comment ... } blah blah *)

(*
    lets (*
        go (*
            really (*
                deep (*
                    even (*
                        though (*
                            most (*
                                compilers (*
                                    will (*
                                        refuse (*
                                            this
                                            
                                            (*(*(*(*(*(*(**)
                                                (*(*(*(*(*(*(**)
                                                    (*(*(*(*(*(*(*(*
                                                    (*(*(*(* seriously who ever ok'd this shit? *)*)*)*)
                                                    *)*)*)*)*)*)*)*)
                                                *)*)*)*)*)*)
                                            *)*)*)*)*)*)
                                        *) durka durka
                                    *) akrud akrud
                                *) rudka rudka
                            *) darku darku
                        *) urkad urkad
                    *) adkur adkur
                *) rudak rudak
            *) kadur kadur
        *) raduk raduk
    *) rukad rukad
*)

begin
    writeln((* pascal comment *)'hello world');
    (* this (* is a nested *) comment that should be handled correctly *)
    writeln('goodbye world'(* should be gone*));
end.

(* end of file *)


