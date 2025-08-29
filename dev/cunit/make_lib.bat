tcc -c -m64	cunit/Automated.c	 -o	Automated.o
tcc -c -m64	cunit/Basic.c	 -o	Basic.o
tcc -c -m64	cunit/Console.c	 -o	Console.o
tcc -c -m64	cunit/CUError.c	 -o	CUError.o
tcc -c -m64	cunit/MyMem.c	 -o	MyMem.o
tcc -c -m64	cunit/TestDB.c	 -o	TestDB.o
tcc -c -m64	cunit/TestRun.c	 -o	TestRun.o
tcc -c -m64	cunit/test_cunit.c	 -o	test_cunit.o
tcc -c -m64	cunit/Util.c	 -o	Util.o
tcc -c -m64	cunit/Win.c	 -o	Win.o
tcc -ar rcs libcunit.a Basic.o Console.o CUError.o MyMem.o TestDB.o TestRun.o test_cunit.o Util.o Win.o Automated.o 
