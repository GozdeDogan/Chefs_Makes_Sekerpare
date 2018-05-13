#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>          /* errno, ECHILD */


//#define DEBUG
#define SHMSIZ 1024
#define SIZE 20
#define NUM_OF_CHEFS 6
#define SHARED_MEMORY_SIZE 4096
#define SEMAPHORE "semaphore"

/*sahip olunan ilk urun, ikinci urun, sahip olan chef*/
int stuffArr[NUM_OF_CHEFS][3] = {{0, 1, -1}, {0, 2, -1}, {0, 3, -1}, {1, 2, -1}, {1, 3, -1}, {2, 3, -1}};


typedef enum {EGGS=0, FLOUR=1, BUTTER=2, SUGAR=3} Stuff;

typedef struct
{
	Stuff stuff1;
	Stuff stuff2;
	int fromChef; // hangi chef'ten urun alacagi
	char sharedMemoryName[SIZE];
	int chefID;
}Chefs;



int numOfChefs=0;
Chefs *chefs;
sem_t sem;
pid_t parentPID;
pid_t childPIDs[NUM_OF_CHEFS];

struct sigaction sa;			/* SIGINT ve SIGTERM sinyali icin tutulan struct */


/******************************** FUNCTION DEFINITIONS ********************************/
void Chefs_Makes_Sekerpare();
void printSharedMemory(int i);
void printChefsToSharedMemory();
void readSharedMemory(int i, Chefs *read_chef);
void usage();
void giveStuffToChefs();
void printChefs();
void printChefsHas();
void printChefhas(int i);
void printChefWaiting(int i, int take[2]);
void has(int i);
void errExit(char msg[SIZE]);
void signalHandler(int sig);
/**************************************************************************************/


int main(int argc, char *argv[]){
	if(argc == 1)
		numOfChefs = NUM_OF_CHEFS;
	else if (argc == 2){
		if(atoi(argv[1]) <= NUM_OF_CHEFS && atoi(argv[1]) >= 2)
			numOfChefs = atoi(argv[1]);
		else{
			usage();
			fprintf(stderr, "\tnumOfChefs <= 6 && numOfChefs >= 2\n");
			fprintf(stderr, "----------------------------------------------------\n");
			return 0;
		}
	}
	else{
		usage();
		return 0;
	}

	#ifdef DEBUG
		fprintf(stderr, "numOfChefs: %d\n", numOfChefs);
	#endif

	#ifdef DEBUG
		fprintf(stderr, "BEFORE:\n");
		for (int i = 0; i < numOfChefs; ++i)
		{
			fprintf(stderr, "stuffArr[%d][0]:%d \t stuffArr[%d][1]:%d \t stuffArr[%d][2]:%d \n", i, stuffArr[i][0], i, stuffArr[i][1], i, stuffArr[i][2]);
		}
	#endif

	Chefs_Makes_Sekerpare();

	#ifdef DEBUG
		fprintf(stderr, "AFTER:\n");
		for (int i = 0; i < numOfChefs; ++i)
		{
			fprintf(stderr, "stuffArr[%d][0]:%d \t stuffArr[%d][1]:%d \t stuffArr[%d][2]:%d \n", i, stuffArr[i][0], i, stuffArr[i][1], i, stuffArr[i][2]);
		}
	#endif

	return 0;
}

