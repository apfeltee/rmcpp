
##
# todo: cross-building for DOS? do people still use DOS?
# linux, maybe. if i can get the compiler to, y'know, compile.
# mac, probably not. don't even know if that's possible.
##


srcfiles = main.cpp lib.cpp
# the main one, for prototyping, debugging, etc
outfile_gcc   = rmcpp.exe
# these are for testing, mostly.
outfile_clang = rmcppclang.exe
outfile_msc   = rmcppvs.exe
outfile_clr   = rmcppclr.exe


cxx_gcc   = g++ -std=c++17
cxx_clang = clang++ -std=c++17
cxx_msc   = cl -std:c++17

# just build gcc by default, please.
default: buildgcc postclean
buildall: buildgcc buildclang buildmsc buildclr postclean

postclean:
	rm -f *.obj *.pdb *.mdb *.ilk

clean: postclean
	rm -f *.exe

buildgcc: $(srcfiles)
	$(cxx_gcc) -Wall -Wextra -g3 -ggdb3 $(srcfiles) -o $(outfile_gcc)

# don't use 
buildclang: $(srcfilse)
	$(cxx_clang) -Wall -Wextra $(srcfiles) -o $(outfile_clang)


# according to cl options, it's actually -Fe<str>
# but this is apparently the right way? wtf?
buildmsc: $(srcfiles)
	$(cxx_msc) $(srcfiles) -link -out:$(outfile_msc)


buildclr: $(srcfiles)
	$(cxx_msc) -clr:pure $(srcfiles) -link -out:$(outfile_clr)
