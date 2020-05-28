#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>


#define CHECK_ERR(a,msg) {if ((a) == -1) { perror((msg)); exit(EXIT_FAILURE); } }
#define CHECK_ERR_MMAP(a,msg) {if ((a) == MAP_FAILED) { perror((msg)); exit(EXIT_FAILURE); } }

#define NUMBER_OF_CYCLES 10
sem_t * process_semaphore;
int * shared_counter;

int main(int argc, char * argv[]) {
	int res;
	pid_t pid;

	process_semaphore = mmap(NULL, // NULL: è il kernel a scegliere l'indirizzo
			sizeof(sem_t) + sizeof(int), // dimensione della memory map
			PROT_READ | PROT_WRITE, // memory map leggibile e scrivibile
			MAP_SHARED | MAP_ANONYMOUS, // memory map condivisibile con altri processi e senza file di appoggio
			-1,
			0); // offset nel file
	CHECK_ERR_MMAP(process_semaphore,"mmap")

	shared_counter = (int *) (process_semaphore + 1);

	printf("initial value of shared_counter=%d, NUMBER_OF_CYCLES=%d\n", *shared_counter, NUMBER_OF_CYCLES);

	res = sem_init(process_semaphore,
					1, // 1 => il semaforo è condiviso tra processi, 0 => il semaforo è condiviso tra threads del processo
					1 // valore iniziale del semaforo (se mettiamo 0 che succede?)
				  );
	CHECK_ERR(res,"sem_init")


	pid = fork();
	CHECK_ERR(pid,"fork")

	if (pid == 0) {
		for (int i = 0; i < NUMBER_OF_CYCLES; i++) {
			if (sem_wait(process_semaphore) == -1) {
				perror("sem_wait");
				exit(EXIT_FAILURE);
			}

			(*shared_counter)++;
      printf("+ Counter: %d\n", *shared_counter);

			if (sem_post(process_semaphore) == -1) {
				perror("sem_post");
				exit(EXIT_FAILURE);
			}
		}
		exit(EXIT_SUCCESS);
	} 
  
  else {
		for (int i = 0; i < NUMBER_OF_CYCLES; i++) {
			if (sem_wait(process_semaphore) == -1) {
				perror("sem_wait");
				exit(EXIT_FAILURE);
			}

			(*shared_counter)--;
      printf("- Counter: %d\n", *shared_counter);

			if (sem_post(process_semaphore) == -1) {
				perror("sem_post");
				exit(EXIT_FAILURE);
			}
		}
	}
  

	while(wait(NULL) != -1);

	res = sem_destroy(process_semaphore);
	CHECK_ERR(res,"sem_destroy")

	return 0;
}