cl -I../ src/c/*.c /W0 /TC /Z7 /Fewindows_bin/make_vc.exe ../osg/src/c/*.c ../opl/src/c/*.c ../oul/src/c/*.c -link windows_bin/*.lib opengl32.lib

rm *.obj
