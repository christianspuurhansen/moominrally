default: start

start: start.cpp
	g++ -std=c++11 -O3 -o start start.cpp -lSDL -lSDL_gfx -lSDL_image -lSDL_mixer -lSDL_net
