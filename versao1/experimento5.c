#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/time.h>
#include <sys/types.h>

#define NO_BARBERS 2
#define NO_WRSEATS 7
#define NO_CLIENTS 20

// Keys para fila de mensagens
#define MESSAGE_QUEUE_BARBER     3102
#define MESSAGE_QUEUE_CUSTOMER   3301

#define SEM_KEY 0x1234

struct sembuf exc_aces_up[1];
struct sembuf exc_aces_down[1];

//Memoria compartilhada
int g_shm_buffer_id;

//Address
int *g_shm_buffer;

//  Incialização das Funções
void barber ();
void customer ();
void cut_hair (char **stringAntes, **stringDepois,  int tamString);
void apreciate_hair (char *stringAntes, *stringDepois, int tamString, double tempoTotal);

int main(){
	// Abrir fila de mensagem
	key_t keyBar = MESSAGE_QUEUE_BARBER;
	key_t keyCus = MESSAGE_QUEUE_CUSTOMER;

	// Abrindo fila para Barbers
	int queue_idBar;
	if(( queue_idBar = msgget(keyBar, IPC_CREAT | 0666)) == -1 ) {
		fprintf(stderr,"Impossivel criar a fila de mensagens!\n");
		exit(1);
	}
	// Abrindo fila para Customers
	int queue_idCus;
	if(( queue_idCus = msgget(keyCus, IPC_CREAT | 0666)) == -1 ) {
		fprintf(stderr,"Impossivel criar a fila de mensagens!\n");
		exit(1);
	}
	//Criando o segmento de memoria compartilhada
	if ((g_shm_buffer_id = shmget(SHM_BUFFER_KEY, sizeof(int), IPC_CREATE | 0000)) == -1) {
		fprintf(stderr, "Impossivel criar o buffer\n");
	}
	if ((g_shm_buffer = (int *)shmat(g_shm_buffer_id, NULL, 0)) == (int *)-1) {
		fprintf(stderr, "Impossivel associar o buffer\n");
	}
	// Buffer

	//Inicializando semaforo
	exc_aces_up[0].sem_num = 0;
	exc_aces_up[0].sem_op = 1;
	exc_aces_up[0].sem_flg = 0;

	exc_aces_down[0].sem_num = 0;
	exc_aces_down[0].sem_op = -1;
	exc_aces_down[0].sem_flg = 0;

	if((exc_aces_id = semget(SEM_KEY, 1, IPC_CREAT | 0666)) == -1) {
		fprintf(stderr, "chamada da semget() falhou, impossivel criar o semaforo!\n");
		exit(1);
	}

	if(semop(exc_aces_id, exc_aces_down, 1) == -1) {
		fprintf(stderr, "chamada semop() falhou, impossivel inicializar o semaforo!\n");
		exit(1);
	}
	if(semop(exc_aces_id, exc_aces_up, 1) == -1) {
		fprintf(stderr, "chamada semop() falhou, impossivel inicializar o semaforo!\n");
		exit(1);
	}


	// Fork
	pid_t clientes[NO_CLIENTS];
	pid_t barbeiros[NO_BARBERS];
	pid_t rtn = 1;
	// Inicializando Clientes
	for (int i = 0; i < NO_CLIENTS; i++) {
		if((int)rtn != 0) {
			rtn = fork();
			clientes[i] = rtn;
		}
		if(rtn == 0) {
			customer(queue_idCus, queue_idBar, (rand() % 1021) + 2); // rand() % 1021 => 0-1021 + 2 => 2-1023
			exit(0);
		}
	}
	// Inicializando Barbeiros
	for (int i = 0; i < NO_BARBERS; i++) {
		if((int)rtn != 0) {
			rtn = fork();
			barbeiros[i] = rtn;
		}
		if(rtn == 0) {
			barber(queue_idCus, queue_idBar); // Falta parametros
			exit(0);
		}
	}

	//Aguardar Finalização dos filhos
	for(int i = 0; i < NO_CLIENTS; i++) {
		wait(0);
	}

	sleep(1);
	for(int i = 0; i < NO_BARBERS; i++) {
		kill(barbeiros[i], 15);
	}

	// Fechando Semaforo
	if( semctl(exc_aces_id, 0, IPC_RMID, 0) != 0 ) {
		fprintf(stderr,"Impossivel remover o conjunto de semaforos!\n");
		exit(1);
	}

	// Fechando Fila de Mensagens
	if(msgctl(queue_idBar, IPC_RMID, NULL) == -1 ) {
		fprintf(stderr,"Impossivel remover a fila de mensagem!\n");
		exit(1);
	}
	if(msgctl(queue_idCus, IPC_RMID, NULL) == -1 ) {
		fprintf(stderr,"Impossivel remover a fila de mensagem!\n");
		exit(1);
	}

	// Fechando buffer
	if(shmctl(g_shm_buffer_id, IPC_RMID, NULL) != 0 ) {
		fprintf(stderr,"Impossivel remover o Buffer!\n");
		exit(1);
	}

	//Finalizaçao da Main
	return 0;
}
// Estrutura a ser enviada para o barbeiro pelo cliente, no momento de atendimento
typedef struct {
	char *stringEmbaralhada;
	pid_t pidCliente;
	struct timeval tempoIncio;
	unsigned int tamString;
} tDadosCustReady;

