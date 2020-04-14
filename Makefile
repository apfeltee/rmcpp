
srcfile = main.cpp
# the main one, for prototyping, debugging, etc
outfile_gcc   = remcpp.exe
# these are for testing, mostly.
outfile_clang = remcppclang.exe
outfile_msc   = remcppvs.exe
outfile_clr   = remcppclr.exe


cxx_gcc   = g++ -std=c++17
cxx_clang = clang++ -std=c++17
cxx_msc   = cl -std:c++latest

# just build gcc by default, please.
default: buildgcc postclean
buildall: buildgcc buildclang buildmsc buildclr postclean

postclean:
	rm -f *.obj *.pdb *.mdb *.ilk

buildgcc: $(srcfile)
	$(cxx_gcc) -Wall -Wextra -g3 -ggdb3 $(srcfile) -o $(outfile_gcc)

# don't use 
buildclang: $(srcfile)
	$(cxx_clang) -Wall -Wextra $(srcfile) -o $(outfile_clang)


# according to cl options, it's actually -Fe<str>
# but this is apparently the right way? wtf?
buildmsc: $(srcfile)
	$(cxx_msc) $(srcfile) -link -out:$(outfile_msc)


buildclr: $(srcfile)
	$(cxx_msc) -clr:pure $(srcfile) -link -out:$(outfile_clr)
