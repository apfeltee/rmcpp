
program foo;
(* blah *)

var foo: integer = 12345(* should be gone *);
// a c++-style comment (* don't matter *)


var blah: string = (* should be gone *)'hurp de(* not a comment *) durp';
{ this (* shouldn't be a *) problem }
var honk: string = 'foop';

(* a pas comment ... { and braced comment ... } blah blah *)


begin
    writeln((* pascal comment *)'hello world');
    (* this (* is a nested *) comment that should be handled correctly *)
    writeln('goodbye world'(* should be gone*));
end.

(* end of file *)