typedef struct {
	char *stringOrganizada;
	struct timeval tempoFinal;
} tdadosBarReady;

void barber (int filaCustReady, int filaBarReady){ // Main Barber
	// Variaveis
	msgbuf_t message_bufferRCV;
	msgbuf_t message_bufferSND;
	struct timeval tempoAtendimento;

	tdadosBarReady *dadosAtendimento = (tdadosBarReady *)(message_bufferSND.mtext);

	while(1) {
		// Espera cliente para comecar atendimento
		if( msgrcv(filaCustReady,(struct msgbuf_t *)&message_bufferRCV,sizeof(tDadosCustReady),MESSAGE_MTYPE,0) == -1 ) {
			fprintf(stderr, "Impossivel receber mensagem!\n");
			exit(1);
		}

		// Começo atendimento
		gettimeofday(&tempoAtendimento);
		dadosAtendimento->tempoFinal = tempoAtendimento;

		cut_hair((data_ptr->stringEmbaralhada), &(dadosAtendimento->stringOrganizada), data_ptr->tamString);

		// Envia resultados do atendimento
		if( msgsnd(filaBarReady,(struct msgbuf_t *)&message_bufferSND,sizeof(tdadosBarReady),0) == -1 ) {
			fprintf(stderr, "Impossivel enviar mensagem!\n");
			exit(1);
		}
	}
	return;
}

void customer (int filaCustReady, int filaBarReady, int tamString){ // Main Customer
	// Variaveis
	char stringEmbaralhada[tamString];
	char stringOrganizada[tamString];
	msgbuf_t message_bufferSND;
	msgbuf_t message_bufferRCV;
	struct timeval tempoIncio;
	double tempoTotal;

	// Controle de espera


	//Incializa Buffer
	tDadosCustReady *dadosCliente = (tDadosCustReady *)(message_bufferSND.mtext);
	message_bufferSND.mtype = MESSAGE_MTYPE;

	// Incia tempo de atendimento
	gettimeofday(&tempoIncio, NULL);
	dadosCliente->tempoIncio = tempoIncio;

	// Pid do Cliente
	dadosCliente->pidCliente = getpid();

	//String Embaralhada
	getRandomString(&stringEmbaralhada, tamString);
	dadosCliente->tamString = tamString;
	dadosCliente->stringEmbaralhada = &stringEmbaralhada;

	// Envia dados na fila de mensagem
	if( msgsnd(filaCustReady,(struct msgbuf_t *)&message_bufferSND,sizeof(tDadosCustReady),0) == -1 ) {
		fprintf(stderr, "Impossivel enviar mensagem!\n");
		exit(1);
	}

	// Espera dados do fim do atendimento
	if( msgrcv(filaBarReady,(struct msgbuf_t *)&message_bufferRCV,sizeof(tdadosBarReady),MESSAGE_MTYPE,0) == -1 ) {
		fprintf(stderr, "Impossivel receber mensagem!\n");
		exit(1);
	}

	// Recebe string Organizada
	stringOrganizada = data_ptr->stringOrganizada;
	// Calculo tempo total atendimento -> (data_ptr->tempoFinal - tempoIncio)
	tempoTotal = tempoIncio.tv_sec  - (data_ptr->tempoFinal).tv_sec;
	tempoTotal += (tempoIncio.tv_usec - (data_ptr->tempoFinal).tv_usec)/(float)MICRO_PER_SECOND;

	apreciate_hair(stringEmbaralhada, stringOrganizada, tamString, tempoTotal);

	return;
}

// Converter string enviada pelo cliente em vários int, ordenado de forma decresente
void cut_hair (char *stringAntes, **stringDepois,  int tamString){

}

void apreciate_hair (char *stringAntes, *stringDepois, int tamString, double tempoTotal){

}

