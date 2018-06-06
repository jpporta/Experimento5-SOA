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
/* Região crítica da variável waiting */
pthread_mutex_t sem_exc_aces;
// Semáforos
sem_t sem_customers;
sem_t sem_barber1;
sem_t sem_barber2;
sem_t sem_barber3;
sem_t printApp;

/* Fila de espera, sempre será menor que num_chairs */
int waiting = 0;
/* Quantos foram atendidos (e sairam da loja) */
int left = 0;

//Protótipos
void cut_hair(int num);
void apreciate_hair(int num, int numBarbeiro, struct timeval tempoIncio);
void *barberThread(void *arg);
void *clientThread(void *arg);

/*A função da main é apenas criar as threads (clientes e barbeiros),
iniciar mutex, esperar threads e destuir mutex*/
int main(){
  pthread_t barbers[num_barbers];
  pthread_t clients[num_clients];
  int numeroBarbeiros[num_barbers];
  int numeroClientes[num_clients];

  /*INICIAR MUTEX*/
  if(pthread_mutex_init(&sem_exc_aces, NULL)){
    fprintf(stderr, "Erro ao inicializar mutex exc_aces\n");
    return -1;
  }
  if(sem_init(&sem_customers,0,0)){
    fprintf(stderr, "Erro ao inicializar semaphore customers\n");
    return -1;
  }
  if(sem_init(&sem_barber1,0,1)){
    fprintf(stderr, "Erro ao inicializar semaphore barber1\n");
    return -1;
  }
  if(sem_init(&sem_barber2,0,1)){
    fprintf(stderr, "Erro ao inicializar semaphore barber2\n");
    return -1;
  }
  if(sem_init(&sem_barber3,0,1)){
    fprintf(stderr, "Erro ao inicializar semaphore barber3\n");
    return -1;
  }
  if(sem_init(&printApp,0,1)){
    fprintf(stderr, "Erro ao inicializar semaphore para print\n");
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
  if(pthread_mutex_destroy(&sem_exc_aces)){
    fprintf(stderr, "Erro ao tentar destruir mutex exc_aces\n");
    return -1;
  }
  if(sem_destroy(&sem_customers)){
    fprintf(stderr, "Erro ao tentar destruir mutex customers\n");
    return -1;
  }
  if(sem_destroy(&sem_barber1)){
    fprintf(stderr, "Erro ao tentar destruir semaphore barber1\n");
    return -1;
  }
  if(sem_destroy(&sem_barber2)){
    fprintf(stderr, "Erro ao tentar destruir semaphore barber2\n");
    return -1;
  }
  if(sem_destroy(&sem_barber3)){
    fprintf(stderr, "Erro ao tentar destruir semaphore barber3\n");
    return -1;
  }
  if(sem_destroy(&printApp)){
    fprintf(stderr, "Erro ao tentar destruir semaphore para print\n");
    return -1;
  }

  return 0;
}

//Procedimento que as threads Barbeiros irão executar
void *barberThread(void *arg){

  int i = *(int *)arg;

  switch(i){
    case 1:
      while(left<27){

          sem_wait(&sem_customers);
          pthread_mutex_lock(&sem_exc_aces);
          waiting--;
          pthread_mutex_unlock(&sem_exc_aces);
          cut_hair(i);
          sem_post(&sem_barber1);

      }
    break;
    case 2:
      while(left<27){

          sem_wait(&sem_customers);
          pthread_mutex_lock(&sem_exc_aces);
          waiting--;
          pthread_mutex_unlock(&sem_exc_aces);
          cut_hair(i);
          sem_post(&sem_barber2);

      }
    break;
    case 3:
      while(left<27){

          sem_wait(&sem_customers);
          pthread_mutex_lock(&sem_exc_aces);
          waiting--;
          pthread_mutex_unlock(&sem_exc_aces);
          cut_hair(i);
          sem_post(&sem_barber3);

      }
    break;
  }
  pthread_exit(NULL);
}

//Procedimento que as threads Clientes irão executar
void *clientThread(void *arg){

  int i = *(int *)arg;
  int whichBarber = 0;
  int atendido = 0;
  struct timeval tempoIncio;

  gettimeofday(&tempoIncio, NULL);
  while(!atendido){

    pthread_mutex_lock(&sem_exc_aces);

    if(waiting < num_chairs){

      waiting++;
      left++;
      sem_post(&sem_customers);
      pthread_mutex_unlock(&sem_exc_aces);

      while(1){
        if(!sem_trywait(&sem_barber1)){
          whichBarber = 1;
          break;
        }
        if(!sem_trywait(&sem_barber2)){
          whichBarber = 2;
          break;
        }
        if(!sem_trywait(&sem_barber3)){
          whichBarber = 3;
          break;
        }
      }

       sem_wait(&printApp);
       apreciate_hair(i, whichBarber, tempoIncio);
       sem_post(&printApp);

      atendido = 1;

    } else{

      //printf("Cliente %i saindo\n", i);
      pthread_mutex_unlock(&sem_exc_aces);
      usleep(50);
    }
  } // Fim do while Atendido


  pthread_exit(NULL);
}

//Procedimento para cortar o cabelo
void cut_hair(int num){
  //printf("Barbeiro %i cortando o cabelo\n", num);
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
