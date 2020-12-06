all : main

main : second.c second.h
	gcc -Wall -Werror -fsanitize=address -std=c11 second.c -o second -lm
clean :
	rm second