void Chefs_Makes_Sekerpare(){
	//pthread_t threads[numOfChefs];
	int i;
	chefs = (Chefs*)calloc(numOfChefs, sizeof(Chefs));
	giveStuffToChefs();

	#ifdef DEBUG
		fprintf(stderr, "\n\nStuffs of Chefs:\n");
		printChefs();
	#endif

	//sem = sem_open (SEMAPHORE, O_CREAT | O_EXCL, 0644, 1); 
	if (sem_init(&sem, 0, 1) == -1)
		errExit("sem_init");

	//hepsi icin shared memory olusturup verilerini yaz!
	printChefsToSharedMemory();
	printChefsHas();

	/**************************** SINYAL HAZIRLIGI ****************************/
	sa.sa_handler = signalHandler;
    sa.sa_flags = SA_RESTART;
    

    if (sigemptyset(&sa.sa_mask) == -1){
		perror("\tFailed to initialize the signal mask");
		exit(EXIT_FAILURE);
	}

	// SIGINT ve SIGTERM sinyalleri eklendi.
	if(sigaddset(&sa.sa_mask, SIGINT) == -1){
		perror("\tFailed to initialize the signal mask");
		exit(EXIT_FAILURE);
	}
	/****************************************************************************/

	parentPID = getpid();

	while(1){
		if(sigaction(SIGINT, &sa, NULL) == -1){
   			perror("\tCan't SIGINT");
   			//exit(EXIT_FAILURE);
   		}
		for(i = 0; i < numOfChefs; i++){
			//fprintf(stderr, "i : %d\n", i);
			if(getpid() == parentPID){
				childPIDs[i] = fork();
				if(childPIDs[i] == -1){
					//sem_unlink (SEMAPHORE);   
                    sem_close(&sem);  
					errExit("fork");
				}
				else if(childPIDs[i] == 0){
					//fprintf(stderr, "child\n");
					break;	
				}
			}
			//fprintf(stderr, "END\n");
		}
		/******************************************************/
	    /******************   PARENT PROCESS   ****************/
	    /******************************************************/
	    if (childPIDs[i] != 0){
	        /* wait for all children to exit */
	        while (childPIDs[i] = waitpid (-1, NULL, 0)){
	            if (errno == ECHILD)
	                break;
	        }

	        /* cleanup semaphores */
	        //sem_unlink (SEMAPHORE);   
	        sem_close(&sem);  

	        //i = 0;
	        exit (0);
	        //wait(NULL);
	        //continue;
	    }

	    /******************************************************/
	    /******************   CHILD PROCESS   *****************/
	    /******************************************************/
	    else{
	    	//fprintf(stderr, "IN CHILD\n");
	        if (sem_wait(&sem) == -1)
				errExit("sem_wait");

			//operation
			#ifdef DEBUG
				fprintf(stderr, "parent_pid: %d\n", getppid());
				fprintf(stderr, "chef%d\n", i+1);
			#endif

			int take[2];
			printChefWaiting(i, take);
			//fprintf(stderr, "take[0]: %d \t take[1]: %d\n", take[0], take[1]);

			Chefs read_chef1, read_chef2; 
			for(int j = 0; j < numOfChefs; j++){
				if(chefs[j].stuff1 == take[0] || chefs[j].stuff1 == take[1]){
					
					read_chef1 = chefs[j];
					/*fprintf(stderr, "read_chef1: %d\t", read_chef1.chefID);
					fprintf(stderr, "stuff1: %d; stuff2: %d\n", read_chef1.stuff1, read_chef1.stuff2);*/
				}
			}


			for(int j = 0; j < numOfChefs; j++){
				if(chefs[j].stuff2 == take[0] || chefs[j].stuff2 == take[1]){		
					read_chef2 = chefs[j];
					/*fprintf(stderr, "read_chef2: %d\t", read_chef2.chefID);
					fprintf(stderr, "stuff1: %d; stuff2: %d\n", read_chef2.stuff1, read_chef2.stuff2, read_chef2);*/
				}
			}
			/*Chefs read_chef;
			readSharedMemory(chefs[i].fromChef, &read_chef);

			fprintf(stderr, "read_chef: %d\n", read_chef.chefID);
			fprintf(stderr, "stuff1: %d; stuff2: %d; fromChef: %d;\n", read_chef.stuff1, read_chef.stuff2, read_chef.fromChef);
			fprintf(stderr, "sharedMemoryName: %s\n", read_chef.sharedMemoryName);*/

			#ifdef DEBUG
				fprintf(stderr, "read_chef1: %d\n", read_chef1.chefID);
				fprintf(stderr, "stuff1: %d; stuff2: %d; fromChef: %d;\n", read_chef1.stuff1, read_chef1.stuff2, read_chef1.fromChef);
				fprintf(stderr, "sharedMemoryName: %s\n", read_chef1.sharedMemoryName);

				fprintf(stderr, "read_chef2: %d\n", read_chef2.chefID);
				fprintf(stderr, "stuff1: %d; stuff2: %d; fromChef: %d;\n", read_chef2.stuff1, read_chef2.stuff2, read_chef2.fromChef);
				fprintf(stderr, "sharedMemoryName: %s\n", read_chef2.sharedMemoryName);
			#endif

			if (sem_post(&sem) == -1)
				errExit("sem_post");


			_exit(EXIT_SUCCESS); 
	    }

	}

	

    if (sem_destroy(&sem) == -1)
		errExit("sem_destroy");

	free(chefs);
}

void printChefsToSharedMemory(){
	for (int i = 0; i < numOfChefs; ++i)
	{
	    printSharedMemory(i);
	}
}

void printSharedMemory(int i){
	/* shared memory file descriptor */
    int shm_fd;
 
    /* pointer to shared memory obect */
    void* ptr;
 
    /* create the shared memory object */
    shm_fd = shm_open(chefs[i].sharedMemoryName, O_CREAT | O_RDWR, 0666);
 
    /* configure the size of the shared memory object */
    ftruncate(shm_fd, SHARED_MEMORY_SIZE);
 
    /* memory map the shared memory object */
    ptr = mmap(0, SIZE, PROT_WRITE, MAP_SHARED, shm_fd, 0);
 
    /* write to the shared memory object */
    ptr = &chefs[i];
    ptr += sizeof(chefs[i]);
}



