all: Chefs_Makes_Sekerpare

Chefs_Makes_Sekerpare: 131044019_main.o
	gcc 131044019_main.o -o Chefs_Makes_Sekerpare -lm -lpthread -lrt

131044019_main.o: 131044019_main.c
	gcc -c 131044019_main.c

clean:
	rm *.o Chefs_Makes_Sekerpare