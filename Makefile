all: project2.o student2.o
	gcc project2.o student2.o -o p2 -Wall -Wextra

project2.o: project2.c
	gcc -c project2.c -o project2.o

student2.o: student2.c
	gcc -c student2.c -o student2.o

clean: project2.o student2.o p2
	rm -f project2.o student2.o p2