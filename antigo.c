#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MAX_DEPARTMENTS 5
#define MAX_REQUESTS_PER_DEPARTMENT 10

typedef enum { T1, T2 } PrinterType;

typedef struct {
  int department;
  int request_number;
} PrintRequest;

typedef struct {
  PrinterType type;
  int success_count;
  int failure_count;
  sem_t mutex;
} Printer;

PrintRequest print_requests[MAX_DEPARTMENTS][MAX_REQUESTS_PER_DEPARTMENT];
Printer printers[MAX_DEPARTMENTS];
int total_requests = 0;

char department_names[MAX_DEPARTMENTS][25] = {
    "Departamento Brasil", "Departamento Russia", "Departamento India",
    "Departamento China", "Departamento South Africa"};

void initialize_print_requests();
void initialize_printers(int num_printers);
void create_print_threads(int num_printers);
void join_print_threads(int num_printers);
void print_request(int department, int request_number, int printer_id);
void *print_thread(void *arg);
void print_summary(int num_printers);
void print_horizontal_line(int length);
void print_frame_row(int length);
void print_frame_cell(const char *content, int width);
void print_frame_separator(int length);

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Uso: %s <quantidade de impressoras>\n", argv[0]);
    return 1;
  }

  int num_printers = atoi(argv[1]);

  srand(time(NULL));

  initialize_print_requests();
  initialize_printers(num_printers);
  create_print_threads(num_printers);

  join_print_threads(num_printers);

  print_summary(num_printers);

  return 0;
}

void initialize_print_requests() {
  for (int i = 0; i < MAX_DEPARTMENTS; i++) {
    for (int j = 0; j < MAX_REQUESTS_PER_DEPARTMENT; j++) {
      print_requests[i][j].department = -1;
    }
  }

  for (int i = 0; i < MAX_DEPARTMENTS; i++) {
    for (int j = 0; j < MAX_REQUESTS_PER_DEPARTMENT; j++) {
      total_requests++;
      print_requests[i][j].department = i;
      print_requests[i][j].request_number = j;
    }
  }
}

void initialize_printers(int num_printers) {
  for (int i = 0; i < num_printers; i++) {
    printers[i].type = (i % 2 == 0) ? T1 : T2;
    printers[i].success_count = 0;
    printers[i].failure_count = 0;
    sem_init(&printers[i].mutex, 0, 1);
  }
}

void create_print_threads(int num_printers) {
  pthread_t threads[MAX_DEPARTMENTS];
  int printer_ids[MAX_DEPARTMENTS];

  for (int i = 0; i < num_printers; i++) {
    printer_ids[i] = i;
    pthread_create(&threads[i], NULL, print_thread, &printer_ids[i]);
  }

  for (int i = 0; i < num_printers; i++) {
    sem_post(&printers[i].mutex);
  }
}

void join_print_threads(int num_printers) {
  pthread_t threads[MAX_DEPARTMENTS];

  for (int i = 0; i < num_printers; i++) {
    pthread_join(threads[i], NULL);
  }
}

void print_request(int department, int request_number, int printer_id) {
  int success = 1;
  int sleep_time = rand() % 5 + 1; // Tempo de impressão aleatório

  // Simula falha na impressão para impressoras do tipo T1
  if (printers[printer_id].type == T1) {
    if (rand() % 100 < 15) { // 15% de chance de falha
      success = 0;
    }
  }

  printf("Setor %s está realizando solicitação #%d de impressão\n",
         department_names[department], request_number);
  printf("Impressora #%d está imprimindo solicitação #%d do setor %s\n",
         printer_id, request_number, department_names[department]);

  sleep(sleep_time);

  if (success) {
    printf("Impressão #%d concluída com sucesso\n", request_number);
    printers[printer_id].success_count++;
  } else {
    printf("Impressão #%d falhou\n", request_number);
    printers[printer_id].failure_count++;
  }

  sem_post(&printers[printer_id].mutex);
}

void *print_thread(void *arg) {
  int printer_id = *((int *)arg);

  while (1) {
    sem_wait(&printers[printer_id].mutex);

    // Verifica se há mais solicitações para imprimir
    int completed = 1;
    for (int i = 0; i < MAX_DEPARTMENTS; i++) {
      if (print_requests[i][0].department != -1) {
        completed = 0;
        break;
      }
    }

    if (completed) {
      sem_post(&printers[printer_id].mutex);
      break;
    }

    int department = -1;
    int request_number = -1;

    // Encontra próxima solicitação a ser impressa
    for (int i = 0; i < MAX_DEPARTMENTS; i++) {
      if (print_requests[i][0].department != -1) {
        department = print_requests[i][0].department;
        request_number = print_requests[i][0].request_number;

        // Move as outras solicitações para frente na fila
        for (int j = 0; j < MAX_REQUESTS_PER_DEPARTMENT - 1; j++) {
          print_requests[i][j] = print_requests[i][j + 1];
        }
        print_requests[i][MAX_REQUESTS_PER_DEPARTMENT - 1].department = -1;

        break;
      }
    }

    print_request(department, request_number, printer_id);
  }

  pthread_exit(NULL);
}

void print_summary(int num_printers) {
  print_frame_separator(55);
  printf("|%-25s|%-14s|%-14s|\n", "Impressora", "Impressões", "Falhas");
  print_frame_separator(55);

  for (int i = 0; i < num_printers; i++) {
    printf("|%-25d|%-14d|%-14d|\n", i, printers[i].success_count,
           printers[i].failure_count);
  }

  print_frame_separator(55);
  printf("\nNúmero de impressões de cada setor:\n");
  for (int i = 0; i < MAX_DEPARTMENTS; i++) {
    int department_total = 0;
    for (int j = 0; j < MAX_REQUESTS_PER_DEPARTMENT; j++) {
      if (print_requests[i][j].department != -1) {
        department_total++;
      }
    }
    printf("Setor %d: %d\n", i, department_total);
  }

  printf("Número total de solicitações: %d\n", total_requests);
}

void print_horizontal_line(int length) {
  for (int i = 0; i < length; i++) {
    printf("-");
  }
  printf("\n");
}

void print_frame_row(int length) {
  printf("|");
  for (int i = 0; i < length - 2; i++) {
    printf(" ");
  }
  printf("|\n");
}

void print_frame_cell(const char *content, int width) {
  int padding = width - strlen(content);
  int left_padding = padding / 2;
  int right_padding = padding - left_padding;

  printf("|");
  for (int i = 0; i < left_padding; i++) {
    printf(" ");
  }
  printf("%s", content);
  for (int i = 0; i < right_padding; i++) {
    printf(" ");
  }
}

void print_frame_separator(int length) {
  printf("+");
  for (int i = 0; i < length - 2; i++) {
    printf("-");
  }
  printf("+\n");
}