#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <unistd.h> 
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/fcntl.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>

#define SEM_KEY 6789
#define SHM_KEY 9876
#define MSG_KEY_BARBER 34567
#define MSG_KEY_CUSTOMER 56473
#define MAXSIZEVECTOR 5115
#define NO_SEATSWR 7
#define NO_OF_CLIENTS 20
#define NO_OF_BARBERS 2
#define MESSAGE_MTYPE 1
#define MICRO_PER_SECOND 1000000

int clientesNaoAtendidos = 0;

int shmid;
int semid;
struct sembuf	g_sem_op1[1];
struct sembuf	g_sem_op2[1];
int horaDeTrabalho = 1;

// Estrutura da mensagem para o barbeiro
typedef struct {
    char stringEmbaralhada[5115];
    pid_t pidCliente;
    int tamString;
} t_msgToBarber;

// Estrura da mensagem de retorno para o cliente
typedef struct {
    char stringOrganizada[5115];
    int tamString;
    pid_t pidBarber;
} t_msgToClient;

typedef struct {
	long mtype;
	char mtext[8192];
} msgbuf_t;

int s, m, q1, q2; // For sig_handler ipcClear

//Declaracao das funcoes
void barber(int idMsgBarber, int idMsgCustomer);
void cliente(int tam, int idMsgBarber, int idMsgCustomer);
void vetorAleatorio(int tam, int *vetor);
void vetorToString (int tam, char * stringRet, int* vetor);
void stringToVetor(int *vetor, char* string);
int comparator(const void *p, const void *q);
void aprraiseHair(struct timeval init, struct timeval end, char *cabeloBaguncado, char *cabeloArrumado, pid_t barberPID, pid_t clientPID);
void ipcClear();

