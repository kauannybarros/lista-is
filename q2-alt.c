/*
Como dito no enunciado: o código abrirá um jogo base de um arquivo q3-aux.txt e usará pthreads para verificar quem ganhou no caso exemplo.

Após isso, será perguntado ao jogador se ele quer jogar de interativa. Caso [y], o jogador segue para um jogo da velha convencional. Caso [n], o programa encerra o jogo.

Para dar dinamicidade ao código, ele foi feito para ser jogado por 2 pessoas de forma interativa -
como um jogo da velha real com o uso de pthreads para ordenar todo o processo.
*/

#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>

#define N 3 

char tabuleiro[N][N];
int vencedor = 0; //1 para player 1, 2 para player 2, 0 para empate
int jogadas = 0; 

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

//inicializa o tabuleiro a partir de um arquivo exemplo
void inicializar_tabuleiro(char *nome_arquivo) {
    FILE *arquivo = fopen("q3-aux.txt", "r");
    if (arquivo == NULL) {
        printf("Erro ao abrir o arquivo %s. Certifique-se de que ele existe.\n", nome_arquivo);
        exit(1);
    }

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            fscanf(arquivo, " %c", &tabuleiro[i][j]); //lê o tabuleiro do arquivo
            if (tabuleiro[i][j] != ' ') {
                jogadas++; //conta as jogadas iniciais
            }
        }
    }
    fclose(arquivo);
}

//exibe o tabuleiro na tela a cada jogada
void mostra_tabuleiro() {
    printf("\n");
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            printf(" %c ", tabuleiro[i][j]);
            if (j < N - 1) printf("|");
        }
        printf("\n");
        if (i < N - 1) printf("---+---+---\n");
    }
    printf("\n");
}

//verifica as linhas do tabuleiro utilizando uma thread pra checar a cada rodada
void* checa_linha(void* arg) { 
    for (int i = 0; i < N; i++) {
        if (vencedor != 0) pthread_exit(NULL); //encerra caso já exista um vencedor
        if (tabuleiro[i][0] != ' ' && tabuleiro[i][0] == tabuleiro[i][1] && tabuleiro[i][1] == tabuleiro[i][2]) {
            pthread_mutex_lock(&mutex); //bloqueia a entrada no mutex por outras threads, do pthread_m_lock até o pthread_m_unlock nos temos a chamada região crítica, nesse caso: a atribuição do vencedor
            if (vencedor == 0) {
                vencedor = (tabuleiro[i][0] == 'X') ? 1 : 2;
            }
            pthread_mutex_unlock(&mutex);
            pthread_exit(NULL); //termina a thread de maneira controlada
        }
    }
    pthread_exit(NULL);
}

//verifica as colunas do tabuleiro utilizando uma thread pra checar a cada rodada
void* checa_coluna(void* arg) {
    for (int i = 0; i < N; i++) {
        if (vencedor != 0) pthread_exit(NULL); //encerra caso já exista um vencedor
        if (tabuleiro[0][i] != ' ' && tabuleiro[0][i] == tabuleiro[1][i] && tabuleiro[1][i] == tabuleiro[2][i]) {
            pthread_mutex_lock(&mutex); //bloqueia a entrada no mutex por outras threads, do pthread_m_lock até o pthread_m_unlock nos temos a chamada região crítica, nesse caso: a atribuição do vencedor
            if (vencedor == 0) {
                vencedor = (tabuleiro[0][i] == 'X') ? 1 : 2;
            }
            pthread_mutex_unlock(&mutex);
            pthread_exit(NULL); //termina a thread de maneira controlada
        }
    }
    pthread_exit(NULL);
}

//verifica as diagonais do tabuleiro utilizando uma thread pra checar a cada rodada
void* checa_diagonal(void* arg) {
    if (vencedor != 0) pthread_exit(NULL); //encerra caso já exista um vencedor
    if (tabuleiro[1][1] != ' ' && ((tabuleiro[0][0] == tabuleiro[1][1] && tabuleiro[1][1] == tabuleiro[2][2]) || (tabuleiro[0][2] == tabuleiro[1][1] && tabuleiro[1][1] == tabuleiro[2][0]))) {
        pthread_mutex_lock(&mutex); //bloqueia a entrada no mutex por outras threads, do pthread_m_lock até o pthread_m_unlock nos temos a chamada região crítica, nesse caso: a atribuição do vencedor
        if (vencedor == 0) {
            vencedor = (tabuleiro[1][1] == 'X') ? 1 : 2;
        }
        pthread_mutex_unlock(&mutex);
        pthread_exit(NULL); //termina a thread de maneira controlada
    }        
    pthread_exit(NULL);
}

//função que chamamos na main pra verificar o tabuleiro a cada rodada(é meio que a função "principal" da main)
void checa_ganhou() {
    pthread_t threads[3]; //cria as threads
    pthread_create(&threads[0], NULL, checa_linha, NULL); 
    pthread_create(&threads[1], NULL, checa_coluna, NULL);
    pthread_create(&threads[2], NULL, checa_diagonal, NULL);
    for (int i = 0; i < 3; i++) {
        pthread_join(threads[i], NULL); //faz com que as threads aguardem o fim das outras
    }
}

//pede a entrada aos jogadores
void jogando(int jogador) {
    int linha, coluna;
    char simbolo = (jogador == 1) ? 'X' : 'O';
    while (1) {
        printf("Jogador %d (%c), insira linha e coluna (de 0 a 2): ", jogador, simbolo);
        scanf("%d %d", &linha, &coluna);
        if (linha >= 0 && linha < N && coluna >= 0 && coluna < N && tabuleiro[linha][coluna] == ' ') {
            tabuleiro[linha][coluna] = simbolo; //insere o símbolo no tabuleiro
            jogadas++;
            break;
        } else {
            printf("Movimento inválido. Tente novamente. \n");
        }
    }
}

int main() {
    char continuar;
    inicializar_tabuleiro("caso_exemplo.txt"); //inicializa o tabuleiro a partir do arquivo exemplo

    printf("Tabuleiro inicializado com o caso exemplo:\n");
    mostra_tabuleiro(); //mostra o tabuleiro inicializado
    checa_ganhou(); //verifica se há um vencedor no caso exemplo

    if (vencedor != 0) {
        printf("O jogador %d venceu no caso exemplo!\n", vencedor);
    } else {
        printf("Nenhum vencedor no caso exemplo.\n");
    }

    printf("Deseja continuar jogando? (y/n): ");
    scanf(" %c", &continuar);

    if (continuar == 'y' || continuar == 'Y') {
        int jogador_da_vez = (jogadas % 2 == 0) ? 1 : 2;
        while (vencedor == 0 && jogadas < N * N) {
            mostra_tabuleiro();
            jogando(jogador_da_vez);
            checa_ganhou();
            if (vencedor == 0) {
                jogador_da_vez = (jogador_da_vez == 1) ? 2 : 1;
            }
        }
        mostra_tabuleiro();
        if (vencedor == 1) {
            printf("O jogador 1 venceu!\n");
        } else if (vencedor == 2) {
            printf("O jogador 2 venceu!\n");
        } else {
            printf("Deu velha!\n");
        }
    }

    pthread_mutex_destroy(&mutex); //libera os recursos do mutex
    return 0;
}
