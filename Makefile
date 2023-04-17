#Autor: Milan Jakubec
#Login: xjakub41
#Datum: 2023-04-16
#Projekt: 2. projekt do IOS (semafory)

all: proj2

proj2: 
	gcc -std=gnu99 -Wall -Wextra -Werror -pedantic proj2.c -o proj2

run: proj2
	./proj2 3 2 100 100 100

clean:
	rm -f proj2

pack:
	zip xjakub41.zip proj2.c proj2.h Makefile