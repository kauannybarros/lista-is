#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h> 

#define num_reg_critica 10 //Tamanho do array (regiao critica)
#define N 5 //Threads leitoras
#define M 3 //Threads escritoras

//para haver uma alternancia mais justa entre leitores/escritores, foi feito um controle parecido com a questao 3
int estado = 0; // 0 = leitores, 1 = escritores
int reg_critica[num_reg_critica]; // Regiao critica
pthread_mutex_t mutex;      
pthread_cond_t ler;     //variavel de condicao para as threads leitoras
pthread_cond_t escrever;    //variavel para as threads escritoras

int cont_leitoras = 0;       //contador de threads leitoras ativas
int cont_escritoras = 0;       //contador pra ver se tem uma thread escritora ativa (se tiver, vai bloquear o resto)

void *leitora(void *arg) {
    int id_leitor = *(int *)arg;

    while (1) {

        pthread_mutex_lock(&mutex); //bloqueia a regiao critica pra nao ocorrer de outra leitora acabar modificando o contador ao mesmo tempo (condicao de disputa)

        //espera se tem escritoras ativas ou se for a vez dos escritores
        while (cont_escritoras > 0 || estado == 1) {
            pthread_cond_wait(&ler, &mutex);
        }

        cont_leitoras++; //atualiza o contador das leitoras ativas

        pthread_mutex_unlock(&mutex);

        printf("Leitora %d lendo: ", id_leitor);
        for (int i = 0; i < num_reg_critica; i++) {
            printf("%d ", reg_critica[i]);
        }
        printf("\n");

        pthread_mutex_lock(&mutex);
        cont_leitoras--;

    //se acabou as leitoras, sinaliza pra escritora   
        if (cont_leitoras == 0) {
            estado = 1; // Próxima vez é dos escritores
            pthread_cond_signal(&escrever);
        }

        pthread_mutex_unlock(&mutex);

        sleep(1); //simula tempo de ler
    }

    free(arg);
    return NULL; //terminou a thread
}

void *escritora(void *arg) {
    int id = *(int *)arg;

    while (1) {
        pthread_mutex_lock(&mutex);

        //se tiver leitora ativa ou escritora ativa espera
        while (cont_leitoras > 0 || cont_escritoras > 0 || estado == 0) {
            pthread_cond_wait(&escrever, &mutex);
        }

        cont_escritoras = 1; //avisa que tem uma escritora ativa

        pthread_mutex_unlock(&mutex);

        //escreve na regiao critica, como o contador de escritoras foi ativado, nenhuma outra thread vai conseguir acessar ao mesmo tempo
        printf("Escritora %d escrevendo\n", id);
        for (int i = 0; i < num_reg_critica; i++) {
            reg_critica[i] = id;
        }

        pthread_mutex_lock(&mutex);
        cont_escritoras = 0;

        //alterna para leitores
        estado = 0;
        pthread_cond_broadcast(&ler);
        pthread_cond_signal(&escrever);

        pthread_mutex_unlock(&mutex);

        sleep(4); //simula o tempo de escrever
    }

    free(arg);
    return NULL;
}

int main() {
    pthread_t leitoras[N], escritoras[M];

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&ler, NULL);
    pthread_cond_init(&escrever, NULL);

    //inicializa o array da reg critica como 0
    for (int i = 0; i < num_reg_critica; i++) {
        reg_critica[i] = 0;
    }

    //threads leitoras
    for (int i = 0; i < N; i++) {
        int *id = malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&leitoras[i], NULL, leitora, id);
    }

    //threads escritoras
    for (int i = 0; i < M; i++) {
        int *id = malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&escritoras[i], NULL, escritora, id);
    }

    for (int i = 0; i < N; i++) {
        pthread_join(leitoras[i], NULL);
    }

    for (int i = 0; i < M; i++) {
        pthread_join(escritoras[i], NULL);
    }

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&ler);
    pthread_cond_destroy(&escrever);

    return 0;
}
