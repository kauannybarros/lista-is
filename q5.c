#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 200 //buffer das execuções pendentes de execuções
#define N 10 //número de threads e de núcleos

//struct para uma requisição de execução
typedef struct {
    int id;
    void *(*funexec)(void *);
    void *arg;
} Task;

//struct para armazenar o resultado de uma execução
typedef struct {
    int id;
    void *result;
} Result;

//buffer para guardar as tarefas(tá sendo implementado como uma lista circular)
Task task_buffer[BUFFER_SIZE];
int buffer_count = 0;
int next_task_id = 1;
int buffer_front = 0;
int buffer_rear = 0;

//buffer temporário que armazena os resultados
Result result_buffer[BUFFER_SIZE];
int result_count = 0;

//mutexes e variáveis de condição para sincronização
pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t buffer_not_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t buffer_not_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t result_available = PTHREAD_COND_INITIALIZER;

//controle de threads em atividade
int active_threads = 0;
pthread_mutex_t active_threads_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t thread_available = PTHREAD_COND_INITIALIZER;

//controle para terminar a thread despachante quando for necessário
volatile int running = 1;

void *funexec(void *arg); //função a ser executada
void *dispatcher_thread(void *arg); //thread despachante
void agendarExecucao(void *(*funexec)(void *), void *arg, int *id); //função que coloca a execução no buffer
void *pegarResultadoExecucao(int id); //resgata o resultado de uma função do buffer de resultados

//thread despachante: gerencia a execução de tarefas
void *dispatcher_thread(void *arg) {
    while (running) {
        pthread_mutex_lock(&buffer_mutex);

        //aguarda enquanto o buffer estiver vazio
        while (buffer_count == 0 && running) {
            pthread_cond_wait(&buffer_not_empty, &buffer_mutex);
        }

        //vê se a thread deve encerrar
        if (!running) {
            pthread_mutex_unlock(&buffer_mutex);
            break;
        }

        //tira uma tarefa do buffer
        Task task = task_buffer[buffer_front];
        buffer_front = (buffer_front + 1) % BUFFER_SIZE;
        buffer_count--;

        pthread_cond_signal(&buffer_not_full);
        pthread_mutex_unlock(&buffer_mutex);

        //aguarda até que haja uma thread disponível
        pthread_mutex_lock(&active_threads_mutex);
        while (active_threads >= N) {
            pthread_cond_wait(&thread_available, &active_threads_mutex);
        }
        active_threads++;
        pthread_mutex_unlock(&active_threads_mutex);

        //cria uma thread para processar a tarefa
        Task *task_copy = malloc(sizeof(Task)); //aloca memória para copiar a tarefa
        *task_copy = task;

        pthread_t thread;
        pthread_create(&thread, NULL, funexec, task_copy);
        pthread_detach(thread); //faz com que a thread seja gerenciada automaticamente
    }
    return NULL;
}

//agenda uma nova tarefa
void agendarExecucao(void *(*funexec)(void *), void *arg, int *id) {
    pthread_mutex_lock(&buffer_mutex);
    //aguarda enquanto o buffer estiver cheio
    while (buffer_count == BUFFER_SIZE) {
        pthread_cond_wait(&buffer_not_full, &buffer_mutex);
    }
    //adiciona a tarefa ao buffer
    Task task;
    task.id = next_task_id++;
    task.funexec = funexec;
    task.arg = arg;

    task_buffer[buffer_rear] = task;
    buffer_rear = (buffer_rear + 1) % BUFFER_SIZE;
    buffer_count++;

    *id = task.id;

    pthread_cond_signal(&buffer_not_empty); //"acorda" o despachante já que o buffer não está vazio
    pthread_mutex_unlock(&buffer_mutex); //libera a região crítica
}

//obtém o resultado de uma tarefa
void *pegarResultadoExecucao(int id) {
    pthread_mutex_lock(&result_mutex);

    //aguarda até que o resultado esteja disponível
    while (1) {
        for (int i = 0; i < result_count; i++) {
            if (result_buffer[i].id == id) {
                void *result = result_buffer[i].result;
                //remove o resultado do buffer
                for (int j = i; j < result_count - 1; j++) {
                    result_buffer[j] = result_buffer[j + 1];
                }
                result_count--;
                pthread_mutex_unlock(&result_mutex);
                return result;
            }
        }
        pthread_cond_wait(&result_available, &result_mutex); //espera até que um resultado esteja disponível
    }
}

//função de execução: executa uma tarefa
void *funexec(void *arg) {
    Task *task = (Task *)arg;
    void *result = task->funexec(task->arg); //executa a função associada à tarefa

    pthread_mutex_lock(&result_mutex);
    Result res;
    res.id = task->id;
    res.result = result;

    result_buffer[result_count++] = res; //guarda o resultado
    pthread_cond_signal(&result_available);
    pthread_mutex_unlock(&result_mutex);

    pthread_mutex_lock(&active_threads_mutex);
    active_threads--; //libera uma thread
    pthread_cond_signal(&thread_available);
    pthread_mutex_unlock(&active_threads_mutex);

    free(task); //libera a memória alocada para a tarefa
    return NULL;
}

//função exemplo para execução concorrente - utilizaremos uma lista com 40 tarefas
void *example_function(void *arg) {
    int *num = (int *)arg;
    printf("Processando tarefa: %d\n", *num);
    sleep(2); // Simula trabalho
    printf("Tarefa %d concluída!\n", *num);
    return arg;
}

int main() {
    pthread_t dispatcher;
    pthread_create(&dispatcher, NULL, dispatcher_thread, NULL); //associamos a thread despachante a função disptcher_thread e a partir daqui ela atuará de forma independente, gerenciando as execuções e monitorando o buffer de tarefas
    int ids[40];

    //agendar a execução das 40 tarefas
    for (int i = 0; i < 40; i++) {
        int *arg = malloc(sizeof(int));
        *arg = i + 1;
        agendarExecucao(example_function, arg, &ids[i]);
        printf("Tarefa %d agendada.\n", ids[i]);
    }

    //pega o resoltado do buffer de resultados e exibe pro usuário
    for (int i = 0; i < 40; i++) {
        int *result = (int *)pegarResultadoExecucao(ids[i]);
        printf("Resultado da tarefa %d: %d\n", ids[i], *result);
        free(result);
    }

    pthread_mutex_lock(&buffer_mutex); //bloqueia pra garantir que running seja alterada sem interferências
    running = 0; //para de executar a thread despachante
    pthread_cond_signal(&buffer_not_empty); //acorda o despachante para que, se ela estivesse dormindo, visse o running 0 e encerrasse
    pthread_mutex_unlock(&buffer_mutex); //libera o mutex

    pthread_join(dispatcher, NULL); //aguarda a finalização da thread despachante pra encerrar tudo no return

    return 0;
}
