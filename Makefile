DEBUG ?= 1

ifeq ($(DEBUG), 1)
	CFLAGS=-DDEBUG -lpthread -lform -lncurses
else
	CFLAGS=-lpthread -lform -lncurses
endif



file_carver : main.o cmd_options.o logger.o screenObj.o
			  g++ -std=c++17 -o file_carver main.o cmd_options.o logger.o screenObj.o $(CFLAGS)

main.o : src/main.cpp src/main.h
		 g++ -std=c++17 -c src/main.cpp $(CFLAGS)

cmd_options.o : src/cmd_options.cpp src/cmd_options.h src/main.h
				g++ -std=c++17 -c src/cmd_options.cpp $(CFLAGS)

logger.o : src/logger.cpp src/logger.h src/main.h
			g++ -std=c++17 -c src/logger.cpp $(CFLAGS)

screenObj.o : src/screenObj.cpp src/screenObj.h src/main.h
				g++ -std=c++17 -c src/screenObj.cpp $(CFLAGS)

clean :
		rm *.o