void readSharedMemory(int i, Chefs *read_chef){
 
    /* shared memory file descriptor */
    int shm_fd;
 
    /* pointer to shared memory object */
    void* ptr;
 
    /* open the shared memory object */
    shm_fd = shm_open(chefs[i].sharedMemoryName, O_RDONLY, 0666);
 
    /* memory map the shared memory object */
    ptr = mmap(0, SHARED_MEMORY_SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);
 
    /* read from the shared memory object */
    read_chef = (Chefs *)ptr;
 	
 	#ifdef DEBUG
	    fprintf(stderr, "\n\nread_chef: %d\n", read_chef->chefID);
		fprintf(stderr, "stuff1: %d; stuff2: %d; fromChef: %d;\n", read_chef->stuff1, read_chef->stuff2, read_chef->fromChef);
		fprintf(stderr, "sharedMemoryName: %s\n\n\n", read_chef->sharedMemoryName);
	#endif
    /* remove the shared memory object */
    shm_unlink(chefs[i].sharedMemoryName);
}



void usage(){
	fprintf(stderr, "----------------------------------------------------\n");
	fprintf(stderr, "\t./Chefs_Makes_Sekerpare #numOfChefs\n");
	fprintf(stderr, "\t\tOR\n");
	fprintf(stderr, "\t./Chefs_Makes_Sekerpare\n");
	fprintf(stderr, "----------------------------------------------------\n");
}

void giveStuffToChefs(){

	chefs[0].stuff1 = stuffArr[0][0];
	chefs[0].stuff2 = stuffArr[0][1];
	chefs[0].chefID = 0;
	chefs[0].fromChef = 5;
	sprintf(chefs[0].sharedMemoryName, "chef0_sharedmemory");
	stuffArr[0][2] = 0;

	chefs[1].stuff1 = stuffArr[5][0];
	chefs[1].stuff2 = stuffArr[5][1];
	chefs[1].chefID = 1;
	chefs[1].fromChef = 0;
	sprintf(chefs[1].sharedMemoryName, "chef1_sharedmemory");
	stuffArr[5][2] = 1;

	int i = 2; 
	while(i < numOfChefs)
	{
		int choice = -1;
		do{
			//srand(time(NULL));
			choice = rand()%5 + 1;
		}while(choice < 1 || choice >= 5);

		if(stuffArr[choice][2] == -1){
			stuffArr[choice][2] = i;
			chefs[i].stuff1 = stuffArr[choice][0];
			chefs[i].stuff2 = stuffArr[choice][1];
			chefs[i].chefID = i;
			chefs[i].fromChef = choice;
			sprintf(chefs[i].sharedMemoryName, "chef%d_sharedmemory", i);
			i++;
		} 
	}
	#ifdef DEBUG
		fprintf(stderr, "Stuffs of Chefs:\n");
		printChefs();
	#endif
}

void printChefs(){
	for (int i = 0; i < numOfChefs; ++i)
	{
		fprintf(stderr, "Cheff %d;\n", chefs[i].chefID); 
		fprintf(stderr, "stuff1: %d; stuff2: %d; fromChef: %d;\n", chefs[i].stuff1, chefs[i].stuff2, chefs[i].fromChef);
		fprintf(stderr, "sharedMemoryName: %s\n", chefs[i].sharedMemoryName);
	}
}

void printChefsHas(){
	fprintf(stderr, "\n\n-------------------------------------------------------------------------\n");
	fprintf(stderr, "Stuffs belonging to the chefs >>\n\n");
	for (int i = 0; i < numOfChefs; ++i)
	{
		printChefhas(i);
	}
	fprintf(stderr, "-------------------------------------------------------------------------\n\n");
}

void printChefhas(int i){
	fprintf(stderr, "chef%d has an endless supply of ", i+1);
	has(chefs[i].stuff1);
	fprintf(stderr, " and ");
	has(chefs[i].stuff2);
	fprintf(stderr, " but lacks ");

	int arr[4] = {0, 1, 2, 3};
	int res = 0;
	for(int j=0; j<4; j++){
		if(j != chefs[i].stuff1 && j != chefs[i].stuff2){
			has(j);
			if(res == 0)
				fprintf(stderr, " and ");
			res++;
		}
	}
	fprintf(stderr, ".\n");

}

void has(int i){
	switch(i){
		case 0:
			fprintf(stderr, "eggs");
			break;
		case 1:
			fprintf(stderr, "flour");
			break;
		case 2:
			fprintf(stderr, "butter");
			break;
		case 3:
			fprintf(stderr, "sugar");
			break;
		default:
			fprintf(stderr, "nothing\n");
	}
}

void printChefWaiting(int i, int take[2]){
	fprintf(stderr, "chef%d waiting for ", i+1);
	int arr[4] = {0, 1, 2, 3};
	int res = 0;
	int take_index = 0;
	for(int j=0; j<4; j++){
		if(j != chefs[i].stuff1 && j != chefs[i].stuff2){
			has(j);
			if(take_index < 2){
				take[take_index] = j;
				take_index++;
			}
			if(res == 0)
				fprintf(stderr, " and ");
			res++;
		}
	}
	fprintf(stderr, ".\n");
}

void errExit(char msg[SIZE]){
	fprintf(stderr, "%s couldn't execute\n", msg);
	exit(EXIT_FAILURE);
}

void signalHandler(int sig){
	if(sig == SIGINT){
		fprintf(stdout, "\n\n\tCTRL-C SIGNAL CAME\n");
		exit(EXIT_SUCCESS);
	}
}