#include<semaphore.h>
#include<pthread.h>
#include<stdlib.h>
#include<stdio.h>
#include "timer.h" 
#include <unistd.h> //para usar a funcao "sysconf"

#define N 500   // Tamanho dos blocos
#define M 3     // Entradas do buffer
#define T 4     // Número de threads

// Variáveis globais - início

char* buffer[M];                        // Buffer de blocos
int topo = 0, quantidade = 0, l = 0, e = 0, terminouDeEscrever = 0, blocosLidos[T-1];  // Variáveis de controle
long long int numeros;                 // Quantidade de números a serem lidos
sem_t condLeitura, condEscrita, mutex; // Semáforo para sincronização das threads
long long int t1[] = {0, 0}, t2 = 0, t3 = 0;     // Variáveis de retorno
char c1 = ' ';

// Variáveis globais - fim

// Função das Threads - início
void* leArquivo(void* arg);
void* iguais(void* arg);
void* triplas(void* arg);
void* sequencia(void* arg);
// Função das Threads - fim

//Funções Criadas

// Funções auxiliares - início
long long int leitura(FILE*, long long int*);   //Função para leitura
void testaTermino();							// Função para verificar término da leitura das threads
void atualizaBlocos(int);						//Função para atualização de bloco
void inicializaBuffer();						//Função para inicializção do buffer
void liberaBuffer();							//Função para libera buffer
// Funções auxiliares - fim

//---------------------------------------------------------------------------------------------------------


//Main

int main(int argc, char* argv[]){

  double inicio, fim, delta1; //variaveis para medir o tempo de execucao

  //descobre quantos processadores a máquina possui
  int numCPU = sysconf(_SC_NPROCESSORS_ONLN); 
  printf("Numero de processadores: %d\n\n", numCPU);

	if(argc != 2){
		puts("Formato de entrada: ./prog <arquivo binário>");
		return -1;
	}
	
	FILE* arquivo = fopen(argv[1], "r"); // Abre arquivo para leitura
	
	// Inicia o buffer e os semáforos
	inicializaBuffer();
	sem_init(&condEscrita, 0, 0);
	sem_init(&condLeitura, 0, 0);
	sem_init(&mutex, 0, 1);
	
	GET_TIME(inicio);

	pthread_t tid[T]; // Identificador da thread no sistema
	if(pthread_create(tid, NULL, leArquivo, (void*) arquivo)){
		puts("ERRO: Não foi possível criar thread para leitura de arquivo");
		return -1;
	}
	if(pthread_create(tid+1, NULL, iguais, NULL)){
		puts("ERRO: Não foi possível criar thread para contar maior sequência de números iguais");
		return -1;
	}
	if(pthread_create(tid+2, NULL, triplas, NULL)){
		puts("ERRO: Não foi possível criar thread para contar triplas");
		return -1;
	}
	if(pthread_create(tid+3, NULL, sequencia, NULL)){
		puts("ERRO: Não foi possível criar thread para contar seuqências de <012345>");
		return -1;
	}
	
	for(int i = 0; i < T; i++){
		if(pthread_join(tid[i], NULL)){
			puts("ERRO: Não foi possível esperar pelas threads");
			return -1;
		}
	}
	
	GET_TIME(fim);
	delta1 = fim - inicio;

	printf("\nMaior sequência de valores idênticos: %lli %lli %c\n", t1[0], t1[1], c1);
	printf("Quantidade de sequências contínuas de tamanho 3 do mesmo valor: %lli\n", t2);
	printf("Quantidade ocorrências da sequência <012345>: %lli\n\n", t3);
	
	printf("Tempo com %d threads: %.8lf s\n", T, delta1);

	// libera o buffer e destrói os semáforos
	liberaBuffer();
	sem_destroy(&condEscrita);
	sem_destroy(&condLeitura);
	sem_destroy(&mutex);
	fclose(arquivo); // Fecha arquivo
	return 0;
}

//-----------------------------------------------------------------------------------------------------------------

