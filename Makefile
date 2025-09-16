# Copyright (c) 2025, Bertrand Mollinier Toublet
# See LICENSE for details of BSD 3-Clause License

CPPFLAGS = -std=c++2a -Wall -Wsign-compare -Werror -Werror=return-type # -g
ifeq ($(debug),1)
CPPFLAGS += -O0 -g
else
CPPFLAGS += -O3
endif

CC = $(CXX)
LDLIBS =

src = coord.cpp \
	  cell.cpp \
	  row.cpp \
	  column.cpp \
	  nonet.cpp \
	  board.cpp \
	  analyzer.cpp \
	  analyzer-nakedsingles.cpp \
	  analyzer-hiddensingles.cpp \
	  analyzer-nakedpairs.cpp \
	  analyzer-lockedcandidates.cpp \
	  analyzer-hiddenpairs.cpp \
	  analyzer-xwing.cpp \
	  analyzer-colorchain.cpp \
	  analyzer-ywing.cpp \
	  analyzer-xychain.cpp \
	  solverstate.cpp \
	  solver.cpp

sudoku-solver: $(src:%.cpp=%.o)

.PHONY: depend
depend:
	makedepend -- $(CPPFLAGS) -- sudoku-solver.cpp $(src)


.PHONY: clean
clean:
	rm -f $(src:%.cpp=%.o) sudoku-solver.o

# DO NOT DELETE

sudoku-solver.o: board.h cell.h coord.h row.h column.h nonet.h solverstate.h
sudoku-solver.o: analyzer.h solver.h verbose.h
coord.o: coord.h
cell.o: cell.h coord.h
row.o: row.h board.h cell.h coord.h column.h nonet.h
column.o: column.h board.h cell.h coord.h row.h nonet.h
nonet.o: nonet.h board.h cell.h coord.h row.h column.h
board.o: board.h cell.h coord.h row.h column.h nonet.h verbose.h
analyzer.o: analyzer.h cell.h coord.h board.h row.h column.h nonet.h
analyzer.o: verbose.h
analyzer-nakedsingles.o: analyzer.h cell.h coord.h board.h row.h column.h
analyzer-nakedsingles.o: nonet.h verbose.h
analyzer-hiddensingles.o: analyzer.h cell.h coord.h board.h row.h column.h
analyzer-hiddensingles.o: nonet.h verbose.h
analyzer-nakedpairs.o: analyzer.h cell.h coord.h board.h row.h column.h
analyzer-nakedpairs.o: nonet.h verbose.h
analyzer-lockedcandidates.o: analyzer.h cell.h coord.h board.h row.h column.h
analyzer-lockedcandidates.o: nonet.h verbose.h
analyzer-hiddenpairs.o: analyzer.h cell.h coord.h board.h row.h column.h
analyzer-hiddenpairs.o: nonet.h verbose.h
analyzer-xwing.o: analyzer.h cell.h coord.h board.h row.h column.h nonet.h
analyzer-xwing.o: verbose.h
analyzer-colorchain.o: analyzer.h cell.h coord.h board.h row.h column.h
analyzer-colorchain.o: nonet.h verbose.h
analyzer-ywing.o: analyzer.h cell.h coord.h board.h row.h column.h nonet.h
analyzer-ywing.o: verbose.h
analyzer-xychain.o: analyzer.h cell.h coord.h board.h row.h column.h nonet.h
analyzer-xychain.o: verbose.h
solverstate.o: solverstate.h board.h cell.h coord.h row.h column.h nonet.h
solverstate.o: analyzer.h
solver.o: solver.h solverstate.h board.h cell.h coord.h row.h column.h
solver.o: nonet.h analyzer.h
