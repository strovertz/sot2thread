#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>

#define MAX 100// Número máximo de impressoras
#define NUM_SETORES 5// Número de setores
#define MAX_IMPRESSOES_SETOR 4 // Número máximo de impressões por setor

typedef struct {
    int tipo; // Tipo da impressora (T1 ou T2)
    int num_impressoes_sucesso; // Número de impressões com sucesso
    int num_impressoes_falha; // Número de falhas de impressão
    sem_t sem_impressora; // Semáforo para controle da impressora
} Impressora;

typedef struct {
    int id; // Identificador do setor
    int num_solicitacoes; // Número de solicitações de impressão a serem feitas pelo setor
    int num_impressoes_realizadas; // Número de impressões realizadas pelo setor
    sem_t sem_setor; // Semáforo para controle do setor
} Setor;

Impressora impressoras[MAX];
Setor setores[NUM_SETORES];

// Função para simular o tempo de espera na impressão
void esperar() {
    int tempo_espera = rand() % 5 + 1; // Tempo de espera aleatório entre 1 e 5 segundos
    sleep(tempo_espera);
}

// Função para simular a impressão
void* imprimir(void* arg) {
    int id_impressora = *(int*)arg;
    Impressora* impressora = &impressoras[id_impressora];

    while (1) {
        sem_wait(&impressora->sem_impressora);

        int setor_id = -1;

        for (int i = 1; i <= NUM_SETORES; i++) {
            if (sem_trywait(&setores[i].sem_setor) == 0) {
                setor_id = i;
                break;
            }
        }

        if (id_impressora != -1 && setores[setor_id].num_solicitacoes < MAX_IMPRESSOES_SETOR) {
            printf("Setor %d está realizando solicitação de impressão #%d.\n", setores[setor_id].id, setores[setor_id].num_solicitacoes);
            if (impressora->tipo == 1 && rand() % 100 < 15) { // T1 com 15% de chance de falha
                printf("Impressão falhou para a solicitação #%d do setor %d. Reenviando solicitação...\n", setores[setor_id].num_impressoes_realizadas, setores[setor_id].id);
                impressora->num_impressoes_falha++;
             } else {
                if (setores[setor_id].num_impressoes_realizadas < MAX_IMPRESSOES_SETOR) {
                    sem_post(&impressoras[id_impressora].sem_impressora);
                    setores[setor_id].num_solicitacoes++;
                    setores[setor_id].num_impressoes_realizadas++;
                    impressora->num_impressoes_sucesso++;
                    esperar();
                    printf("Solicitação #%d do Setor %d impressa com sucesso pela Impressora #%d (Tipo: %d).\n", setores[setor_id].num_solicitacoes, setores[setor_id].id, id_impressora, impressora->tipo);
                } 
            }
             sem_post(&impressora->sem_impressora);
            if (setores[setor_id].num_impressoes_realizadas == MAX_IMPRESSOES_SETOR) {
                        sem_post(&setores[setor_id].sem_setor);
                        break;
                    }

            sem_post(&setores[setor_id].sem_setor);
        } else {
            sem_post(&impressora->sem_impressora);
            break;
        }

        sem_post(&impressora->sem_impressora);
    }

    pthread_exit(NULL);
}

// Função para simular o envio de solicitação de impressão por um setor
void* enviar_solicitacao(void* arg) {
    int setor_id = *(int*)arg;
    int max = 0;
   printf("Setor ID: %d", setor_id);
    Setor* setor = &setores[setor_id];
    for (int i = 0; i <  MAX; i++) {
        if (impressoras->tipo != 1 && impressoras->tipo != 2) {
            max = i;
            break;
        }
    }

   for (int i = setor->num_solicitacoes; i > 0; i--) {
        sem_wait(&setor->sem_setor);

        int impressora_id = -1;
        int tipo_impressora = -1;
        int sem_value_i;
        int sem_value_j;
        for (int j = 0; j < max; j++) {
            //sem_getvalue(&impressoras[j].sem_impressora, &sem_value_i);
            if (sem_trywait(&impressoras[j].sem_impressora) == 0 && impressoras[j].tipo == 1)  {
                impressora_id = j;
                tipo_impressora = 1;
                break;
            }
        }

        if (impressora_id == -1) {
            for (int j = 0; j < MAX; j++) {
                if (sem_trywait(&impressoras[j].sem_impressora) == 0 && impressoras[j].tipo == 2) {
                    impressora_id = j;
                    tipo_impressora = 2;
                    break;
                }
            }
        }

    

        sem_post(&setor->sem_setor);
    }

    pthread_exit(NULL);
}



int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Uso: ./impressoras <MAX>\n");
        return 1;
    }

    int max = atoi(argv[1]);

    srand(time(NULL));

    pthread_t threads_impressoras[max];
    pthread_t threads_setores[NUM_SETORES];


    // Inicialização dos setores
    for (int i = 0; i < NUM_SETORES; i++) {
        Setor* setor = &setores[i];
        setor->id = i + 1;
        setor->num_solicitacoes = 0; // Definir a quantidade de solicitações por setor conforme necessário
        setor->num_impressoes_realizadas = 0;
        sem_init(&setor->sem_setor, 0, 1);
    }

    // Inicialização das impressoras
    for (int i = 0; i < max; i++) {
        Impressora* impressora = &impressoras[i];
        impressora->tipo = (i % 2) + 1; // Definindo os tipos de impressora (T1 ou T2)
        impressora->num_impressoes_sucesso = 0;
        impressora->num_impressoes_falha = 0;
        sem_init(&impressora->sem_impressora, 0, 1);
    }

    // Criação das threads dos setores
    for (int i = 0; i < NUM_SETORES; i++) {
        int* id_setor = malloc(sizeof(int));
        *id_setor = i;
        //printf("Setor %d Criado", setores[i].id); //cria todos os setores corretamente, o erro deve estar na hora de enviar solicitacao
        if(pthread_create(&threads_setores[i], NULL, enviar_solicitacao, id_setor)) printf("Pthread create error no setor %d \n", i);
    }
   // Criação das threads das impressoras
    for (int i = 0; i < max; i++) {
        int* id_impressora = malloc(sizeof(int));
        *id_impressora = i;
        if(pthread_create(&threads_impressoras[i], NULL, imprimir, id_impressora)) printf("Pthread create error no setor %d \n", i);
    }
    // Aguardar finalização das threads das impressoras
    for (int i = 0; i < max; i++) {
        pthread_join(threads_impressoras[i], NULL);
    }

    // Aguardar finalização das threads dos setores
    for (int i = 0; i < NUM_SETORES; i++) {
        pthread_join(threads_setores[i], NULL);
    }

    printf("\n--- Relatório de Impressões ---\n");

    for (int i = 0; i < max; i++) {
        Impressora* impressora = &impressoras[i];
        printf("Impressora #%d:\n", i);
        printf("  Tipo: T%d\n", impressora->tipo);
        printf("  Impressões com sucesso: %d\n", impressora->num_impressoes_sucesso);
        printf("  Falhas de impressão: %d\n", impressora->num_impressoes_falha);
        printf("\n");
    }

    return 0;
}