CC =g++
CFLAGS = -std=c++11 -lpthread
PROJNAME =testovac.cpp
RESULT =testovac
LOGIN = xheles02
FILES = Makefile testovac.1 manual.pdf testovac.cpp

$(RESULT): $(PROJNAME)
	$(CC) $(CFLAGS) $(PROJNAME) -o $(RESULT)

clean:
	rm -f *~
	rm -f $(RESULT)

tar: clean
	tar -cf $(LOGIN).tar $(FILES)

rmtar:
	rm -f $(LOGIN).tar
