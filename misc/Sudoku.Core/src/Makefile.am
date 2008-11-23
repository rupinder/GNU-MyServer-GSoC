## Makefile.am -- Process this file with automake to produce Makefile.in

generated = 

#sources = Configuration.cs Debug.cs SimpleSolver.cs SimpleBuilder.cs Program.cs
#
#references = 
#
#resources = 
#
#sudoku.exe : $(generated) $(sources)
#	$(MCS) $(MCSFLAGS) -out:$@ $(references) $(resources) $(generated) $(sources)
#
#all : sudoku.exe
#	mv sudoku.exe ../bin
#	mv sudoku.exe.mdb ../bin
#
#CLEANFILES = sudoku.exe ../bin/sudoku.exe ../bin/sudoku.exe.mdb

sources = Configuration.cs Debug.cs SimpleSolver.cs SimpleBuilder.cs Program.cs

sudoku.dll : $(generated) $(sources)
	$(MCS) $(MCSFLAGS) -out:$@ $(references) $(resources) $(generated) $(sources) -target:library

all : sudoku.dll
	mv sudoku.dll ../bin

CLEANFILES = ../bin/sudoku.dll

DISTCLEANFILES = Makefile