int main(int argc, char *argv[]){
    // Shared memory creation
    key_t shmkey = SHM_KEY;
    int *seatWR;
    int idMsgBarber;
    int idMsgCustomer;
    signal(SIGINT, ipcClear);
    if((shmid = shmget(shmkey, sizeof(int), IPC_CREAT | 0666)) < 0){
        perror("Impossivel criar SHM");
        exit(1);
    }
    if((seatWR = (int*)shmat(shmid, NULL, 0)) == (void*)-1){
        perror("Impossivel acessar SHM");
        exit(1);
    }
    m = shmid;
    // Inicializacao da memoria compartilhada
    *seatWR = NO_SEATSWR;
    // Fim memoria compartilhada em main
    //Semaforos creation
    key_t semkey = SEM_KEY;
    //Criando operações
    g_sem_op1[0].sem_num  =  0;
	g_sem_op1[0].sem_op   = -1;
	g_sem_op1[0].sem_flg  =  0;

    g_sem_op2[0].sem_num =  0;
	g_sem_op2[0].sem_op  =  1;
	g_sem_op2[0].sem_flg =  0;

    //Criando o semaforo
    if((semid = semget(semkey, 1, IPC_CREAT | 0666)) == -1){
        perror("Impossivel criar semaforo");
        exit(1);
    }
    s = semid;
    //Testando as operacoes
    // Bloqueio
    if(semop(semid, g_sem_op2, 1) == -1){
        perror("Impossivel inicializar semaforo");
        exit(1);
    }
    // Liberacao
        if(semop(semid, g_sem_op1, 1) == -1){
        perror("Impossivel inicializar semaforo");
        exit(1);
    }
    // Fim semaforos
    //Criando fila de mensagens
    key_t keyMsgBarber = MSG_KEY_BARBER;
    key_t keyMsgCustomer = MSG_KEY_CUSTOMER;
    
    if((idMsgBarber = msgget(keyMsgBarber, IPC_CREAT | 0666)) == -1){
        perror("Impossivel criar fila msg barbeiro");
        exit(1);
    }
    q1 = idMsgBarber;
    if((idMsgCustomer = msgget(keyMsgCustomer, IPC_CREAT | 0666)) == -1){
        perror("Impossivel criar fila msg Customer");
        exit(1);
    }
    q2 = idMsgCustomer;
    // Aterando tamanho das filas
    struct msqid_ds buf1;
    if (msgctl(idMsgBarber, IPC_STAT, &buf1) == -1){
        perror("Nao foi possivel alterar fila de msg");
        exit(1);
    }
    struct msqid_ds buf2;
    if (msgctl(idMsgCustomer, IPC_STAT, &buf2) == -1){
        perror("Nao foi possivel alterar fila de msg");
        exit(1);
    }
    buf1.msg_qbytes = 8300;
    buf2.msg_qbytes = 8300;
    if (msgctl(idMsgBarber, IPC_SET, &buf1) == -1){
        perror("Nao foi possivel alterar fila de msg");
        exit(1);
    }
    if (msgctl(idMsgCustomer, IPC_SET, &buf2) == -1){
        perror("Nao foi possivel alterar fila de msg");
        exit(1);
    }
    // Todos IPCs Criados ---------------------------------------------------
    pid_t pidsClientes[NO_OF_CLIENTS];
    pid_t pidsBarbers[NO_OF_BARBERS];
    pid_t pidPai = getpid();
    pid_t rtn = (pid_t)1;
    time_t t;
    // Criando filhos
    for(int i = 0; i < NO_OF_BARBERS; i++){
        if(rtn != 0){
            rtn = fork();
            pidsBarbers[i] = rtn;
            if(rtn == 0) barber(idMsgBarber, idMsgCustomer);
        }
    }
    for(int i = 0; i < NO_OF_CLIENTS; i++){
        int tamRand = (rand() % 1022) + 2;
        if(rtn != 0){
            rtn = fork();
            pidsClientes[i] = rtn;
            if(rtn == 0) cliente(tamRand, idMsgBarber, idMsgCustomer);
        }
    }
    if(getpid() != pidPai) exit(0);

    for(int i = 0; i < NO_OF_BARBERS; i++){
        printf("Barbeiro no: %d PID: %d\n", i, pidsBarbers[i]);
    }
    for(int i = 0; i < NO_OF_CLIENTS; i++){
        printf("Cliente no: %d PID: %d\n", i, pidsClientes[i]);
    }
    //Esperando filhos
    for(int i = 0; i < NO_OF_CLIENTS; i++){
        waitpid(pidsClientes[i], NULL, 0);
    }
    horaDeTrabalho = 0;
    for(int i = 0; i < NO_OF_BARBERS; i++){
        kill(pidsBarbers[i], SIGINT);
    }
    //Finalizado IPCs -------------------------------------------------------
    ipcClear();

    return 0;
}
void barber(int idMsgBarber, int idMsgCustomer){
    msgbuf_t msgBufferEnv;
    msgbuf_t msgBufferRec;
    t_msgToBarber *msgRec = (t_msgToBarber *)(msgBufferRec.mtext);
    t_msgToClient *msgEnv = (t_msgToClient *)(msgBufferEnv.mtext);
    int vetor[MAXSIZEVECTOR];
    char cabelo[MAXSIZEVECTOR*5];
    int *seatWR;

    sleep(1);

    if((seatWR = (int*)shmat(shmid, NULL, 0)) == (void*)-1){
        perror("Impossivel acessar SHM");
        exit(1);
    }
    // loop
    while(horaDeTrabalho){
        // Receber Menssagem
        if(msgrcv(idMsgBarber, (struct msgbuf_t *)&msgBufferRec, sizeof(t_msgToBarber), MESSAGE_MTYPE, 0) == -1){
            perror("Erro ao receber menssagem @ barber");
            exit(1);
        }
        printf("menssagem recebida\n String = %s\n Pid = %d\n TamString = %d\n", msgRec->stringEmbaralhada, msgRec->pidCliente, msgRec->tamString);
        // Arrumando Cabelo
        strcpy(cabelo, msgRec->stringEmbaralhada);
        stringToVetor(vetor, cabelo);
        qsort((void*)vetor,sizeof(vetor), sizeof(vetor[0]), comparator);
        vetorToString(msgRec->tamString, cabelo, vetor);
        printf("cabelo tratado\n");
        // Devolver cabelo pro cliente
        msgBufferEnv.mtype = MESSAGE_MTYPE;
        msgEnv->pidBarber = getpid();
        strcpy(msgEnv->stringOrganizada, cabelo);
        msgEnv->tamString = msgRec->tamString;
        
        printf("Mandando msg\n");
        size_t sizeMsgClient = sizeof(msgEnv);
        if(msgsnd(idMsgCustomer, (struct msgbuf_t *)&msgBufferEnv, sizeMsgClient, 0) == -1){
            perror("Erro ao enviar mensagem @ Barber");
            exit(1);
        }

        if(semop(semid, g_sem_op1, 1) == -1){
            perror("Erro em fechar semaforo");
            exit(1);
        }
        // Regiao Critica
        *seatWR = *seatWR + 1;
        if(semop(semid, g_sem_op2, 1) == -1){
            perror("Erro em abrir semaforo");
            exit(1);
        }
            
    }
    exit(0);
}
void cliente(int tam, int idMsgBarber, int idMsgCustomer){
    struct timeval timeInit;
    struct timeval timeFim;

    msgbuf_t msgBufferEnv;
    msgbuf_t msgBufferRec;
    t_msgToBarber *msgEnv = (t_msgToBarber *)(msgBufferEnv.mtext);
    t_msgToClient *msgRec = (t_msgToClient *)(msgBufferRec.mtext);

    //Get cabelo baguncado
    char cabeloBaguncado[tam*5];
    char cabeloArrumado[tam*5];
    int vetor[tam];
    int *seatWR;
    vetorAleatorio(tam, vetor);
    vetorToString(tam, cabeloBaguncado, vetor);
    sleep(2);
    gettimeofday(&timeInit, NULL);
    // printf("alteracao em semaforos\n");
    // Verificar se barbeiro disponivel e cadeira na sala de espera
    if(semop(semid, g_sem_op2, 1) == -1){
        perror("Erro em fechar semaforo");
        exit(1);
    }
    // printf("Regiao critica, tentar usar SHM\n");
    // Regiao Critica
    if((seatWR = (int*)shmat(shmid, NULL, 0)) == (void*)-1){
        perror("Impossivel acessar SHM");
        exit(1);
    }
    // se nao houver assentos cliente vai embora
    if(*seatWR == 0){
        clientesNaoAtendidos++;
        if(semop(semid, g_sem_op2, 1) == -1){
            perror("Erro em abrir semaforo");
        }
        exit(1);
    }
    // se houver ocupa o lugar e continua
    *seatWR = *seatWR - 1;
    // printf("Regiao critica, usou SHM, seatWR = %d\n", *seatWR);
    if(semop(semid, g_sem_op1, 1) == -1){
            perror("Erro em abrir semaforo");
            exit(1);
    }
    // printf("Sai do semaforo, comeca enviar msg\n");
    // envio a lista de mensagens
    // Insercao na lista de mensagem
    pid_t myPid = getpid();
    msgBufferEnv.mtype = MESSAGE_MTYPE;
    msgEnv->tamString = tam;
    msgEnv->pidCliente = myPid;
    strcpy((msgEnv->stringEmbaralhada), cabeloBaguncado);
    size_t sizeMsgSend = sizeof(*msgEnv);
    if(msgsnd(idMsgBarber,(struct msgbuf_t *)&msgBufferEnv, sizeMsgSend, 0) == -1){
        if(errno == EINVAL) printf("YEAP");
        perror("Impossivel mandar mensagem @ client");
        exit(1);
    }
    // Recebendo cabelo arrumado  
    size_t sizeMsgRcv = sizeof(*msgRec);
    if(msgrcv(idMsgCustomer, (struct msgbuf_t *)&msgBufferRec, sizeMsgRcv, MESSAGE_MTYPE, 0) == -1){
        perror("Impossivel receber mensagem @ client");
        exit(1);
    }
    gettimeofday(&timeFim, NULL);
    strcpy(cabeloArrumado, msgRec->stringOrganizada);
    // Avaliado corte
    aprraiseHair(timeInit, timeFim, cabeloBaguncado, cabeloArrumado, msgRec->pidBarber, myPid);
    exit(0);
}
void vetorAleatorio(int tam, int *vetor){
    time_t t;
   int tamString = 0;

   srand((unsigned) time(&t));
   for(int x = 0; x < tam; x++){
       vetor[x] = (rand() % 1022) + 2;
   }
}
void vetorToString (int tam, char * stringRet, int* vetor){

    int strControl = 0;
    for(int x = 0; x < tam; x++){
        int num = vetor[x];
        if(strControl != 0){
            stringRet[strControl] = ' ';
            strControl++;
        }
        int divi;
        for (divi = 1; divi <= num; divi *= 10);
        do
        {
        divi /= 10;
        stringRet[strControl] = (num / divi) + '0';
        strControl++;
        num %= divi;
        } while (divi > 1);
    }
    stringRet[strControl] = '\0';
}
void stringToVetor(int *vetor, char* string){
    char delim[2] = " ";
    char *token;
    int vetorControl = 0;
    char stringDestroy[MAXSIZEVECTOR];

    strcpy(stringDestroy, string);

    token = strtok(stringDestroy, delim);
    while(token != NULL){
        vetor[vetorControl] = atoi(token);
        vetorControl++;
        token = strtok(NULL, delim);
    }
}
int comparator(const void *p, const void *q){
	int l = *(const int *)p;
	int r = *(const int *)q;
    if(l>r) return 1;
    return -1;
}
void aprraiseHair(struct timeval init, struct timeval end, char *cabeloBaguncado, char *cabeloArrumado, pid_t barberPID, pid_t clientPID){
    int tempoTotal;
    tempoTotal = end.tv_sec - init.tv_sec;
    tempoTotal += (end.tv_usec - init.tv_usec)/(float)MICRO_PER_SECOND;
    printf("Cabelo Desarrumado : %s\nCabelo Arrumado : %s\nTempo: %d\nBarbeiro: %d, Cliente: %d\n", cabeloBaguncado, cabeloArrumado, tempoTotal, barberPID, clientPID);

    return;
}
void ipcClear(){
     //Removing shared memory
    if(shmctl(m, IPC_RMID, NULL) != 0){
        perror("Erros em destruir SHM");
        exit(1);
    }
    // Removing Semaphore
    if(semctl(s, 0, IPC_RMID, 0) != 0){
        perror("Erro em destruir SEM");
        exit(1);
    }
    // Removing menssage queue
    if(msgctl(q1, IPC_RMID, NULL) == -1){
        perror("Erro em destruir msgBarber");
        exit(1);
    }
    if(msgctl(q2, IPC_RMID, NULL) == -1){
        perror("Erro em destruir msgCustomer");
        exit(1);
    }
}