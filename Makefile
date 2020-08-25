CXX = g++
FLAGS = --std=c++11 -O2
# FLAGS = --std=c++11 -O2 -fsanitize=address

PROGRAMS = bin/gambler
UTILS_HEADER = utils/game.h utils/rng.h utils/net.h utils/evalHandTables utils/rank_hand.h
UTILS_SRC = utils/net.cpp utils/rng.cpp utils/game.cpp utils/rank_hand.cpp
UTILS_OBJ = utils/net.o utils/rng.o utils/game.o utils/rank_hand.o

all: $(PROGRAMS)

clean:
	rm -f $(PROGRAMS) $(UTILS_OBJ)

clear:
	rm -f ./logs/*

utils/net.o: $(UTILS_SRC) $(UTILS_HEADER)
utils/rng.o: $(UTILS_SRC) $(UTILS_HEADER)
utils/game.o: $(UTILS_SRC) $(UTILS_HEADER)
utils/rank_hand.o: $(UTILS_SRC) $(UTILS_HEADER)

bin/gambler: gambler.cpp $(UTILS_OBJ)
	$(CXX) $(FLAGS) -o $@ gambler.cpp $(UTILS_OBJ)

