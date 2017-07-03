cd bin
valgrind --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-fds=yes ./OTE.linux > log.txt 2>&1
cd ..

