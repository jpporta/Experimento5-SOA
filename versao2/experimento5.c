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
#include <string.h>
#include <time.h>

#define num_barbers 3
#define num_clients 27
#define num_chairs 7
#define MAXSIZEVECTOR 5115

/* Mutex e Variáveis globais*/
/* Região crítica*/
//Para struct mensChair
pthread_mutex_t sem_exc_aces1;
//Para struct mensAfter
pthread_mutex_t sem_exc_aces2;
// Semáforos
sem_t sem_customers;
sem_t sem_barber;

typedef struct mensChair{
  char *stringEmbaralhada;
  int numCliente;
  unsigned int tam;
  struct mensChair *prox;
} mensChair;

typedef struct mensAfter{
  char *stringPronta;
  int numBarber;
  int numCliente;
  struct mensAfter *prox;
} mensAfter;

/* Fila de espera, sempre será menor que num_chairs */
int waiting = 0;
/* Quantos foram atendidos (e sairam da loja) */
int left = 0;
/* Filas */
struct mensChair *filaMensagem;
struct mensAfter *prontaMensagem;

//Protótipos
void cut_hair(int num, struct mensChair *notReady, struct mensAfter *ready);
void apreciate_hair(int num, struct timeval tempoIncio, struct mensChair *structVelha, struct mensAfter *structNova);
void *barberThread(void *arg);
void *clientThread(void *arg);
/* Protótipos para manipulação de lista ligada */
void criaVaziaEmb(struct mensChair **ponteiro);
void insereEmb(struct mensChair **source, struct mensChair *novo);
void criaVaziaOrd(struct mensAfter **ponteiro);
void insereOrd(struct mensAfter **source, struct mensAfter *new);
int retiraListaPronto(struct mensAfter **source, struct mensAfter *structCliente, int numeroCliente);
// Prototipos para a srting
void vetorAleatorio(int tam, int *vetor);
void vetorToString (int tam, char *stringRet, int* vetor);
void stringToVetor(int *vetor, char* string);
int comparator(const void *p, const void *q);




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

  //Inicializando Listas Ordenadas
  criaVaziaEmb(&filaMensagem);
  criaVaziaOrd(&prontaMensagem);

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
  //Número do Barbeiro
  int i = *(int *)arg;
  //Struct com a string ordenada
  struct mensAfter *strReady = (struct mensAfter *) malloc(sizeof(struct mensAfter));
  //Struct com a string embaralhada
  struct mensChair *strNotReady = (struct mensChair *)malloc(sizeof(struct mensChair));

  //Loop para o barbeiro esperar todos serem atendidos
  while(left < num_clients){
    //Espera algum cliente entrar na fila
    sem_wait(&sem_customers);
    //Acesso à região crítica
    pthread_mutex_lock(&sem_exc_aces1);
    //Pega o primeiro da fila
    strNotReady = filaMensagem;
    //Anda a fila
    filaMensagem = strNotReady->prox;
    //Libera região crítica
    pthread_mutex_unlock(&sem_exc_aces1);
    //Corta o cabelo
    cut_hair(i, strNotReady, strReady);
    //Acesso à 2 região crítica
    pthread_mutex_lock(&sem_exc_aces2);
    //Insere na fila prontos
    insereOrd(&prontaMensagem,strReady);
    //Libera região crítica
    pthread_mutex_unlock(&sem_exc_aces2);
    //Libera barbeiro
    sem_post(&sem_barber);
  }

  //Sai da thread
  pthread_exit(NULL);
}

//Procedimento que as threads Clientes irão executar
void *clientThread(void *arg){
  //Status para ver se ele tirou ou nao da filaM
  int retirado = 0;
  //Numero do cliente
  int i = *(int *)arg;
  //Condição para LOOP
  int atendido = 0;
  //Para cálculo de tempo
  struct timeval tempoIncio;
  /* Setando struct para tirar da lista 2 */
  struct mensAfter *final = (struct mensAfter *)malloc(sizeof(struct mensAfter));
  /* SETANDO STRUCT PARA COLOCAR NA LISTA */
  //Aloca struct
  struct mensChair *atual = (struct mensChair *)malloc(sizeof(struct mensChair));
  //O tamanho aleatorio da string
  atual->tam = ((rand() % 1021) + 2);
  //Passa o numero do cliente
  atual->numCliente = i;
  //Declara a string embaralhada e ordenada
  char strEmbaralhada[atual->tam];
  int vetEmbaralhado[atual->tam];
  //Gera a string aleatoria
  //getRandomString(&strEmbaralhada,atual->tam);
  vetorAleatorio(atual->tam, vetEmbaralhado);
  /*for (int z = 0; z < atual->tam; z++) {
    printf("%d ", vetEmbaralhado[z]);
  }*/
  //printf("\n");
  vetorToString(atual->tam, strEmbaralhada, vetEmbaralhado);
  //printf("STRING -> %s\n", strEmbaralhada);
  //Coloca na struct
  atual->stringEmbaralhada = strEmbaralhada;
  /* FINAL SETUP */

  //Pega o tempo que o cliente entrou na loja
  gettimeofday(&tempoIncio, NULL);

  //Enquanto ele não for atendido, vai ficar no loop tentando
  while(!atendido){

    //Região crítica
    pthread_mutex_lock(&sem_exc_aces1);
    //Ve quantas pessoas tem esperando
    sem_getvalue(&sem_customers,&waiting);

    //Se tiver menos que o número de cadeiras permite
    if(waiting < num_chairs){

      //Coloca as informações na fila
      insereEmb(&filaMensagem, atual);
      //Libera região crítica
      pthread_mutex_unlock(&sem_exc_aces1);
      //Aumenta o numero de customers esperando
      sem_post(&sem_customers);
      //Espera ter um barbeiro livre
      sem_wait(&sem_barber);
      //Acessa 2 região crítica
      while(!retirado){
        pthread_mutex_lock(&sem_exc_aces2);
        //Retira a struct certa da lista de prontos
        retirado = retiraListaPronto(&prontaMensagem, final, i);
        //Libera acesso 2 regiao crítica
        pthread_mutex_unlock(&sem_exc_aces2);

      }
      //Aprecia cabelo
      apreciate_hair(i, tempoIncio, atual, final);
      //Aumenta a quantidade de pessoas atendidas
      left++;
      //Foi atendido
      atendido = 1;

    } else {
      //Libera região crítica
      pthread_mutex_unlock(&sem_exc_aces1);
      //Sai pra tomar sorvete
      usleep(50);
    }
  } // Fim do while Atendido

  //Sai da thread
  pthread_exit(NULL);
}

