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
    int id; // Adiciona um identificador para a fila
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

BlockingQueue *newBlockingQueue(unsigned int sizeBuffer, int id)
{
    BlockingQueue *Q = (BlockingQueue *)malloc(sizeof(BlockingQueue));
    if (Q == NULL)
    {
        printf("Erro ao alocar memoria para a fila\n");
        exit(1);
    }
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

void putBlockingQueue(BlockingQueue *Q, int valor)
{
    pthread_mutex_lock(&Q->mutex);
    while (Q->statusBuffer == Q->sizeBuffer)
    {
        printf("Fila cheia\n");
        pthread_cond_wait(&Q->full, &Q->mutex);
    }

    Elem *NovoElemento = (Elem *)malloc(sizeof(Elem));
    if (NovoElemento == NULL)
    {
        printf("Erro ao alocar memoria para o novo elemento\n");
        exit(1);
    }
    NovoElemento->value = valor;
    NovoElemento->prox = NULL;

    if (Q->last == NULL)
    {
        Q->head = NovoElemento;
    }
    else
    {
        Q->last->prox = NovoElemento;
    }
    Q->last = NovoElemento;
    Q->statusBuffer++;

    pthread_cond_signal(&Q->empty);
    pthread_mutex_unlock(&Q->mutex);
}

int takeBlockingQueue(BlockingQueue *Q)
{
    pthread_mutex_lock(&Q->mutex);
    while (Q->statusBuffer == 0)
    {
        printf("Fila vazia\n");
        pthread_cond_wait(&Q->empty, &Q->mutex);
    }

    Elem *ElementoRemovido = Q->head;
    int valor = ElementoRemovido->value;
    Q->head = Q->head->prox;
    if (Q->head == NULL)
    {
        Q->last = NULL;
    }
    free(ElementoRemovido);
    Q->statusBuffer--;

    pthread_cond_signal(&Q->full);
    pthread_mutex_unlock(&Q->mutex);

    return valor;
}

// Função de produtor
void *producer(void *args)
{
    struct producerArgs *prodArgs = (struct producerArgs *)args;
    BlockingQueue *Q = prodArgs->Q;
    int id = prodArgs->id;
    int item = 0;

    while (1)
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
    struct consumerArgs *consArgs = (struct consumerArgs *)args;
    BlockingQueue *Q = consArgs->Q;
    int id = consArgs->id;

    while (1)
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
    int filas = 0;
    printf("Quantas filas deseja criar?");
    scanf("%d", &filas);
    BlockingQueue *FILA = (BlockingQueue *)malloc(filas * sizeof(BlockingQueue *));
    for (int i = 0; i < filas; i++)
    {
        printf("qual o buffer da fila %d", i);
        int quantidade = 0;
        scanf("%d", &quantidade);
        FILA[i] = newBlockingQueue(quantidade, i); // Define o identificador da fila
    }

    int prodq = 0, consq = 0;
    printf("Insira a quantidade de produtores e consumidores:");
    scanf("%d %d", &prodq, &consq);

    pthread_t *prod = (pthread_t *)malloc(prodq * sizeof(pthread_t));
    pthread_t *cons = (pthread_t *)malloc(consq * sizeof(pthread_t));
    prodArgs = (struct producerArgs *)malloc(prodq * sizeof(struct producerArgs));
    consArgs = (struct consumerArgs *)malloc(consq * sizeof(struct consumerArgs));

    for (int i = 0; i < prodq; i++)
    {
        prodArgs[i].Q = FILA[i % filas];
        prodArgs[i].id = i;
        pthread_create(&prod[i], NULL, producer, &prodArgs[i]);
    }

    for (int i = 0; i < consq; i++)
    {
        consArgs[i].Q = FILA[i % filas];
        consArgs[i].id = i;
        pthread_create(&cons[i], NULL, consumer, &consArgs[i]);
    }

    for (int i = 0; i < prodq; i++)
    {
        pthread_join(prod[i], NULL);
    }

    for (int i = 0; i < consq; i++)
    {
        pthread_join(cons[i], NULL);
    }

    for (int i = 0; i < filas; i++)
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
