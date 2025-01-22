# Copyright (c) 2025, Bertrand Mollinier Toublet
# See LICENSE for details of BSD 3-Clause License
CPPFLAGS=-std=c++2a -Wall -O0 -g -Wsign-compare -Werror -Werror=return-type # -g
CC=$(CXX)
LDLIBS=

src = board.cpp cell.cpp coord.cpp

sudoku-solver: $(src:%.cpp=%.o)

.PHONY: depend
depend:
	makedepend -- $(CPPFLAGS) -- sudoku-solver.cpp $(src)


.PHONY: clean
clean:
	rm -f $(src:%.cpp=%.o)

# DO NOT DELETE

sudoku-solver.o: board.h cell.h coord.h
board.o: board.h cell.h coord.h
cell.o: cell.h coord.h