// Função das Threads - início
void* leArquivo(void* arg){
	int i = 0;
	long long int lidos = 0;
	
	sem_wait(&mutex);
	if(fread(buffer[topo], sizeof(char), N, (FILE*) arg) != 0){ // Se leu algo, então atualiza as variáveis de controle
		numeros = atoi(buffer[0]); // Quantidade de números que seram lidos no arquivo
		while(buffer[0][i] == ' ') i++;	// Pula espaços em branco no início do arquivo
		while(buffer[0][i] != ' '){ // Limpa o primeiro número da sequência no buffer (precisa que tamanho do buffer seja maior que 20 bytes)
			buffer[0][i] = ' ';
			i++;
		}
		for(int i = 0; i < N; i++){ // Percorre o arquivo para contar quantos números foram escritos
			if(lidos == numeros){
				terminouDeEscrever = 1;
				break;
			}
			if(buffer[topo][i] == '0' || buffer[topo][i] == '1' || buffer[topo][i] == '2' || buffer[topo][i] == '3' || buffer[topo][i] == '4' || buffer[topo][i] == '5')
			lidos++;
		}
		topo = (topo+1) % M;
		quantidade++;
		i++;
	}
	else terminouDeEscrever = 1;
	for(int i = 0; i < l; i++) sem_post(&condLeitura);
	l = 0;
	sem_post(&mutex);
	
	i = 0;
	if(lidos != numeros) while(leitura((FILE*) arg, &lidos) > 0);

	pthread_exit(NULL);
}
//----------------------------------------------------------------------------------------------------
void* iguais(void* arg){ // Maior sequência de valores idênticos
	int teste = 0, id = 0, base = 0, entrou = 1;
	long long int lidos = 0;
	char c = '0';
	while(1){
		testaTermino();
		
		// Padrão para computar maior sequência de iguais
		for(int i = 0; i < N; i++){
			if(buffer[base][i] == '0' || buffer[base][i] == '1' || buffer[base][i] == '2' || buffer[base][i] == '3' || buffer[base][i] == '4' || buffer[base][i] == '5'){
				lidos++;
				
				if(c == buffer[base][i]) teste++;
				else{
					if(teste > t1[1]){
						entrou = 0;
						c1 = c;
						t1[0] = lidos - teste;
						t1[1] = teste;
					}
					teste = 1;
					c = buffer[base][i];
				}
				if(lidos == numeros){
					if(entrou){ c1 = c; t1[0] = lidos - teste; t1[1] = teste; }
					pthread_exit(NULL);
				}
			}
		}
		if(entrou){ c1 = c; t1[0] = lidos - teste; t1[1] = teste; }
		base = (base+1) % M;
		
		atualizaBlocos(id);
	}
	
}
//----------------------------------------------------------------------------------------------------
void* triplas(void* arg){ // Triplas de números
	int teste = 0, id = 1, base = 0;
	long long int lidos = 0;
	char c = '0';
	while(1){
		testaTermino();
		
		// Padrão para computar triplas de números
		for(int i = 0; i < N; i++){
			if(buffer[base][i] == '0' || buffer[base][i] == '1' || buffer[base][i] == '2' || buffer[base][i] == '3' || buffer[base][i] == '4' || buffer[base][i] == '5'){
				lidos++;
				
				if(c == buffer[base][i]) teste++;
				else{
					c = buffer[base][i];
					teste = 1;
				}
				if(teste == 3){
					teste = 0;
					t2++;
				}
				if(lidos == numeros) pthread_exit(NULL);
			}
		}
		base = (base+1) % M;
		
		atualizaBlocos(id);
	}
}
//----------------------------------------------------------------------------------------------------
void* sequencia(void* arg){ // Sequência <012345>
	int teste = 0, id = 2, base = 0;
	long long int lidos = 0;
	char c[] = {'0', '1', '2', '3', '4', '5'};
	while(1){
		testaTermino();
		
		// Padrão para computar sequência <012345>		
		for(int i = 0; i < N; i++){
			if(buffer[base][i] == '0' || buffer[base][i] == '1' || buffer[base][i] == '2' || buffer[base][i] == '3' || buffer[base][i] == '4' || buffer[base][i] == '5'){
				lidos++;
					
				if(c[teste] == buffer[base][i]) teste++;
				else if(buffer[base][i] == '0') teste = 1;
				else teste = 0;
				if(teste == 5){
					teste = 0;
					t3++;
				}
				
				if(lidos == numeros) pthread_exit(NULL);
			}
		}
		base = (base+1) % M;
		
		atualizaBlocos(id);
	}
}
// Threads - fim


