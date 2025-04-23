all:
	@g++ -c main.cpp -std=c++17 -m64 -g -Wall -I bengine
	@g++ main.o -o main.out -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf

run:
	@g++ -c main.cpp -std=c++17 -m64 -g -Wall -I bengine
	@g++ main.o -o main.out -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf
	@./main.out
