DEBUG ?= 1

ifeq ($(DEBUG), 1)
	CFLAGS=-ggdb -DDEBUG -lpthread -lncurses
else
	CFLAGS=-lpthread -lncurses
endif



file_carver : main.o cmd_options.o logger.o screenObj.o
			  g++ -std=c++17 -o file_carver main.o cmd_options.o logger.o screenObj.o $(CFLAGS)

main.o : main.cpp main.h
		 g++ -std=c++17 -c main.cpp $(CFLAGS)

cmd_options.o : cmd_options.cpp cmd_options.h main.h
				g++ -std=c++17 -c cmd_options.cpp $(CFLAGS)

logger.o : logger.cpp logger.h main.h
			g++ -std=c++17 -c logger.cpp $(CFLAGS)

screenObj.o : screenObj.cpp screenObj.h main.h
				g++ -std=c++17 -c screenObj.cpp $(CFLAGS)

clean :
		rm file_carver main.o logger.o cmd_options.o screenObj.o