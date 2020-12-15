#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "timer.h"
#include <unistd.h> //para usar a funcao "sysconf"


//variaveis globais
float *matA; //matriz de entrada
float *matX; //matriz de saida 
float *matB; //segunda matriz de entrada
int dim; //dimensoes da matriz de entrada
int threads; //numero de threads desejadas



void *multiplicaMatrizMatriz(void *tid)
{
   int td = *(int *) tid;
   int i, j, k;

   for(i = td; i < dim; i+=threads)
   {
   	
		for(k = 0; k < dim; k++)
		{
			for(j = 0; j < dim; j++)
			{
			 	matX[i*dim+k] += matA[i*dim+j] * matB[j*dim+k];
			}
		}
   }

   free(tid);
   pthread_exit(NULL);
}


//main
int main(int argc, char *argv[]) {
   
   int numCPU = sysconf(_SC_NPROCESSORS_ONLN); 
   printf("Numero de processadores: %d\n", numCPU);

    double inicio, fim, delta1, delta2, delta3;

   //int dim;
   GET_TIME(inicio); 

   if(argc<2){
      printf("Digite %s<dimensão da matriz>\n", argv[0]);
      return 1;
   }
   
   dim = atoi(argv[1]);

   //int i, j, k;

   //alocação de memoria para as estruturas de dados
   matA = (float *) malloc(sizeof(float) *dim *dim);
   if(matA == NULL) {printf("ERRO--malloc\n"); return 2;}
   matB = (float *) malloc(sizeof(float) *dim *dim);
   if(matB == NULL) {printf("ERRO--malloc\n"); return 2;}
   matX = (float *) malloc(sizeof(float) *dim *dim);
   if(matX == NULL) {printf("ERRO--malloc\n"); return 2;}

   //inicialização das estruturas de dados

   for(int i=0; i<dim; i++){
      for (int j = 0; j < dim; j++){
         matA[i*dim+j] = 2;
         matB[i*dim+j] = 1;
         matX[i*dim+j] = 0;
      }
   }

   GET_TIME(fim);

   //calcula o tempo gasto com as inicializacoes
   delta1 = fim - inicio;

 /*
   //exibição dos resultados
   puts("Matriz A");
   for (int i = 0; i < dim; i++){
      for (int j = 0; j < dim; j++){
         printf("%.lf ", matA[i*dim+j]);
      }
      puts("");
   }

   puts("Matriz B");
   for (int i = 0; i < dim; i++){
      for (int j = 0; j < dim; j++){
         printf("%.lf ", matB[i*dim+j]);
      }
      puts("");
   }
*/
   
//multiplicação sequencial

/*   for(i = 0; i < dim; i++){
      for(k = 0; k < dim; k++){
         for(j = 0; j < dim; j++){
            matX[i*dim+k] += matA[i*dim+j] * matB[j*dim+k];
         }
      }
   }

*/

   pthread_t *tid_sistema;
   int *tid;
   int t;

   printf("Digite o numero de threads desejadas:\n");
   scanf("%d", &threads);

   if(threads>dim) threads = dim;

   tid_sistema = (pthread_t *) malloc(sizeof(pthread_t) * threads);
   if(tid_sistema == NULL)
   {
      printf("--ERRO: malloc()\n");
      exit(-1);
   }

   GET_TIME(inicio);

   for(t=0; t<threads; t++){
      tid = malloc(sizeof(int)); 
      if(tid==NULL){ 
         printf("--ERRO: malloc()\n"); exit(-1); 
      }

      *tid = t;
      
      if (pthread_create(&tid_sistema[t], NULL, multiplicaMatrizMatriz, (void*) tid)){
         printf("--ERRO: pthread_create()\n"); exit(-1);
      }
   }
   //espera todas as threads terminarem e imprime o vetor de saída
   for(t=0; t<threads; t++) 
   {
     if (pthread_join(tid_sistema[t], NULL)) 
     {
         printf("--ERRO: pthread_join()\n"); 
         exit(-1);
     }
   }
   
   GET_TIME(fim);

   delta2 = fim - inicio;


/* 
   //exibição dos resultados

   puts("Matriz X");
   for (int i = 0; i < dim; i++){
      for (int j = 0; j < dim; j++){
         printf("%.lf ", matX[i*dim+j]);
      }
      puts("");
   }
*/
  
   //libera os espacos de memoria alocados
   GET_TIME(inicio);
   free(matA);   
   free(matX);   
   free(matB); 
   GET_TIME(fim);

   //calcula o tempo gasto com as finalizacoes 
   delta3 = fim - inicio;

   printf("Tempo inicializacoes: %.8lf\n", delta1);
   printf("Tempo multiplicacao matriz e matriz com %d threads: %.8lf\n", threads, delta2);
   printf("Tempo finalizacoes: %.8lf\n", delta3);


   return 0;
}