//----------------------------------------------------------------------------------------------------

// Funções auxiliares - início
//Função para leitura
long long int leitura(FILE* arquivo, long long int* lidos){
	sem_wait(&mutex);
	if(quantidade == M){ // Se buffer está cheio, então espera
		e++;
		sem_post(&mutex);
		sem_wait(&condEscrita);
		sem_wait(&mutex);
	}
	sem_post(&mutex);
	
	long long int ret = fread(buffer[topo], sizeof(char), N, arquivo); // Lê arquivo e escreve no buffer
	
	if(ret != 0){ // Se escreveu algo, então atualiza as variáveis de controle
		for(int i = 0; i < N; i++){
			if(*lidos == numeros){
				ret = 0;
				sem_wait(&mutex);
				terminouDeEscrever = 1;
				sem_post(&mutex);
				break;
			}
			if(buffer[topo][i] == '0' || buffer[topo][i] == '1' || buffer[topo][i] == '2' || buffer[topo][i] == '3' || buffer[topo][i] == '4' || buffer[topo][i] == '5')
				*lidos = *lidos + 1;
		}
		topo = (topo+1) % M;
		sem_wait(&mutex);
		quantidade++;
		sem_post(&mutex);
	}
	else{ // Senão escreve, então termina de escrever
		sem_wait(&mutex);
		terminouDeEscrever = 1;
		sem_post(&mutex);
	}
	sem_wait(&mutex);
	if(l > 0){ l--; sem_post(&condLeitura); } // Libera as threads bloqueadas
	sem_post(&mutex);
	return ret;
}
//----------------------------------------------------------------------------------------------------------------
//Função para verificar término da leitura das threads
void testaTermino(){
	sem_wait(&mutex);
	while(quantidade == 0 && terminouDeEscrever == 0){
		l++;
	 	sem_post(&mutex);
	 	sem_wait(&condLeitura);
	 	sem_wait(&mutex);
		if(l > 0){ l--; sem_post(&condLeitura); }
	}
	if(quantidade == 0 && terminouDeEscrever){ sem_post(&mutex); pthread_exit(NULL); }
	sem_post(&mutex);
}
//----------------------------------------------------------------------------------------------------------------
//Função para atualização de blocos
void atualizaBlocos(int id){	
	sem_wait(&mutex);
	blocosLidos[id]++;
	int menor = blocosLidos[0];
	for(int i = 1; i < T-1; i++) if(menor > blocosLidos[i]) menor = blocosLidos[i]; // Pega o menor número de blocos lidos entre as 3 threads
	for(int i = 0; i < T-1; i++) blocosLidos[i] -= menor; // Subtrai a menor quantidade de blocos lidos pelas threads
	quantidade -= menor; // Diz que as threads leram a quantidade na variável "menor"
	if(menor && e){ e--; sem_post(&condEscrita); }
	sem_post(&mutex);
	
	sem_wait(&mutex);
	while(quantidade != 0 && blocosLidos[id] == quantidade){ // Enquanto os blocos lidos forem iguais a quantidade, então deve esperar
		l++;
		sem_post(&mutex);
		sem_wait(&condLeitura);
		sem_wait(&mutex);
		if(l > 0){ l--; sem_post(&condLeitura); }
	}
	sem_post(&mutex);
}

//------------------------------------------------------------------------------------------------------
//Função para inicializção do buffer
void inicializaBuffer(){
	for(int i = 0; i < M; i++) buffer[i] = malloc(N*sizeof(char));
	for(int i = 0; i < T-1; i++) blocosLidos[i] = 0;
}

//------------------------------------------------------------------------------------------------------
//Função para libera buffer
void liberaBuffer(){
	for(int i = 0; i < M; i++) free(buffer[i]);
}
// Funções auxiliares - fim

