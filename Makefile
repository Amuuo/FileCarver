file_carver : main.o
			  g++ -std=c++17 -o file_carver main.o -lpthread -lncurses

main.o : main.cpp main.h
		 g++ -std=c++17 -c main.cpp -lpthread -lncurses

clean :
		rm file_carver main.o