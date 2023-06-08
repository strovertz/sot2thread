#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>

#define MAX 100 //  impressora
#define NUM_SETORES 5// steores
#define MAX_IMPRESSOES_SETOR 5 // impressões por setor

typedef struct {
    int tipo; // t1??? t2??? 
    int num_impressoes_sucesso; //sucesso
    int num_impressoes_falha; // falhas de impressão
    sem_t sem_impressora; // controle da impressora
} Impressora;

typedef struct {
    int id; // Id setor
    int num_solicitacoes; // a serem feitas pelo setor
    int num_impressoes_realizadas; // impressões scesso
    sem_t sem_setor; // Semáforo para controle do setor
} Setor;

Impressora impressoras[MAX];
Setor setores[NUM_SETORES];

void esperar() {
    int tempo_espera = rand() % 5 + 1; 
    sleep(tempo_espera);
}

// simula a impressão
void* imprimir(void* arg) {
    int id_impressora = *(int*)arg;
    Impressora* impressora = &impressoras[id_impressora];
    bool teste = true;
    while (teste) {
        sem_wait(&impressora->sem_impressora);

        int setor_id = 0;

        for (int i = 0; i < NUM_SETORES; i++) {
            if (sem_trywait(&setores[i].sem_setor) == 0) {
                setor_id = i;
                break;
            }
        }

        if (id_impressora != -1 && setores[setor_id].num_solicitacoes <= MAX_IMPRESSOES_SETOR) {
            printf("Setor %d está realizando solicitação de impressão #%d.\n", setores[setor_id].id, setores[setor_id].num_solicitacoes);
            if (impressora->tipo == 1 && rand() % 100 < 15) { 
                printf("Impressão falhou para a solicitação #%d do setor %d. Reenviando solicitação...\n", setores[setor_id].num_impressoes_realizadas, setores[setor_id].id);
                impressora->num_impressoes_falha++;
                setores[setor_id].num_solicitacoes++;
             } else {
                if (setores[setor_id].num_impressoes_realizadas <=MAX_IMPRESSOES_SETOR) {
                    sem_post(&impressoras[id_impressora].sem_impressora);
                    setores[setor_id].num_solicitacoes++;
                    setores[setor_id].num_impressoes_realizadas++;
                    impressora->num_impressoes_sucesso++;
                    esperar();
                    printf("Solicitação #%d do Setor %d impressa com sucesso pela Impressora #%d (Tipo: %d).\n", setores[setor_id].num_solicitacoes, setores[setor_id].id, id_impressora, impressora->tipo);
                } 
            }
             sem_post(&impressora->sem_impressora);
             for ( int i = 0; i < NUM_SETORES; i++) {
                if (setores[setor_id].num_impressoes_realizadas == MAX_IMPRESSOES_SETOR) {
                            sem_post(&setores[setor_id].sem_setor);
                            teste = false;
                        }
                }
            sem_post(&setores[setor_id].sem_setor);
        } else {
            sem_post(&impressora->sem_impressora);
            teste = false;
            break;
        }

        sem_post(&impressora->sem_impressora);
    }

    pthread_exit(NULL);
}

//  simula o envio de solicitação de impressão poir um setor
void* enviar_solicitacao(void* arg) {
    int setor_id = *(int*)arg;
    int max = 0;
   printf("Setor ID: %d\n", setor_id);
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
            for (int j = 0; j < max; j++) {
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


void relatorio(int max){
    printf("\n--- Relatório de Impressões ---\n");
    for (int i = 0; i < max; i++) {
        Impressora* impressora = &impressoras[i];
        printf("Impressora #%d:\n", i);
        printf("  Tipo: T%d\n", impressora->tipo);
        printf("  Impressões com sucesso: %d\n", impressora->num_impressoes_sucesso);
        printf("  Falhas de impressão: %d\n", impressora->num_impressoes_falha);
        printf("\n");
    }
    if(max == NUM_SETORES || max > NUM_SETORES){
        for (int i = 0; i < NUM_SETORES; i++) {
            printf("Setor #%d:\n", i);
            printf("  Numero de Solicitacoes: %d\n", setores[i].num_solicitacoes);
            printf("  Impressões com sucesso: %d\n", setores[i].num_impressoes_realizadas);
            printf("\n");
        }
    } else if(NUM_SETORES > max) {
        for (int i = 0; i < max; i++) {
            printf("Setor #%d:\n", i);
            printf("  Numero de Solicitacoes: %d\n", setores[i].num_solicitacoes);
            printf("  Impressões com sucesso: %d\n", setores[i].num_impressoes_realizadas);
            printf("\n");
        }
    }
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


    // init setores
    for (int i = 0; i < NUM_SETORES; i++) {
        Setor* setor = &setores[i];
        setor->id = i;
        setor->num_solicitacoes = 0; //define a qtd de solicitações por setor conforme necessário
        setor->num_impressoes_realizadas = 0;
        sem_init(&setor->sem_setor, 0, 1);
    }

    // inicializa impresspra
    for (int i = 0; i < max; i++) {
        Impressora* impressora = &impressoras[i];
        impressora->tipo = (i % 2) + 1; // t1/t2
        impressora->num_impressoes_sucesso = 0;
        impressora->num_impressoes_falha = 0;
        sem_init(&impressora->sem_impressora, 0, 1);
    }

    // cria trhead setor
    for (int i = 0; i < NUM_SETORES; i++) {
        int* id_setor = malloc(sizeof(int));
        *id_setor = i;
        if (&i == NULL || id_setor== NULL) printf("problema no malloc");
        //printf("Setor %d Criado", setores[i].id); //cria todos os setores corretamente, o erro deve estar na hora de enviar solicitacao
        if(pthread_create(&threads_setores[i], NULL, enviar_solicitacao, id_setor)) printf("Pthread create error no setor %d \n", i);
    }
   // cria thread imp
    for (int i = 0; i < max; i++) {
        int* id_impressora = malloc(sizeof(int));
        *id_impressora = i;
        if (&i == NULL || id_impressora == NULL) printf("deu pau");
        if(pthread_create(&threads_impressoras[i], NULL, imprimir, id_impressora)) printf("Pthread create error no setor %d \n", i);
    }
    //iespera finalização das threads das impressoras
    for (int i = 0; i < max; i++) {
        pthread_join(threads_impressoras[i], NULL);
    }

    // espera finalização das threads dos setores
    for (int i = 0; i < NUM_SETORES; i++) {
        pthread_join(threads_setores[i], NULL);
    }

    relatorio(max);

    return 0;
}

