/*  ALTERAÇÕES EM RELAÇÃO À TAREFA 1
* - Usar threads
* - Usar mutex
* - Usar semáforos customers e barbers
* - Ter 3 barbeiros, 27 clientes e 7 cadeiras
* - O corte (cut_hair) deve ser uma ordenação crescente do que o cliente disponibiliza
* - Não se deve usar fila de mensagens
* - O cliente deve ter acesso e exibir o próprio número, número do barbeiro que cortou o seu cabelo,
  quanto tempo demorou para ser atendido, os números que foram passados para o barbeiro e o resultado
  da ordenação
* - Os clientes não atendidos devem ir tomar um sorvete e entrar no estabelecimento novamente
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>
//Para o usleep
#include <unistd.h>
//temp
#include <sys/types.h>

#define num_barbers 3
#define num_clients 27
#define num_chairs 7

/* Mutex e Variáveis globais*/
/* Região crítica*/
//Para struct mensChair
pthread_mutex_t sem_exc_aces1;
//Para struct mensAfter
pthread_mutex_t sem_exc_aces2;
// Semáforos
sem_t sem_customers;
sem_t sem_barber;

typedef struct{
  char* stringEmbaralhada;
  int numCliente;
  unsigned int tam;
} mensChair;

typedef struct{
  char* stringPronta;
  int numCliente;
} mensAfter;

/* Fila de espera, sempre será menor que num_chairs */
int waiting = 0;
/* Quantos foram atendidos (e sairam da loja) */
int left = 0;
/* Filas */
struct mensChair *filaMensagem;
struct mensAfter *prontaMensagem;

//Protótipos
void cut_hair(int num);
void apreciate_hair(int num, int numBarbeiro, struct timeval tempoIncio);
void *barberThread(void *arg);
void *clientThread(void *arg);
/* Protótipos para manipulação de lista ligada */
void criaVaziaEmb(struct mensChair **ponteiro);
int insereEmb(struct mensChair **source, struct mensChair *novo);
void criaVaziaOrd(struct mensAfter **ponteiro);
int insereOrd(stuct mensAfter **source, struct mensAfter *new);



/*A função da main é apenas criar as threads (clientes e barbeiros),
iniciar mutex, esperar threads e destuir mutex*/
int main(){
  pthread_t barbers[num_barbers];
  pthread_t clients[num_clients];
  int numeroBarbeiros[num_barbers];
  int numeroClientes[num_clients];


  /*INICIAR MUTEX*/
  if(pthread_mutex_init(&sem_exc_aces1, NULL)){
    fprintf(stderr, "Erro ao inicializar mutex exc_aces1\n");
    return -1;
  }
  if(pthread_mutex_init(&sem_exc_aces2, NULL)){
    fprintf(stderr, "Erro ao inicializar mutex exc_aces2\n");
    return -1;
  }
  if(sem_init(&sem_customers,0,0)){
    fprintf(stderr, "Erro ao inicializar semaphore customers\n");
    return -1;
  }
  if(sem_init(&sem_barber,0,num_barbers)){
    fprintf(stderr, "Erro ao inicializar semaphore barber\n");
    return -1;
  }

  //Disparando barbeiros
  for(int i = 0; i < num_barbers;i++){
    numeroBarbeiros[i] = i + 1;
    if(pthread_create(&barbers[i], NULL, barberThread, &numeroBarbeiros[i])){
      fprintf(stderr, "Erro ao Inicializar barbeiro %d\n", i);
      return -1;
    }
  }

  //Disparando clientes
  for(int i = 0; i < num_clients; i++){
    numeroClientes[i] = i + 1;
    if(pthread_create(&clients[i], NULL, clientThread, &numeroClientes[i])){
      fprintf(stderr, "Erro ao Inicializar cliente %d\n", i);
      return -1;
    }
  }

  //Espera clientes e barbeiros
  for(int i = 0; i < num_barbers; i++){
    if(pthread_join(barbers[i],NULL)){
      fprintf(stderr, "Erro ao esperar Barbeiro %d\n", i+1);
      return -1;
    }
  }
  for(int i = 0; i < num_clients; i++){
    if(pthread_join(clients[i], NULL)){
      fprintf(stderr, "Erro ao esperar cliente %d\n", i);
      return -1;
    }
  }

  //Destruindo MUTEX
  if(pthread_mutex_destroy(&sem_exc_aces1)){
    fprintf(stderr, "Erro ao tentar destruir mutex exc_aces1\n");
    return -1;
  }
  if(pthread_mutex_destroy(&sem_exc_aces2)){
    fprintf(stderr, "Erro ao tentar destruir mutex exc_aces2\n");
    return -1;
  }
  if(sem_destroy(&sem_customers)){
    fprintf(stderr, "Erro ao tentar destruir semaphore customers\n");
    return -1;
  }
  if(sem_destroy(&sem_barber)){
    fprintf(stderr, "Erro ao tentar destruir semaphore barber\n");
    return -1;
  }

  return 0;
}

