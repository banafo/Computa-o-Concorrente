#include <pthread.h> //para criação das threads e locks.
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "timer.h"  //para usar GET_TIME()
#include <unistd.h> //para usar a funcao "sysconf"

//#define TAM 10000000
//#define NTHREADS 10

int NTHREADS;     // Variavel para numero de threads
int contaThread;  // Varaivel auxilia para testes
pthread_mutex_t mutex;  //Variavel para usar lock e unlock

//Struct para os dados que as threads vao receber
// args_thread deve ser thread-safe, pois enquanto vet é
// compartilhado, lados [esquerda, direita] não se sobrepõe entre as threads.
typedef struct  {
  double *vet;
  int low;
  int high;
}args_threads;


//Funçãos Criadas
void troca(double vet[], int a, int b);             //Função para fazer troca de posição
int particao(double vet[], int low, int high);      //Função para fazer a partição do vetor
void quicksort(double vet[], int low, int high);    //Quick Sort sequencial auxiliar
void quick_sort(double vet[], int tam, int threads);//Função chamada na main do programa
void *Quicksort_parallela(void *threadarg);         // Quick Sort auxiliar para a função quick_sort  
int ordenado(double vet[], int tam);                //Função para testa se o vetor esta ordenado


//----------------------------------------------------------------------------------------------------------------

