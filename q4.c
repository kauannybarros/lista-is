#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct elem
{
    int value;
    struct elem *prox;
} Elem;

typedef struct blockingQueue
{
    unsigned int sizeBuffer, statusBuffer;
    Elem *head, *last;
    pthread_mutex_t mutex;
    pthread_cond_t empty;
    pthread_cond_t full;
    int id; // Identificador da fila
} BlockingQueue;

struct producerArgs
{
    BlockingQueue *Q;
    int id;
};

struct consumerArgs
{
    BlockingQueue *Q;
    int id;
};

struct producerArgs *prodArgs;
struct consumerArgs *consArgs;
// Função de criação da fila
BlockingQueue *newBlockingQueue(unsigned int sizeBuffer, int id)
{
    BlockingQueue *Q = (BlockingQueue *)malloc(sizeof(BlockingQueue)); // Aloca memória para a fila
    if (Q == NULL)
    {
        printf("Erro ao alocar memoria para a fila\n");
        exit(1);
    }
    // inicialização dos parametros
    Q->sizeBuffer = sizeBuffer;
    Q->statusBuffer = 0;
    Q->head = NULL;
    Q->last = NULL;
    Q->id = id; // Define o identificador da fila
    pthread_mutex_init(&Q->mutex, NULL);
    pthread_cond_init(&Q->empty, NULL);
    pthread_cond_init(&Q->full, NULL);
    return Q;
}
// Função de inserir na fila(produtor)
void putBlockingQueue(BlockingQueue *Q, int valor)
{
    pthread_mutex_lock(&Q->mutex);           // Bloqueia o mutex
    while (Q->statusBuffer == Q->sizeBuffer) // Verifica se a fila está cheia
    {
        printf("Fila cheia\n");
        pthread_cond_wait(&Q->full, &Q->mutex);
    }

    Elem *NovoElemento = (Elem *)malloc(sizeof(Elem)); // Aloca memória para o novo elemento
    if (NovoElemento == NULL)
    {
        printf("Erro ao alocar memoria para o novo elemento\n");
        exit(1);
    }
    NovoElemento->value = valor; // Define o valor do novo elemento
    NovoElemento->prox = NULL;   // Define o próximo como nulo

    if (Q->last == NULL) // Verifica se a fila está vazia
    {
        Q->head = NovoElemento; // Atualiza o head se tiver vazia
    }
    else // Atualiza o último elemento se n tiver vazia
    {
        Q->last->prox = NovoElemento; // Atualiza o último elemento
    }
    Q->last = NovoElemento;
    Q->statusBuffer++;

    pthread_cond_signal(&Q->empty);
    pthread_mutex_unlock(&Q->mutex);
}

int takeBlockingQueue(BlockingQueue *Q) // Função de retirar da fila(consumidor)
{
    pthread_mutex_lock(&Q->mutex); // Bloqueia o mutex
    while (Q->statusBuffer == 0)   // Verifica se a fila está vazia
    {
        printf("Fila vazia\n");
        pthread_cond_wait(&Q->empty, &Q->mutex);
    }
    // Remove o elemento da fila
    Elem *ElementoRemovido = Q->head;
    int valor = ElementoRemovido->value; // Recebe o valor do elemento
    Q->head = Q->head->prox;             // Atualiza o head
    if (Q->head == NULL)
    {
        Q->last = NULL;
    }
    free(ElementoRemovido); // Libera a memória do elemento
    Q->statusBuffer--;

    pthread_cond_signal(&Q->full);
    pthread_mutex_unlock(&Q->mutex);

    return valor;
}

// Função de produtor
void *producer(void *args)
{
    struct producerArgs *prodArgs = (struct producerArgs *)args; // Recebe os argumentos
    BlockingQueue *Q = prodArgs->Q;                              // Recebe a fila
    int id = prodArgs->id;                                       // Recebe o identificador
    int item = 0;

    while (1) // loop infinito como foi pedido na questao para o produtor trabalhar
    {
        putBlockingQueue(Q, item);
        printf("Produtor %d produziu: %d na fila %d\n", id, item, Q->id);
        item++;
        // Simula tempo de produção
        sleep(1);
    }

    pthread_exit(NULL);
}

// Função de consumidor
void *consumer(void *args)
{
    struct consumerArgs *consArgs = (struct consumerArgs *)args; // Recebe os argumentos
    BlockingQueue *Q = consArgs->Q;                              // Recebe a fila
    int id = consArgs->id;                                       // Recebe o identificador

    while (1) // Loop infinito como foi pedido na questao para o produtor trabalhar
    {
        int item = takeBlockingQueue(Q);
        printf("Consumidor %d consumiu: %d da fila %d\n", id, item, Q->id);
        // Simula tempo variável de consumo
        sleep(1);
    }

    pthread_exit(NULL);
}

int main(int argc, char const *argv[])
{
    int filas = 0; // Define a quantidade de filas
    printf("Quantas filas deseja criar?");
    scanf("%d", &filas);
    BlockingQueue **FILA = (BlockingQueue **)malloc(filas * sizeof(BlockingQueue *));
    for (int i = 0; i < filas; i++) // Cria as filas e define o buffer de cada uma
    {
        printf("qual o buffer da fila %d", i);
        int quantidade = 0;
        scanf("%d", &quantidade);
        FILA[i] = newBlockingQueue(quantidade, i); // Define o identificador da fila
    }

    int prodq = 0, consq = 0; // Define a quantidade de produtores e consumidores
    printf("Insira a quantidade de produtores e consumidores:");
    scanf("%d %d", &prodq, &consq);
    // Cria os vetores de threads para produtores e consumidores
    pthread_t *prod = (pthread_t *)malloc(prodq * sizeof(pthread_t));
    pthread_t *cons = (pthread_t *)malloc(consq * sizeof(pthread_t));
    prodArgs = (struct producerArgs *)malloc(prodq * sizeof(struct producerArgs));
    consArgs = (struct consumerArgs *)malloc(consq * sizeof(struct consumerArgs));

    for (int i = 0; i < prodq; i++) // Cria as threads de produtores
    {
        prodArgs[i].Q = FILA[i % filas];
        prodArgs[i].id = i;
        pthread_create(&prod[i], NULL, producer, &prodArgs[i]);
    }

    for (int i = 0; i < consq; i++) // Cria as threads de consumidores
    {
        consArgs[i].Q = FILA[i % filas];
        consArgs[i].id = i;
        pthread_create(&cons[i], NULL, consumer, &consArgs[i]);
    }

    for (int i = 0; i < prodq; i++) //   Espera as threads de produtores terminarem
    {
        pthread_join(prod[i], NULL);
    }

    for (int i = 0; i < consq; i++) // Espera as threads de consumidores terminarem
    {
        pthread_join(cons[i], NULL);
    }

    for (int i = 0; i < filas; i++) // Libera a memória das filas
    {
        pthread_cond_destroy(&FILA[i]->empty);
        pthread_cond_destroy(&FILA[i]->full);
        free(FILA[i]);
    }
    free(FILA);
    free(prod);
    free(cons);
    free(prodArgs);
    free(consArgs);

    return 0;
}
