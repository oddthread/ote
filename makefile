all:
	gcc -ggdb -Wno-empty-body -Wno-unused-variable -Wno-unused-function -std=c99 -I../ ../oul/src/c/*.c ../ovp/src/c/*.c ../opl/src/c/*.c ../osg/src/c/*.c src/c/*.c -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf -lSDL2_net -lm -o bin/ote
	
packages:
	sudo apt-get install libsdl2-dev
	sudo apt-get install libsdl2-image-dev
	sudo apt-get install libsdl2-ttf-dev
	sudo apt-get install libsdl2-mixer-dev
	sudo apt-get install libsdl2-net-dev