//Procedimento para cortar o cabelo
void cut_hair(int num, struct mensChair *notReady, struct mensAfter *ready){
  /* ORDENAR A STRING DA NOTREADY E COLOCAR NA READY */
  //char stringZona[MAXSIZEVECTOR];
  //char stringBack[MAXSIZEVECTOR];
  int vet[notReady->tam];

  stringToVetor(vet, notReady->stringEmbaralhada);
  qsort((void *)notReady->stringEmbaralhada, notReady->tam, sizeof(notReady->stringEmbaralhada[0]), comparator);
  vetorToString(notReady->tam, ready->stringPronta, vet);

  usleep(500);
}

//Procedimento para apreciar o cabelo
void apreciate_hair(int num, struct timeval tempoIncio, struct mensChair *structVelha, struct mensAfter *structNova){
  struct timeval tempoFim;
  double tempoAtendimento;
  gettimeofday(&tempoFim, NULL);

  tempoAtendimento = tempoFim.tv_sec - tempoIncio.tv_sec;
  tempoAtendimento += (tempoFim.tv_usec - tempoIncio.tv_usec)/(float)1000;
  /* PEGAR STRING ANTIGA, STRING NOVA, TEMPO QUE DEMOROU E QUAL BARBEIRO QUE CORTOU O CABELO */
  printf("Tempo de atendimento : %lf\t-\tString embaralhada : %s\t-\tString pronta : %s\t-\tCliente : %d\t-\tBarbeiro : %d\n", tempoAtendimento, structVelha->stringEmbaralhada, structNova->stringPronta, structNova->numCliente, structNova->numBarber);
}

void criaVaziaEmb(struct mensChair **ponteiro){
  *ponteiro = NULL;
}
void insereEmb(struct mensChair **source, struct mensChair *novo){
  //Struct auxiliar
  mensChair *aux;

  //Caso exista o 1 elemento da lista
  if(*source){
    //Utiliza a struct auxiliar para andar
    aux = *source;
    //Enquanto não chegar no final
    while(aux->prox){
      //Anda
      aux = aux->prox;
    }

  aux->prox = novo;

  } else {
    //Caso não tenha elemento, é adicionado como o primeiro elemento
    *source = novo;
    //return 1;
  }

}
void criaVaziaOrd(struct mensAfter **ponteiro){
  *ponteiro = NULL;
}
void insereOrd(struct mensAfter **source, struct mensAfter *new){
  //Struct auxiliar
  struct mensAfter *aux;

  //Caso exista o 1 elemento da lista
  if(*source){
    //Utiliza a struct auxiliar para andar
    aux = *source;
    //Enquanto não chegar no final
    while(aux->prox){
      //Anda
      aux = aux->prox;
    }
    //Se tiver menos elementos que o numero de barbeiros, adiciona
    aux->prox = new;
  } else {
    //Caso não tenha elemento, é adicionado como o primeiro elemento
    *source = new;
    //return 1;
  }
}

int retiraListaPronto(struct mensAfter **source, struct mensAfter *structCliente, int numeroCliente){
  int status = 0;
  //Struct auxiliares
  struct mensAfter *aux, *remover;
  //Primeiro ponteiro
  aux = *source;

  //Se o primeiro elemento for o correto
  if(aux->numCliente == numeroCliente){
    //Salvo ele
    structCliente = aux;
    //O começo da lista é o elemento 2
    *source = aux->prox;
    //Libero espaço
    free(aux);
    //Voltou com sucesso
    status = 1;
    return status;
  }
  else {
    //Enquanto não for o final da fila -- VAI DAR MERDA
    while(aux != NULL){
      //Se o proximo elemento for o correto
      if(((aux->prox)->numCliente) == numeroCliente){
        //Pega o elemento
        remover = (aux->prox);
        //Pula ele (anterior aponta pro proximo)
        aux->prox = remover->prox;
        //Salva o elemento
        structCliente = remover;
        //Libera espaço
        free(remover);
        //Voltará sucesso
        status = 1;
        return status;
      }
      //Anda a lista
      aux = aux->prox;
    }
  }

  return 0;
}

void vetorAleatorio(int tam, int *vetor){
    time_t t;
   // int tamString = 0;

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
