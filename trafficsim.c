#include <sys/types.h>
#include <linux/unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdio.h>

#define BUFF_SIZE 1000

struct cs1550_sem {
	int value;
	struct Node *head;
	struct Node *tail;
};

/* Wrapper Function for Down Syscall */
void down(struct cs1550_sem *sem) {
       syscall(__NR_cs1550_down, sem);
}

/* Wrapper Function for Up Syscall */
void up(struct cs1550_sem *sem) {
	syscall(__NR_cs1550_up, sem);
}


int main(int argc, char *argv[]){

	/* Declaration of semaphores */
	struct cs1550_sem * empty;
	struct cs1550_sem * full;
	struct cs1550_sem * mutex;

	//Allocate space for our semaphores. Multipled by 3 because we have 3 semaphores so we need 3 chunks
	void *sem_ptr = mmap(NULL, sizeof(struct cs1550_sem)*3, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0); 

	/* Allocating chunks for semaphores */
	empty = (struct cs1550_sem*)sem_ptr;	//Sets empty sem to shared space
	full = (struct cs1550_sem*)sem_ptr+1;	//Sets full sem to shared space offset 1
	mutex = (struct cs1550_sem*)sem_ptr+2;	//Sets mutex sem to shared space offset 2

	/*Initialization of semaphores */
	empty->value = BUFF_SIZE;
	full->value = 0;
	mutex->value = 1;

	empty->head = NULL;
	empty->tail = NULL;

	full->head = NULL;
	full->tail = NULL;

	mutex->head = NULL;
	mutex->tail = NULL;

	//Allocate space for buffer size because we need buffsize_ptr 
	void *mem = mmap(NULL, sizeof(int) * BUFF_SIZE+1, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);

	int *buffer, *producer, *consumer, *buffsize_ptr;

	buffsize_ptr = (int*)mem;	//Contains n
	producer = (int*)mem+1;		//Contains producer index
	consumer = (int*)mem+2;		//Contains consumer index
	buffer = (int*)mem+3;	//Shared array

	*buffsize_ptr = BUFF_SIZE;	//Initialize buffer size
	*producer = 0;	//Set index of producer
	*consumer = 0;	//Set index of consumer

	int num_producers = 2;	//North and South producers
	int num_consumers = 1;	//Flag person

	int i;
	int j;
	char direction;

	int counter = 0;


	/* Producers */
	for(i = 0; i < num_producers; i++){	//Create threads for producers
		if(fork() == 0) {
			int pitem; //Producer item
			printf("\n");
			while(counter < 30){

				srand(time(NULL));		//Seed random num gen
				int r = rand() % 10;	//Random number from 0-9

				down(empty);
				down(mutex); //Grab the lock

				 if(r < 8){
				 	
					
				 	pitem = *producer;
				 	buffer[*producer] = pitem;

				 	if(i == 0){
				 		direction = 'N';
				 	} else {
				 		direction = 'S';
				 	}

				 	printk("Car Number %d going %c \n", pitem, direction); //add 65 for ascii offset
					*producer = (*producer + 1) % *buffsize_ptr;

					counter++;
				 } else {
				 	printf("No cars. Flagperson has fallen asleep\n");
				 	sleep(20);
				 	printf("Flagperson has woken up\n");
				 }


				up(mutex); //Release the lock
				up(full);


			}
		}
	}

	/* Consumers */
	 for(i = 0; i < num_consumers; i++){
	 	if(fork() == 0){
			int citem; //Consumer item

	 		while(counter < 30){
	 			down(full);
	 			down(mutex); //Grab the lock

	 			if(i == 0){
	 				direction = 'N';
	 			} else {
					direction = 'S';
	 			}

				citem = buffer[*consumer];
				printk("Flagperson waved through car number %d going %c\n", citem, direction);
				sleep(2);	//Time to cross road
	 			*consumer = (*consumer + 1) % *buffsize_ptr;

	 			up(mutex);	//Release lock
	 			up(empty); 
	 			// sleep(3);
	 			counter++;
	 		}
	 	}
	 }



	return 0;
}