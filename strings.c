#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define MAXSIZEVECTOR 5115

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
// void organizarVetor(int *vetor, int tam){
//     int sizeVetor = sizeof(vetor)/sizeof(vetor[0]);
//     qsort((void *)vetor, sizeVetor, sizeof(vetor[0]), comparator);
// }

int main(){
    int tam = 30;
    int vetor[tam];
    int vetor2[tam];
    char stringZona[MAXSIZEVECTOR];
    char stringBack[MAXSIZEVECTOR];

    // Parte do cliente
    vetorAleatorio(tam, vetor);
    vetorToString(tam, stringZona, vetor);

    // Parte do barbeiro
    stringToVetor(vetor2, stringZona);
    // organizarVetor(vetor2, tam);
    int sizeVetor = sizeof(vetor2)/sizeof(vetor2[0]);
    qsort((void *)vetor2, sizeVetor, sizeof(vetor2[0]), comparator);
    vetorToString(tam, stringBack, vetor2);
    printf("%s\n", stringZona);
    printf("%s\n", stringBack);
    return 0;
}