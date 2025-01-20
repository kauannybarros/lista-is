#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>

typedef struct dadostread
{
    const char *palavra;
    const char *nomearquivo;
} dadostread;
// execução das trheads
int busca_palavra(void *arg)
{ // seta os valores para a trhead procurar no arquivo
    dadostread *dados = (dadostread *)arg;
    const char *palavra = dados->palavra;
    const char *nome_arquivo = dados->nomearquivo;
    // abertura dos arquivos
    FILE *arquivo = fopen(nome_arquivo, "r");
    if (arquivo == NULL)
    {
        printf("erro ao abrir o arquivo: %s\n", nome_arquivo);
        return -1;
    }
    // defino o valor de linhas maximo bem como o numero atual da linha que a trhead esta procurando
    char linhas[1024];
    int numeroatual = 0;
    while (fgets(linhas, sizeof(linhas), arquivo)) // loop de procura usando fgets para olhar linha por linha do arquivo
    {
        numeroatual++;               // se nao achou na linha 1, vai na 2 etc...
        if (strstr(linhas, palavra)) // quando achar printa o nome e o numero atual do arquivo
        {                            // Verifica se a palavra está na linha
            printf("<%s>: (%d)\n", nome_arquivo, numeroatual);
        }
    }

    fclose(arquivo);
    return 0;
}

int main(int argc, char const *argv[])
{ // setei um valor maximo de arquivos, porem pode ser modificado, desde que tenha mais arquivos na pasta
    int quantidade = 10;
    thrd_t threads[quantidade];
    dadostread dados[quantidade];
    char palavra[10] = "cavalo"; // modifique esse nome caso queria procurar outra palavra

    // Array de nomes de arquivos
    // caso queira mais casos testes aumente o valor do vetor bem como adapte os nomes dos arquivos
    const char *nomearquivos[11] = {
        "arquivo_1.txt", "arquivo_2.txt", "arquivo_3.txt", "arquivo_4.txt", "arquivo_5.txt",
        "arquivo_6.txt", "arquivo_7.txt", "arquivo_8.txt", "arquivo_9.txt", "arquivo_10.txt"};
    // for de criação das trheads
    for (int i = 0; i < quantidade; i++)
    { // estou passando aq a palavra que vai buscar para o dado da tread especifica, bem como o nome do seu arquivo em especifico
        dados[i].palavra = palavra;
        dados[i].nomearquivo = nomearquivos[i];

        // Cria a thread
        if (thrd_create(&threads[i], busca_palavra, &dados[i]) != thrd_success)
        {
            fprintf(stderr, "Erro ao criar a thread para o arquivo: %s\n", nomearquivos[i]);
            return 1;
        }
    }

    // Aguarda todas as threads terminarem
    for (int i = 0; i < quantidade; i++)
    {
        thrd_join(threads[i], NULL);
    }

    return 0;
}