//Main
//Gera Numeros randomicos
//Inicialização do vetor
//Chama a função quick_sort
//Tempo dos execuçoes
int main(int argc, char *argv[])
{
  pthread_mutex_init(&mutex, NULL);
  srand(time(NULL));
  int NUM ;
  double inicio, fim, delta1, delta2, delta3; //variaveis para medir o tempo de execucao

  //descobre quantos processadores a maquina possui
  int numCPU = sysconf(_SC_NPROCESSORS_ONLN); 
  printf("Numero de processadores: %d\n\n", numCPU);


//--------------------------------------------------------------------------------------------------------------------

  GET_TIME(inicio);

  if(argc < 3) {
    printf("Use: %s Entra com <numero de elementos> <numero de threads>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  //Pegando os dados do usuario
  NUM = atoi(argv[1]);
  NTHREADS = atoi(argv[2]);
  contaThread = NTHREADS;

  //verifica para numero de threads não seja maior que o tamanho do vetor
  if(NTHREADS > NUM) NTHREADS = NUM ;

  //Allocação de espaço para o vetor
  double *vet = (double *) malloc(NUM * sizeof(double));

  //initializando o vetor com numeros randômicos
  for (int i = 0; i < NUM; i++) {
    vet[i] = rand()%10 + 1;
  }
  GET_TIME(fim);

  //calcula o tempo gasto com as inicializacoes
  delta1 = fim - inicio;

  //imprimir vetor nao sorteado
  printf("Vetor Original:\n");
  printf("[ ");
  /*for (int i = 0; i < NUM; i++) {
     printf("%f ", vet[i] );
  }*/
  puts("]\n");
  
  GET_TIME(inicio);
  quick_sort(vet, NUM, NTHREADS); // função para ordenar os elementos do vetor
  GET_TIME(fim);
  
  //calcula o tempo gasto com a parte concorrente (Quick_Sort)
  delta2 = fim - inicio;

  printf("Vetor Ordenado:\n");
  printf("[ ");

  GET_TIME(inicio);

  if (!ordenado(vet, NUM))
  {
    printf("Vetor nao foi ordenado.\n");
  }

  /*for (int i = 0; i < NUM; i++) {
    printf("%f ", vet[i] );
  }*/

  puts("]");

  free(vet); // libera o espaçõ
  GET_TIME(fim);

  //calcula o tempo gasto com as finalizacoes 
  delta3 = fim - inicio;

   //exibe os tempos gastos em cada parte do programa 
  printf("Tempo inicializacoes: %.8lf\n", delta1);
  printf("Tempo quicksort com %d threads: %.8lf\n", NTHREADS, delta2);
  printf("Tempo finalizacoes: %.8lf\n", delta3);

  pthread_mutex_destroy(&mutex);
  pthread_exit(NULL);
}



//------------------------------------------------------------------------------------------------------------------------------


//Função chamada na main do programa
//Se Thread_level > 1, então particione e faça Threads Quicksort_parallela para resolver a esquerda e
//do lado direito. Caso Thread_level < 1 retorna e se for igual 1 chame sequencial.

void quick_sort(double vet[], int size, int Thread_level)
{
  if(Thread_level < 1) return;   //Se numero de threads for menor que o program termina
  else if(Thread_level == 1) quicksort(vet, 0, size-1);   //se o numero de thread for igual a 1 quicksort sequencial é chamado
  else{
    int mid = particao(vet, 0, size-1);
    
  	// criar threads para o lados esquerdo e direito. Deve criar seus argumentos de dados.
  	args_threads vetor_dadosThread[2];
  	
  	for (int i = 0; i < 2; i++) vetor_dadosThread[i].vet = vet;
    vetor_dadosThread[0].low = 0;
    vetor_dadosThread[0].high = mid - 1;
    vetor_dadosThread[1].low = mid + 1;
    vetor_dadosThread[1].high = size-1;
    contaThread -= 2;

   //Instancie os threads no QuickSOrt devido à propriedade transitiva nenhum elemento nos lados esquerdo e direito do mid terá
   // que ser comparado novamente.

    pthread_t id_sistema[2];
    for (int i = 0; i < 2; i++) {  
      if (pthread_create(&id_sistema[i], NULL, Quicksort_parallela,(void *) &vetor_dadosThread[i])) {
        printf("--ERRO: pthread_create()\n");
        exit(-1);
      }
    }
    //Junte os lados esquerdo e direito para terminar.
    for (int i = 0; i < 2; i++) {   
      if (pthread_join(id_sistema[i],NULL)) {
        printf("--ERRO: pthread_join()\n");
        exit(-1);
      }
    }
  }
}
//------------------------------------------------------------------------------------------------------------------------------
// Quick Sort auxiliar para a função quick_sort 

void *Quicksort_parallela(void *threadarg)
{
  
  int mid, i;

  args_threads *dados;
  dados = (args_threads *) threadarg;
  

  pthread_mutex_lock(&mutex);
  int teste0 = (contaThread == 0);
  int teste2 = (contaThread > 1);
  pthread_mutex_unlock(&mutex);
  
  if (teste0 || dados->low == dados->high+1) { // Não há mais threads para criar ou há mais threads que o tamanho do vetor
    quicksort(dados->vet, dados->low, dados->high);
  }
  else if (teste2) { // cria threads para os lados esquerdo e direito com seus argumentos de dados.
    pthread_mutex_lock(&mutex);
    contaThread -= 2;
    pthread_mutex_unlock(&mutex);
    
    //Partição do vetor
    mid = particao(dados->vet, dados->low, dados->high);

    // criar threads para o lados esquerdo e direito. Deve criar seus argumentos de dados.
  	args_threads vetor_dadosThread[2];
    
    for (i = 0; i < 2; i++) vetor_dadosThread[i].vet = dados->vet;
    vetor_dadosThread[0].low = dados->low;
    vetor_dadosThread[0].high = mid - 1;
    vetor_dadosThread[1].low = mid + 1;
    vetor_dadosThread[1].high = dados->high;


   //Instancie os threads no QuickSOrt devido à propriedade transitiva nenhum elemento nos lados esquerdo e direito do mid terá
   // que ser comparado novamente.

    pthread_t id_sistema[2];
    for (i = 0; i < 2; i++) {  
      if (pthread_create(&id_sistema[i], NULL, Quicksort_parallela,(void *) &vetor_dadosThread[i])) {
        printf("--ERRO: pthread_create()\n");
        exit(-1);
      }
    }
    //Junte para terminar.
    for (i = 0; i < 2; i++) {   
      if (pthread_join(id_sistema[i],NULL)) {
        printf("--ERRO: pthread_join()\n");
        exit(-1);
      }
    }
  }
  else { // Chama a última thread restante
  	pthread_mutex_lock(&mutex);
  	contaThread--;
  	pthread_mutex_unlock(&mutex);
  	
  	mid = particao(dados->vet, dados->low, dados->high);
  	
  	// Função pthread receber apenas um argumento, usamos struct.
  	args_threads targs;
    targs.vet = dados->vet;
    targs.low = dados->low;
    targs.high = mid-1;
  	
  	
  	pthread_t tid_sistema;
    if (pthread_create(&tid_sistema, NULL, Quicksort_parallela, (void *) &targs)) {
      printf("--ERRO: pthread_create()\n"); exit(-1);
    } 
  
    quicksort(dados->vet, mid+1, dados->high);
     
     //Junte para terminar.
    if (pthread_join(tid_sistema, NULL)) {
      printf("--ERRO: pthread_join()\n"); exit(-1);
    }
  }

  pthread_exit(NULL);
}

//------------------------------------------------------------------------------------------------------------------------------
//Quick Sort sequencial auxiliar
void quicksort(double vet[], int low, int high)
{
  if (low >= high)
    return;
  int b = particao(vet, low, high);
  quicksort(vet, low, b - 1);
  quicksort(vet, b + 1, high);
}
//------------------------------------------------------------------------------------------------------------------------------
//Função para fazer troca de posição
void troca(double vet[], int a, int b)
{
  double temp = vet[a];
  vet[a] = vet[b];
  vet[b] = temp;
}
//------------------------------------------------------------------------------------------------------------------------------
//Função para fazer a partição do vetor

int particao (double vet[], int low, int high) 
{ 
  int b = low;
  double pivot = vet[high];

  for (int i = low; i <= high; i++) 
  { 
    if (vet[i] < pivot) 
    { 
      troca(vet , i, b); 
      b++;
    } 
  } 
  troca(vet, high, b);  
  return b; 
} 

//--------------------------------------------------------------------------------------------------------------------------------
//Função para testa se o vetor esta ordenado

int ordenado(double vet[], int tam)
{
  for (int i = 1; i < tam; i++) {
    if (vet[i] < vet[i - 1]) {
      printf("Na posicao %d, %f < %f \n", i, vet[i], vet[i-1]);
      return 0;
    }
  }
  return 1;
}
