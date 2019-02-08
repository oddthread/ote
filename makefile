ote:
	gcc -ggdb -mwindows -Wno-empty-body -Wno-unused-variable -Wno-unused-function -std=c90 -I../ ../oul/src/c/*.c ../ovp/src/c/*.c ../opl/src/c/*.c ../osg/src/c/*.c src/c/*.c -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf -lSDL2_net -lm -o bin/ote