//Procedimento que as threads Barbeiros irão executar
void *barberThread(void *arg){

  int i = *(int *)arg;
  while(left < 27){
    sem_wait(&sem_customers);
    cut_hair(i);
    sem_post(&sem_barber);
  }

  pthread_exit(NULL);
}

//Procedimento que as threads Clientes irão executar
void *clientThread(void *arg){

  int i = *(int *)arg;
  int loopCounter = 0;
  int atendido = 0;
  struct timeval tempoIncio;
  /* SETANDO STRUCT PARA COLOCAR NA LISTA */
  struct mensChair *atual = malloc(sizeof(struct mensChair));

  atual->tam = (rand() % 1021) + 2);
  atual->numCliente = i;

  char strEmbaralhada[atual.tam];
  char strOrdenada[atual.tam];

  getRandomString(&strEmbaralhada,atual.tam);
  atual->stringEmbaralhada = &strEmbaralhada;
  /* FINAL SETUP */

  gettimeofday(&tempoIncio, NULL);

  while(!atendido){

    sem_getvalue(&sem_customers,&waiting);

    if(waiting < num_chairs){

      sem_post(&sem_customers);
      sem_wait(&sem_barber);
      //TRAVAR AQUI PARA ORDENACAO

      apreciate_hair(i, whichBarber, tempoIncio);
      left++;
      atendido = 1;

    } else {

      //printf("Cliente %i saindo para sorvete\n", i);
      usleep(50);
    }
  } // Fim do while Atendido

  pthread_exit(NULL);
}

//Procedimento para cortar o cabelo
void cut_hair(int num){
  //TRAVAR PARA ORDENAR
  //printf("%d Cortando cabelo\n", num);
  usleep(500);
}

//Procedimento para apreciar o cabelo
void apreciate_hair(int num, int numBarbeiro, struct timeval tempoIncio){
  struct timeval tempoFim;
  double tempoAtendimento;
  gettimeofday(&tempoFim, NULL);

  tempoAtendimento = tempoFim.tv_sec - tempoIncio.tv_sec;
  tempoAtendimento += (tempoFim.tv_usec - tempoIncio.tv_usec)/(float)1000;
  printf("Cliente %i\t\tBarbeiro %d\t\tTemp - %lf (ms)\n", num, numBarbeiro, tempoAtendimento);
}

void criaVaziaEmb(struct mensChair **ponteiro){
  *ponteiro = NULL;
}
int insereEmb(struct mensChair **source, struct mensChair *novo){
  //Struct auxiliar
  mensChair *aux;
  //Contador de quantos elementos tem na lista
  count = 1;

  //Caso exista o 1 elemento da lista
  if(*source){
    //Utiliza a struct auxiliar para andar
    aux = *source;
    //Enquanto não chegar no final
    while(aux->prox){
      //Anda
      aux = aux->prox;
      //Contador ++
      count++;
    }
    //Se tiver menos elementos que o numero de cadeiras, adiciona
    if(count < num_chairs){
      aux->prox = novo;
      return 1;
    } else {
      //Se não, vai ficar esperando
      return 0;
    }
  } else {
    //Caso não tenha elemento, é adicionado como o primeiro elemento
    *source = novo;
    return = 1;
  }

}
void criaVaziaOrd(struct mensAfter **ponteiro){
  *ponteiro = NULL;
}
int insereOrd(stuct mensAfter **source, struct mensAfter *new){
  //Struct auxiliar
  mensAfter *aux;
  //Contador de quantos elementos tem na lista
  count = 1;

  //Caso exista o 1 elemento da lista
  if(*source){
    //Utiliza a struct auxiliar para andar
    aux = *source;
    //Enquanto não chegar no final
    while(aux->prox){
      //Anda
      aux = aux->prox;
      //Contador ++
      count++;
    }
    //Se tiver menos elementos que o numero de barbeiros, adiciona
    if(count < num_barbers){
      aux->prox = new;
      return 1;
    } else {
      //Se não, vai ficar esperando
      return 0;
    }
  } else {
    //Caso não tenha elemento, é adicionado como o primeiro elemento
    *source = new;
    return = 1;
  }
}
