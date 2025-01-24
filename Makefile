# Copyright (c) 2025, Bertrand Mollinier Toublet
# See LICENSE for details of BSD 3-Clause License
CPPFLAGS=-std=c++2a -Wall -O0 -g -Wsign-compare -Werror -Werror=return-type # -g
CC=$(CXX)
LDLIBS=

src = coord.cpp \
	  cell.cpp \
	  row.cpp \
	  column.cpp \
	  nonet.cpp \
	  board.cpp

sudoku-solver: $(src:%.cpp=%.o)

.PHONY: depend
depend:
	makedepend -- $(CPPFLAGS) -- sudoku-solver.cpp $(src)


.PHONY: clean
clean:
	rm -f $(src:%.cpp=%.o) sudoku-solver.o

# DO NOT DELETE

sudoku-solver.o: board.h cell.h coord.h row.h column.h nonet.h
coord.o: coord.h
cell.o: cell.h coord.h
row.o: row.h board.h cell.h coord.h column.h nonet.h
column.o: column.h board.h cell.h coord.h row.h nonet.h
nonet.o: row.h board.h cell.h coord.h column.h nonet.h
board.o: board.h cell.h coord.h row.h column.h nonet.h
