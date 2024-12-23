#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#define MAX_LINE_LENGTH 1024
#define BLOCK_SIZE 100000
#define HASH_SIZE 100003

//Tamanho da tabela hash de sessões
#define TAMANHO_HASH_SESSAO 109951203

#define STRING_LEN 45
#define HASH_SIZE_BUSCA 101

//Ordem da árvore B+
#define ORDEM 5

typedef struct BPlusNode {
    bool folha;
    int numChaves;
    long long chaves[ORDEM];
    long long offsets[ORDEM];
    struct BPlusNode *filhos[ORDEM + 1];
    struct BPlusNode *prox;
} BPlusNode;

typedef struct {
    BPlusNode *raiz;
} BPlusTree;

typedef struct {
    bool removido;
    int id;
    float preco;
    char marca[30];
    char categoria[45];
} Produto;

typedef struct {
    long long id;
    int id_user;
    char sessao[100];
    char data_acesso[24];
    char categoria[45];
    char type[10];
} Acesso;

typedef struct {
    long long id, offset;
} Indice;

// Estrutura de um nó da tabela hash de produtos
typedef struct _hashNode {
    int id;
    struct _hashNode *next;
} HashNode;

// Estrutura de um nó da tabela hash de marcas
typedef struct _stringNode {
    char nome[STRING_LEN];
    int quantidade;
    struct _stringNode *next;
} StringNode;

//Tabela hash para armazenamento de produtos
typedef struct {
    StringNode *tabela[HASH_SIZE_BUSCA];
} TabelaHash;

//Nodo da tabela hash de sessões
typedef struct HashSessaoNode {
    char sessao[100];       // Sessão indexada
    long long offset;       // Offset do registro no arquivo
    struct HashSessaoNode *prox;  // Próximo nó na lista para resolver colisões
} HashSessaoNode;

//Tabela hash de sessões
typedef struct {
    HashSessaoNode *tabela[TAMANHO_HASH_SESSAO];
} HashSessaoTable;

//Função para imprimir os acessos
void printarAcesso(Acesso acesso) {
    printf("ID: %lld | User_ID: %d | Tipo: %s | Sessao: %s | Data: %s\n", 
            acesso.id, acesso.id_user, acesso.type, acesso.sessao, acesso.data_acesso);
}

//Função para mostrar um produto na tela
void printarProduto(Produto produto) {
    printf("ID: %d | Marca: %s | Categoria: %s | Preco: %.2f\n", 
    produto.id, produto.marca, produto.categoria, produto.preco);
}

//Função para inicializar a tabela hash de sessões
HashSessaoTable *inicializarTabelaHashSessao() {
    HashSessaoTable *tabela = malloc(sizeof(HashSessaoTable));
    for (int i = 0; i < TAMANHO_HASH_SESSAO; i++)
        tabela->tabela[i] = NULL;
    return tabela;
}

//Função do cálculo hash para uma chave de sessão
unsigned int hashSessao(const char *sessao) {
    unsigned int hash = 0;
    for (int i = 0; sessao[i] != '\0'; i++)
        hash = (hash * 31 + sessao[i]) % TAMANHO_HASH_SESSAO;
    return hash;
}

//Função para inserir uma sessão dentro da tabela hash
void inserirHashSessao(HashSessaoTable *tabela, const char *sessao, long long offset) {
    unsigned int indice = hashSessao(sessao);

    HashSessaoNode *novo = malloc(sizeof(HashSessaoNode));
    strcpy(novo->sessao, sessao);
    novo->offset = offset;
    novo->prox = tabela->tabela[indice];

    // Inserir no início da lista encadeada
    tabela->tabela[indice] = novo;
}

//Função para buscar uma lista dentro da tabela hash de sessões
long long buscarHashSessao(HashSessaoTable *tabela, const char *sessao) {
    clock_t inicio_exec, fim_exec;
    double tempo_gasto;

    inicio_exec = clock();
    unsigned int indice = hashSessao(sessao);

    HashSessaoNode *atual = tabela->tabela[indice];
    HashSessaoNode *before = atual;
    while (atual) {
        if (strcmp(atual->sessao, sessao) == 0) {
            FILE *arquivoAcessos = fopen("acessos.dat", "rb");
            if (!arquivoAcessos) {
                printf("Erro ao abrir o arquivo de acessos.\n");
                return -1;
            }

            if (fseek(arquivoAcessos, atual->offset, SEEK_SET) != 0) {
                printf("Erro ao posicionar o ponteiro no offset.\n");
                fclose(arquivoAcessos);
                return -1;
            }

            Acesso acesso;
            if (fread(&acesso, sizeof(Acesso), 1, arquivoAcessos) != 1) {
                printf("Erro ao ler o registro de acesso.\n");
                fclose(arquivoAcessos);
                return -1;
            }

            printarAcesso(acesso);
            fclose(arquivoAcessos);
            fim_exec = clock();
            tempo_gasto = ((double)(fim_exec - inicio_exec)) / CLOCKS_PER_SEC;
            printf("A funcao demorou %f segundos para executar.\n", tempo_gasto);
        }
        before = atual;
        atual = atual->prox;
    }

    if (before != atual)
        return before->offset;

    return -1; // Não encontrado
}

//Função para criar os índices da tabela de acessos em uma tabela hash, em cima da coluna sessão
void criarIndiceHashSessao(HashSessaoTable *tabela) {
    if (!tabela) {
        printf("Erro ao alocar memória para a tabela hash.\n");
        return;
    }

    clock_t inicio, fim;
    double tempo_gasto;

    inicio = clock();

    FILE *arquivoAcessos = fopen("acessos.dat", "rb");

    if (!arquivoAcessos) {
        printf("Erro ao abrir o arquivo de acessos.\n");
        return;
    }

    Acesso acesso;
    long long offset;

    fseek(arquivoAcessos, 0, SEEK_SET);

    while (fread(&acesso, sizeof(Acesso), 1, arquivoAcessos)) {
        offset = ftell(arquivoAcessos) - sizeof(Acesso);
        inserirHashSessao(tabela, acesso.sessao, offset);
    }

    printf("Indice hash criado com sucesso.\n");

    fim = clock();
    tempo_gasto = ((double)(fim - inicio)) / CLOCKS_PER_SEC;
    printf("A funcao demorou %f segundos para executar.\n", tempo_gasto);

    fclose(arquivoAcessos);
}

//Função para buscar um acesso na tabela hash por meio da sessão
Acesso buscarAcessoPorSessao(HashSessaoTable *tabela, const char *sessao) {
    long long offset = buscarHashSessao(tabela, sessao);

    if (offset != -1) {
        FILE *arquivoAcessos = fopen("acessos.dat", "rb");
        Acesso acesso;

        fseek(arquivoAcessos, offset, SEEK_SET);
        fread(&acesso, sizeof(Acesso), 1, arquivoAcessos);
        fclose(arquivoAcessos);

        return acesso;
    }

    printf("Sessão não encontrada.\n");
    Acesso vazio = {0};
    return vazio;
}

//Função para inicializar a árvore B+
BPlusTree *inicializarArvore() {
    BPlusTree *arvore = malloc(sizeof(BPlusTree));
    arvore->raiz = NULL;
    return arvore;
}

//Função para buscar um nodo na árvore B+
BPlusNode *buscarNodo(BPlusTree *arvore, long long chave) {
    BPlusNode *atual = arvore->raiz;
    while (atual && !atual->folha) {
        int i = 0;
        while (i < atual->numChaves && chave >= atual->chaves[i]) i++;
        atual = atual->filhos[i];
    }
    return atual; // Retorna a folha que pode conter a chave
}

//Função para inserir um novo nó folha na árvore B+
void inserirNoFolha(BPlusNode *folha, long long chave, long long offset) {
    int i = 0;
    while (i < folha->numChaves && chave > folha->chaves[i]) i++;

    for (int j = folha->numChaves; j > i; j--) {
        folha->chaves[j] = folha->chaves[j - 1];
        folha->offsets[j] = folha->offsets[j - 1];
    }

    folha->chaves[i] = chave;
    folha->offsets[i] = offset;
    folha->numChaves++;
}

//Função para dividir um nó da árvore B+
BPlusNode *dividirNo(BPlusNode *no, long long *chavePromovida) {
    int meio = ORDEM / 2;

    // Criar novo nó para armazenar metade das chaves
    BPlusNode *novo = malloc(sizeof(BPlusNode));
    novo->folha = no->folha;
    novo->numChaves = no->folha ? ORDEM - meio : ORDEM - meio - 1;

    // Transferir metade das chaves para o novo nó
    for (int i = 0; i < novo->numChaves; i++) {
        novo->chaves[i] = no->chaves[meio + i];
        if (no->folha)
            novo->offsets[i] = no->offsets[meio + i];
        else
            novo->filhos[i] = no->filhos[meio + i];
    }

    if (!no->folha)
        novo->filhos[novo->numChaves] = no->filhos[ORDEM];

    no->numChaves = meio;
    *chavePromovida = no->chaves[meio];

    if (no->folha) {
        novo->prox = no->prox;
        no->prox = novo;
    }

    return novo;
}

//Função para a inserção recursiva na árvore B+
void inserirRecursivo(BPlusNode *no, long long chave, long long offset, BPlusNode **novoNo, long long *chavePromovida) {
    int i = no->numChaves;

    if (no->folha) {
        // Inserir na folha
        while (i > 0 && chave < no->chaves[i - 1]) i--;
        inserirNoFolha(no, chave, offset);
    } else {
        // Encontrar filho correto para descer
        while (i > 0 && chave < no->chaves[i - 1]) i--;
        i--;

        BPlusNode *filho = no->filhos[i + 1];
        BPlusNode *novoFilho = NULL;
        long long chaveFilhoPromovida;

        inserirRecursivo(filho, chave, offset, &novoFilho, &chaveFilhoPromovida);

        // Se houve divisão no filho
        if (novoFilho) {
            for (int j = no->numChaves; j > i + 1; j--) {
                no->chaves[j] = no->chaves[j - 1];
                no->filhos[j + 1] = no->filhos[j];
            }

            no->chaves[i + 1] = chaveFilhoPromovida;
            no->filhos[i + 2] = novoFilho;
            no->numChaves++;
        }
    }

    // Dividir nó se necessário
    if (no->numChaves == ORDEM)
        *novoNo = dividirNo(no, chavePromovida);
    else
        *novoNo = NULL;
}

//Função para inserir um índice na árvore B+
void inserirArvore(BPlusTree *arvore, long long chave, long long offset) {
    if (!arvore->raiz) {
        // Criar raiz inicial
        arvore->raiz = malloc(sizeof(BPlusNode));
        arvore->raiz->folha = true;
        arvore->raiz->numChaves = 1;
        arvore->raiz->chaves[0] = chave;
        arvore->raiz->offsets[0] = offset;
        arvore->raiz->prox = NULL;
    } else {
        BPlusNode *novoNo = NULL;
        long long chavePromovida;

        inserirRecursivo(arvore->raiz, chave, offset, &novoNo, &chavePromovida);

        // Se a raiz foi dividida, criar nova raiz
        if (novoNo) {
            BPlusNode *novaRaiz = malloc(sizeof(BPlusNode));
            novaRaiz->folha = false;
            novaRaiz->numChaves = 1;
            novaRaiz->chaves[0] = chavePromovida;
            novaRaiz->filhos[0] = arvore->raiz;
            novaRaiz->filhos[1] = novoNo;

            arvore->raiz = novaRaiz;
        }
    }
}

//Função para criar os índices dos produtos em uma árvore B+
void criarIndiceProdutosComArvore(BPlusTree *arvore) {
    clock_t inicio, fim;
    double tempo_gasto;

    inicio = clock();

    FILE *arquivoProdutos = fopen("produtos.dat", "rb");

    if (!arquivoProdutos) {
        printf("Erro ao abrir o arquivo de produtos.\n");
        return;
    }

    Produto produto;
    long long offset;

    fseek(arquivoProdutos, 0, SEEK_SET);

    int times = 0;
    while (fread(&produto, sizeof(Produto), 1, arquivoProdutos)) {
        offset = ftell(arquivoProdutos) - sizeof(Produto);
        inserirArvore(arvore, produto.id, offset);
        times++;
    }

    printf("Arvore B+ criada com sucesso.\n");

    fim = clock();
    tempo_gasto = ((double)(fim - inicio)) / CLOCKS_PER_SEC;
    printf("A funcao demorou %f segundos para executar.\n", tempo_gasto);

    fclose(arquivoProdutos);
}

//Função para buscar um produto na árvore B+
int buscarProdutoArvore(Produto *retornarProduto, BPlusTree *arvore, long long id) {
    clock_t inicio, fim;
    double tempo_gasto;

    inicio = clock();
    printf("Buscando produto com ID: %lld\n", id);
    BPlusNode *folha = buscarNodo(arvore, id);

    if (folha) {
        for (int i = 0; i < folha->numChaves; i++) {
            if (folha->chaves[i] == id) {

                FILE *arquivoProdutos = fopen("produtos.dat", "rb");
                if (!arquivoProdutos) {
                    printf("Erro ao abrir o arquivo de produtos.\n");
                    return 0;
                }

                Produto produto;
                fseek(arquivoProdutos, folha->offsets[i], SEEK_SET);
                fread(&produto, sizeof(Produto), 1, arquivoProdutos);
                fclose(arquivoProdutos);

                if (produto.removido)
                    break;

                printarProduto(produto);
                *retornarProduto = produto;
                fim = clock();
                tempo_gasto = ((double)(fim - inicio)) / CLOCKS_PER_SEC;
                printf("A funcao de busca em arvore demorou %f segundos para executar.\n", tempo_gasto);
                return 1;
            }
        }
    }

    printf("Produto com ID %lld nao encontrado na arvore.\n", id);
    return 0;
}

// Função para inserir um produto no arquivo
void inserirProduto(FILE *arquivo, Produto produto) {
    fseek(arquivo, 0, SEEK_END);
    fwrite(&produto, sizeof(Produto), 1, arquivo);
}

//Função para criar o arquivo de índices dos produtos
void criarIndiceProdutos() {
    clock_t inicio_exec, fim_exec;
    double tempo_gasto;

    inicio_exec = clock();

    printf("Criando indice dos produtos...\n");
    FILE *arquivoProdutos = fopen("produtos.dat", "rb");
    FILE *arquivoIndiceProdutos = fopen("indice_produtos.dat", "wb");

    if (!arquivoProdutos || !arquivoIndiceProdutos) {
        printf("Erro ao abrir os arquivos.\n");
        return;
    }

    Produto produto;
    Indice indice;
    long long offset;

    fseek(arquivoProdutos, 0, SEEK_SET);

    while (fread(&produto, sizeof(Produto), 1, arquivoProdutos)) {
        indice.id = produto.id;

        offset = ftell(arquivoProdutos) - sizeof(Produto);
        indice.offset = offset;

        fwrite(&indice, sizeof(Indice), 1, arquivoIndiceProdutos);
    }

    fflush(arquivoIndiceProdutos);

    printf("Arquivo de indice dos produtos criado com sucesso.\n");

    fclose(arquivoProdutos);
    fclose(arquivoIndiceProdutos);

    fim_exec = clock();
    tempo_gasto = ((double)(fim_exec - inicio_exec)) / CLOCKS_PER_SEC;
    printf("A funcao de criacao de indice demorou %f segundos para executar.\n", tempo_gasto);
}

//Função para criar o arquivo de índices dos acessos
void criarIndiceAcessos() {
    clock_t inicio, fim;
    double tempo_gasto;

    inicio = clock();
    FILE *arquivoAcessos = fopen("acessos.dat", "rb");
    FILE *arquivoIndiceAcessos = fopen("indice_acessos.dat", "wb");

    if (arquivoAcessos == NULL || arquivoIndiceAcessos == NULL) {
        printf("Erro ao abrir os arquivos.\n");
        return;
    }

    Acesso acesso;
    Indice indice;
    long long offset;

    fseek(arquivoAcessos, 0, SEEK_SET);

    while (fread(&acesso, sizeof(Acesso), 1, arquivoAcessos)) {
        // Salvar o ID do acesso no índice
        indice.id = acesso.id;

        // Calcular o offset atual (posição do registro no arquivo de acessos)
        offset = ftell(arquivoAcessos) - sizeof(Acesso);
        indice.offset = offset;

        // Escrever o índice no arquivo de índices
        fwrite(&indice, sizeof(Indice), 1, arquivoIndiceAcessos);
    }

    fim = clock();

    fflush(arquivoIndiceAcessos);
    
    printf("Arquivo de indice dos acessos criado com sucesso.\n");

    tempo_gasto = ((double)(fim - inicio)) / CLOCKS_PER_SEC;
    printf("A funcao demorou %f segundos para executar.\n", tempo_gasto);

    fclose(arquivoAcessos);
    fclose(arquivoIndiceAcessos);
}

//Função para achar o último id de acesso existente no arquivo de extensão
Acesso pegarUltimoAcessoExtensao() {
    Acesso acesso;
    acesso.id = 0;

    FILE *arquivo = fopen("acessos_extensao.dat", "rb");
    if (!arquivo) return acesso;

    while (fread(&acesso, sizeof(Acesso), 1, arquivo));
    return acesso;
}

//Função para inserir um novo acesso no arquivo de extensão
void inserirAcesso(FILE *arquivo, Acesso acesso) {
    Acesso ultimoAcesso = pegarUltimoAcessoExtensao();
    acesso.id = ultimoAcesso.id + 1;
    fseek(arquivo, 0, SEEK_END);
    fwrite(&acesso, sizeof(Acesso), 1, arquivo);
}

//Função para mesclar os arquivos de extensão e de acesso
void mesclarArquivosAcesso(long long *retornarOffset) {
    printf("Mescando arquivos de acessos... Isso pode demorar um pouco!\n");
    clock_t inicio_exec, fim_exec;
    double tempo_gasto;

    inicio_exec = clock();

    FILE *arquivo = fopen("acessos.dat", "ab+");
    FILE *arquivoExtensao = fopen("acessos_extensao.dat", "rb");
    FILE *arquivoIndice = fopen("indice_acessos.dat", "ab+");

    if (!arquivo || !arquivoExtensao || !arquivoIndice) return;

    fseek(arquivo, 0, SEEK_SET);
    Acesso acesso;
    while (fread(&acesso, sizeof(Acesso), 1, arquivo));

    long long ultimoId = acesso.id;

    fseek(arquivoIndice, 0, SEEK_SET);
    Indice ind;
    while (fread(&ind, sizeof(Indice), 1, arquivoIndice));

    fseek(arquivoExtensao, 0, SEEK_SET);
    
    while (fread(&acesso, sizeof(Acesso), 1, arquivoExtensao)) {
        acesso.id = ultimoId + 1;
        ultimoId++;

        fwrite(&acesso, sizeof(Acesso), 1, arquivo);

        Indice indice;
        long long offset;

        indice.id = acesso.id;

        // Calcular o offset atual (posição do registro no arquivo de produtos)
        offset = ftell(arquivo) - sizeof(Acesso);
        indice.offset = offset;
        *retornarOffset = offset;

        // Escrever o índice no arquivo de índices
        fwrite(&indice, sizeof(Indice), 1, arquivoIndice);
    }

    fclose(arquivo);
    fclose(arquivoExtensao);
    fclose(arquivoIndice);

    printf("Arquivos de acesso mesclados com sucesso.\n");
    remove("acessos_extensao.dat");

    fim_exec = clock();
    tempo_gasto = ((double)(fim_exec - inicio_exec)) / CLOCKS_PER_SEC;
    printf("A funcao de mesclagem demorou %f segundos para executar.\n", tempo_gasto);
}

//Função para criar um acesso quando o usuário busca o produto
void criarAcesso(Produto produto, HashSessaoTable *tabela) {
    FILE *arquivo = fopen("acessos.dat", "rb");

    if (!arquivo) {
        printf("Erro ao abrir o arquivo.\n");
        return;
    }

    FILE *arquivoExtensao = fopen("acessos_extensao.dat", "ab");
    if (!arquivoExtensao)
        arquivoExtensao = fopen("acessos_extensao.dat", "wb");

    time_t tempoAtual;
    time(&tempoAtual);

    // Converter para a hora local
    struct tm *horaLocal = localtime(&tempoAtual);
    char data_acesso[24];
    sprintf(data_acesso, "%04d-%02d-%02d %02d:%02d:%02d UTC",
           horaLocal->tm_year + 1900,
           horaLocal->tm_mon + 1,
           horaLocal->tm_mday,
           horaLocal->tm_hour,
           horaLocal->tm_min,
           horaLocal->tm_sec);
    
    Acesso primeiroAcesso;
    fread(&primeiroAcesso, sizeof(Acesso), 1, arquivo);

    Acesso novoAcesso;
    novoAcesso.id_user = primeiroAcesso.id_user;
    strcpy(novoAcesso.sessao, primeiroAcesso.sessao);
    strcpy(novoAcesso.data_acesso, data_acesso);
    strcpy(novoAcesso.categoria, produto.categoria);
    strcpy(novoAcesso.type, "view");

    inserirAcesso(arquivoExtensao, novoAcesso);

    printf("Um novo acesso foi registrado no arquivo de extensao.\n");

    fclose(arquivoExtensao);

    long long offset;
    mesclarArquivosAcesso(&offset);

    clock_t inicio_exec, fim_exec;
    double tempo_gasto;

    inicio_exec = clock();
    inserirHashSessao(tabela, novoAcesso.sessao, offset);

    fim_exec = clock();
    tempo_gasto = ((double)(fim_exec - inicio_exec)) / CLOCKS_PER_SEC;
    printf("A funcao de insercao no hash demorou %f segundos para executar.\n", tempo_gasto);
}

// Função para listar alguns produtos do arquivo
void listarProdutos(FILE *arquivo, FILE *arquivoExtensao) {
    Produto produto;

    //Listar produtos do arquivo principal
    fseek(arquivo, 0, SEEK_SET);
    int i = 0;
    while (i < 1000 && fread(&produto, sizeof(Produto), 1, arquivo)) {
        if (!produto.removido)
            printarProduto(produto);
        i++;
    }

    //Listar produtos do arquivo de extensão
    if (arquivoExtensao) {
        fseek(arquivoExtensao, 0, SEEK_SET);
        while (fread(&produto, sizeof(Produto), 1, arquivoExtensao)) {
            if (!produto.removido)
                printarProduto(produto);
        }
    }
}

//Função para listar alguns dos registros de acesso
void listarAcessos(FILE *arquivo, FILE *arquivoExtensao) {
    Acesso acesso;

    //Listar acessos do arquivo principal
    fseek(arquivo, 0, SEEK_SET);
    int i = 0;
    while (i <= 10000500 && fread(&acesso, sizeof(Acesso), 1, arquivo)) {
        if (i >= 10000000 && i <= 10000500)
            printarAcesso(acesso);
        i++;
    }

    //Listar acessos do arquivo de extensão
    if (arquivoExtensao) {
        fseek(arquivoExtensao, 0, SEEK_SET);
        while (fread(&acesso, sizeof(Acesso), 1, arquivoExtensao))
            printarAcesso(acesso);
    }
}

//Função para buscar um produto nos arquivos
int buscarProduto(FILE *arquivoProdutos, FILE *arquivoExtensao, FILE *arquivoIndice, int idBuscado, Produto *retornarProduto) {
    Indice indice;
    Produto produto;
    int inicio = 0, fim, meio;
    long numRegistros;

    clock_t inicio_exec, fim_exec;
    double tempo_gasto;

    inicio_exec = clock();

    // Obter o número de registros no arquivo de índice
    fseek(arquivoIndice, 0, SEEK_END);
    numRegistros = ftell(arquivoIndice) / sizeof(Indice);
    fim = numRegistros - 1;

    // Pesquisa binária no arquivo de índice
    while (inicio <= fim) {
        meio = (inicio + fim) / 2;

        // Posicionar o ponteiro de leitura no registro do meio
        fseek(arquivoIndice, meio * sizeof(Indice), SEEK_SET);
        fread(&indice, sizeof(Indice), 1, arquivoIndice);

        // Comparar o ID do índice com o ID buscado
        if (indice.id == idBuscado && !produto.removido) {
            // ID encontrado no índice, buscar no arquivo de produtos
            fseek(arquivoProdutos, indice.offset, SEEK_SET);
            fread(&produto, sizeof(Produto), 1, arquivoProdutos);

            // Exibir os dados do produto encontrado
            printf("\nProduto encontrado:\n");
            printarProduto(produto);
            *retornarProduto = produto;

            fim_exec = clock();
            tempo_gasto = ((double)(fim_exec - inicio_exec)) / CLOCKS_PER_SEC;
            printf("A funcao demorou %f segundos para executar.\n", tempo_gasto);
            return 1;
        } else if (indice.id < idBuscado) {
            inicio = meio + 1;
        } else {
            fim = meio - 1;
        }
    }

    if (arquivoExtensao) {
        while (fread(&produto, sizeof(Produto), 1, arquivoExtensao)) {
            if (produto.id == idBuscado && !produto.removido) {
                printf("\nProduto encontrado no arquivo de extensao:\n");
                printarProduto(produto);
                *retornarProduto = produto;

                fim_exec = clock();
                tempo_gasto = ((double)(fim_exec - inicio_exec)) / CLOCKS_PER_SEC;
                printf("A funcao demorou %f segundos para executar.\n", tempo_gasto);
                return 1;
            }
        }
    }

    // Caso o ID não seja encontrado no índice
    return 0;
}

//Função para buscar um acesso nos arquivos
int buscarAcesso(FILE *arquivoAcessos, FILE *arquivoExtensao, FILE *arquivoIndice, long long idBuscado, Acesso *retornarAcesso) {
    clock_t inicio_exec, fim_exec;
    double tempo_gasto;

    inicio_exec = clock();
    
    Indice indice;
    Acesso acesso;
    int inicio = 0, fim, meio;
    long numRegistros;

    // Obter o número de registros no arquivo de índice
    fseek(arquivoIndice, 0, SEEK_END);
    numRegistros = ftell(arquivoIndice) / sizeof(Indice);
    fim = numRegistros - 1;

    // Pesquisa binária no arquivo de índice
    while (inicio <= fim) {
        meio = (inicio + fim) / 2;

        // Posicionar o ponteiro de leitura no registro do meio
        fseek(arquivoIndice, meio * sizeof(Indice), SEEK_SET);
        fread(&indice, sizeof(Indice), 1, arquivoIndice);

        // Comparar o ID do índice com o ID buscado
        if (indice.id == idBuscado) {
            // ID encontrado no índice, buscar no arquivo de produtos
            fseek(arquivoAcessos, indice.offset, SEEK_SET);
            fread(&acesso, sizeof(Acesso), 1, arquivoAcessos);

            // Exibir os dados do produto encontrado
            printf("\nAcesso encontrado:\n");
            printarAcesso(acesso);
            *retornarAcesso = acesso;
            fim_exec = clock();
            tempo_gasto = ((double)(fim_exec - inicio_exec)) / CLOCKS_PER_SEC;
            printf("A funcao demorou %f segundos para executar.\n", tempo_gasto);
            return 1;
        } else if (indice.id < idBuscado) {
            inicio = meio + 1;
        } else {
            fim = meio - 1;
        }
    }

    if (arquivoExtensao) {
        while (fread(&acesso, sizeof(Acesso), 1, arquivoExtensao)) {
            if (acesso.id == idBuscado) {
                printf("\nAcesso encontrado no arquivo de extensao:\n");
                printarAcesso(acesso);
                *retornarAcesso = acesso;
                fim_exec = clock();
                tempo_gasto = ((double)(fim_exec - inicio_exec)) / CLOCKS_PER_SEC;
                printf("A funcao demorou %f segundos para executar.\n", tempo_gasto);
                return 1;
            }
        }
    }

    // Caso o ID não seja encontrado no índice
    return 0;
}

//Função para dizer se um produto existe nos arquivos
int produtoExiste(int id) {
    FILE *arquivo = fopen("produtos.dat", "rb");
    FILE *arquivoIndice = fopen("indice_produtos.dat", "rb");
    FILE *arquivoExtensao = fopen("produtos_extensao.dat", "rb");

    if (!arquivo || !arquivoIndice) {
        printf("Erro ao abrir os arquivos.\n");
        return 0;
    }

    Produto produto;
    return buscarProduto(arquivo, arquivoExtensao, arquivoIndice, id, &produto);
}

//Função para pedir uma busca de um produto ao usuário
void pedirBuscaProduto(HashSessaoTable *tabela) {
    FILE *arquivoProdutos = fopen("produtos.dat", "rb");
    FILE *arquivoProdutosExtensao = fopen("produtos_extensao.dat", "rb");
    FILE *arquivoIndice = fopen("indice_produtos.dat", "rb");

    if (!arquivoProdutos || !arquivoIndice) {
        printf("Erro ao abrir os arquivos.\n");
        return;
    }

    printf("Insira o ID do produto que quer buscar:\n");
    int id;
    scanf("%d", &id);

    Produto produto;
    if (!buscarProduto(arquivoProdutos, arquivoProdutosExtensao, arquivoIndice, id, &produto))
        printf("Produto com ID %d nao encontrado.\n", id);
    else
        criarAcesso(produto, tabela);

    fclose(arquivoProdutos);
    fclose(arquivoIndice);
    if (arquivoProdutosExtensao)
        fclose(arquivoProdutosExtensao);
}

void pedirBuscaProdutoArvore(BPlusTree *arvore, HashSessaoTable *tabela) {
    printf("Insira o ID do produto que quer buscar:\n");
    int id;
    scanf("%d", &id);

    Produto produto;
    if (buscarProdutoArvore(&produto, arvore, id))
        criarAcesso(produto, tabela);
}

void pedirBuscaSessaoHash(HashSessaoTable *tabela) {
    char sessao[100];
    fgets(sessao, sizeof(sessao), stdin);
    sessao[strcspn(sessao, "\n")] = '\0';
    long long l = buscarHashSessao(tabela, sessao);
    if (l != -1)
        printf("Informacoes sobre a sessao foram impressas.\n");
    else
        printf("Sessão não encontrada.\n");

}

//Função para pedir uma busca de um produto ao usuário
void pedirBuscaAcesso() {
    FILE *arquivoAcessos = fopen("acessos.dat", "rb");
    FILE *arquivoAcessosExtensao = fopen("acessos_extensao.dat", "rb");
    FILE *arquivoIndice = fopen("indice_acessos.dat", "rb");

    if (arquivoAcessos == NULL || arquivoIndice == NULL) {
        printf("Erro ao abrir os arquivos.\n");
        return;
    }

    printf("Insira o ID do acesso que quer buscar:\n");
    long long id;
    scanf("%lld", &id);

    Acesso acesso;
    if (!buscarAcesso(arquivoAcessos, arquivoAcessosExtensao, arquivoIndice, id, &acesso))
        printf("Acesso com ID %d nao encontrado.\n", id);

    fclose(arquivoAcessos);
    fclose(arquivoIndice);
    if (arquivoAcessosExtensao)
        fclose(arquivoAcessosExtensao);
}

// Função hash simples
int hash(int id) {
    return id % HASH_SIZE;
}

// Função para inserir na tabela hash
void inserirHash(HashNode* hashTable[], int id) {
    int index = hash(id);
    HashNode* newNode = (HashNode*)malloc(sizeof(HashNode));
    newNode->id = id;
    newNode->next = hashTable[index];
    hashTable[index] = newNode;
}

// Função para verificar se um ID já está na tabela hash
int existeNaHash(HashNode* hashTable[], int id) {
    int index = hash(id);
    HashNode* current = hashTable[index];
    while (current != NULL) {
        if (current->id == id)
            return 1;
        
        current = current->next;
    }
    return 0;
}

// Função para liberar a memória da tabela hash
void liberarHash(HashNode* hashTable[]) {
    for (int i = 0; i < HASH_SIZE; i++) {
        HashNode* current = hashTable[i];
        while (current != NULL) {
            HashNode* temp = current;
            current = current->next;
            free(temp);
        }
    }
}

//Função para escrever um produto em um arquivo
void escreverProduto(FILE *arquivoProdutos, Produto produto) {
    fwrite(&produto, sizeof(Produto), 1, arquivoProdutos);
}

//Função para escrever um acesso em um arquivo
void escreverAcesso(FILE *arquivoAcessos, Acesso acesso) {
    fwrite(&acesso, sizeof(Acesso), 1, arquivoAcessos);
}

//Função para realizar a criação dos arquivos de dados, a partir do arquivo CSV
void criarArquivos() {
    clock_t inicio, fim;
    double tempo_gasto;

    inicio = clock();
    FILE *csvFile = fopen("all-data.csv", "r");
    FILE *arquivoProdutos = fopen("produtos.dat", "wb");
    FILE *arquivoAcessos = fopen("acessos.dat", "wb");

    if (!csvFile || !arquivoProdutos || !arquivoAcessos) {
        printf("Erro ao abrir os arquivos.\n");
        return;
    }

    char line[MAX_LINE_LENGTH];
    long long idAcesso = 1;
    char *token;
    int idProduto, idUser;
    float preco;
    char marca[30], categoria[45], sessao[100], data_acesso[24], eventType[10];
    Produto produtoAtual;
    produtoAtual.removido = false;
    Acesso acessoAtual;

    // Criar e inicializar a tabela hash
    HashNode* hashTable[HASH_SIZE] = {0};

    // Ignorar a primeira linha de cabeçalho do CSV
    fgets(line, MAX_LINE_LENGTH, csvFile);

    long long times = 0;
    while (fgets(line, MAX_LINE_LENGTH, csvFile)) {
        times++;

        // Limpar quebras de linha
        line[strcspn(line, "\r\n")] = '\0';

        char *start = line;
        char *end;
        int fieldCount = 0;
        
        while ((end = strchr(start, ',')) != NULL) {
            // Calcula o comprimento do token
            int len = end - start;
            
            if (len != 0) {
                char token[100];
                strncpy(token, start, len);
                token[len] = '\0';
                switch (fieldCount) {
                    case 0: {
                        strcpy(data_acesso, token);
                        break;
                    }
                    case 1: {
                        strcpy(eventType, token);
                        break;
                    }
                    case 2: {
                        idProduto = atoi(token);
                        break;
                    }
                    case 4: {
                        strcpy(categoria, token);
                        break;
                    }
                    case 5: {
                        strcpy(marca, token);
                        break;
                    }
                    case 6: {
                        preco = atof(token);
                        break;
                    }
                    case 7: {
                        idUser = atoi(token);
                        break;
                    }
                }
            }
            
            fieldCount++;
            start = end + 1;
        }

        // Processa o último campo (após a última vírgula)
        if (*start != '\0')
            strcpy(sessao, start);

        // Preencher a estrutura de Produto
        produtoAtual.id = idProduto;
        strcpy(produtoAtual.marca, marca);
        strcpy(produtoAtual.categoria, categoria);
        produtoAtual.preco = preco;

        // Verificar se o produto já foi armazenado usando a tabela hash
        if (!existeNaHash(hashTable, idProduto)) {
            inserirHash(hashTable, idProduto);
            escreverProduto(arquivoProdutos, produtoAtual);
        }

        // Preencher os dados de Acesso
        acessoAtual.id = idAcesso++;
        acessoAtual.id_user = idUser;
        strcpy(acessoAtual.categoria, categoria);
        strcpy(acessoAtual.sessao, sessao);
        strcpy(acessoAtual.data_acesso, data_acesso);
        strcpy(acessoAtual.type, eventType);

        // Escrever o acesso no arquivo binário
        escreverAcesso(arquivoAcessos, acessoAtual);
    }

    fclose(csvFile);
    fclose(arquivoProdutos);
    fclose(arquivoAcessos);

    // Liberar a memória usada pela tabela hash
    liberarHash(hashTable);

    fim = clock();

    printf("Arquivos criados com sucesso.\n");

    tempo_gasto = ((double)(fim - inicio)) / CLOCKS_PER_SEC;
    printf("A funcao demorou %f segundos para executar.\n", tempo_gasto);

    printf("%lld acessos foram criados.\n", times);
}

// Função para comparar produtos com base no ID
int compararProdutos(const void *a, const void *b) {
    return ((Produto *)a)->id - ((Produto *)b)->id;
}

// Função para comparar acessos com base no ID
int compararAcessos(const void *a, const void *b) {
    return ((Acesso *)a)->id - ((Acesso *)b)->id;
}

// Função para mesclar arquivos temporários
void mesclarArquivosProdutos(int numFiles) {
    clock_t inicio_exec, fim_exec;
    double tempo_gasto;

    inicio_exec = clock();

    printf("Mesclando arquivos dos produtos...\n");
    FILE **tempFiles = malloc(numFiles * sizeof(FILE *));
    Produto *buffers = malloc(numFiles * sizeof(Produto));
    int *indices = malloc(numFiles * sizeof(int));

    int i;
    for (i = 0; i < numFiles; i++) {
        char tempFileName[20];
        snprintf(tempFileName, sizeof(tempFileName), "tempProdutos%d.dat", i);
        tempFiles[i] = fopen(tempFileName, "rb");
        fread(&buffers[i], sizeof(Produto), 1, tempFiles[i]);
        indices[i] = 0;
    }

    FILE *output = fopen("produtos.dat", "wb");

    while (1) {
        //Achar menor índice
        int minIndex = -1;
        for (int i = 0; i < numFiles; i++) {
            if (indices[i] < 1 && (minIndex == -1 || buffers[i].id < buffers[minIndex].id)) {
                minIndex = i;
            }
        }

        // Se todos os arquivos foram lidos, quebrar loop
        if (minIndex == -1) break;

        //Escrever produto no arquivo, se não estiver marcado como removido
        if (buffers[minIndex].removido != 1)
            fwrite(&buffers[minIndex], sizeof(Produto), 1, output);

        if (fread(&buffers[minIndex], sizeof(Produto), 1, tempFiles[minIndex]) != 1)
            indices[minIndex] = 1; // Marca como lido
    }

    //Fechar e remover arquivos remporários
    for (i = 0; i < numFiles; i++) {
        fclose(tempFiles[i]);
        char nomeArquivo[20];
        sprintf(nomeArquivo, "tempProdutos%d.dat", i);
        remove(nomeArquivo);
    }

    fflush(output);

    free(tempFiles);
    free(buffers);
    free(indices);
    fclose(output);

    printf("Arquivo de produtos ordenado com sucesso.\n");

    fim_exec = clock();
    tempo_gasto = ((double)(fim_exec - inicio_exec)) / CLOCKS_PER_SEC;
    printf("A funcao de mesclagem demorou %f segundos para executar.\n", tempo_gasto);
}

//Função para colocar os produtos da área de extensão no arquivo principal
void mesclarProdutos() {
    clock_t inicio_exec, fim_exec;
    double tempo_gasto;

    inicio_exec = clock();

    printf("Mesclando arquivos dos produtos...\n");
    FILE *arquivoPrincipal = fopen("produtos.dat", "rb+");
    FILE *arquivoExtensao = fopen("produtos_extensao.dat", "rb");

    //Se não há arquivo de extensão, não é preciso mesclar
    if (!arquivoPrincipal || !arquivoExtensao) return;

    Produto produto;
    long long offsetPrincipal;

    // Mover para o final do arquivo principal para adicionar registros da extensão
    fseek(arquivoPrincipal, 0, SEEK_END);
    offsetPrincipal = ftell(arquivoPrincipal);

    while (fread(&produto, sizeof(Produto), 1, arquivoExtensao)) {
        // Escreve o produto da extensão no final do arquivo principal
        fwrite(&produto, sizeof(Produto), 1, arquivoPrincipal);
    }

    // Limpar a extensão após a mesclagem
    fclose(arquivoExtensao);
    remove("produtos_extensao.dat");

    fclose(arquivoPrincipal);

    printf("Mesclagem dos produtos concluida e extensao limpa.\n");
    fim_exec = clock();
    tempo_gasto = ((double)(fim_exec - inicio_exec)) / CLOCKS_PER_SEC;
    printf("A funcao de mesclagem demorou %f segundos para executar.\n", tempo_gasto);
}

// Função para ordenar o arquivo em blocos
int ordenarBlocosProdutos() {
    clock_t inicio_exec, fim_exec;
    double tempo_gasto;

    inicio_exec = clock();

    printf("Ordenado blocos dos produtos...\n");
    FILE *input = fopen("produtos.dat", "rb");
    if (!input) {
        perror("Erro ao abrir arquivo de entrada");
        return 0;
    }

    Produto *buffer = malloc(BLOCK_SIZE * sizeof(Produto));
    if (!buffer) {
        perror("Erro ao alocar memória");
        fclose(input);
        return 0;
    }

    int blockIndex = 0;

    while (1) {
        // Lê um bloco de produtos
        size_t readItems = fread(buffer, sizeof(Produto), BLOCK_SIZE, input);
        if (readItems == 0) {
            // Se não leu nada, saia do loop
            break;
        }

        // Ordena o bloco carregado
        qsort(buffer, readItems, sizeof(Produto), compararProdutos);

        // Escreve o bloco ordenado em um arquivo temporário
        char tempFileName[20];
        snprintf(tempFileName, sizeof(tempFileName), "tempProdutos%d.dat", blockIndex++);
        FILE *tempFile = fopen(tempFileName, "wb");
        if (!tempFile) {
            perror("Erro ao criar arquivo temporário");
            free(buffer);
            fclose(input);
            return 0;
        }

        fwrite(buffer, sizeof(Produto), readItems, tempFile);
        fclose(tempFile);
    }

    free(buffer);
    fclose(input);

    fim_exec = clock();
    tempo_gasto = ((double)(fim_exec - inicio_exec)) / CLOCKS_PER_SEC;
    printf("A funcao de ordenacao demorou %f segundos para executar.\n", tempo_gasto);
    return blockIndex;
}

//Função para começar a ordenação do arquivo de produtos
void ordenarArquivo() {
    mesclarProdutos();
    int blocos = ordenarBlocosProdutos();
    if (blocos == 0) return;

    mesclarArquivosProdutos(blocos);
    criarIndiceProdutos();
}

//Função para pedir um novo produto ao usuário
int pedirNovoProduto() {
    char input[100];
    int id;

    Produto produto;
    produto.removido = false;
    printf("Insira o ID do produto:\n");
    scanf("%d", &id);

    if (produtoExiste(id)) {
        printf("Um produto com esse ID ja existe!\n");
        return 0;
    }

    produto.id = id;

    printf("Insira o preco do produto:\n");
    scanf("%f", &produto.preco);

    getchar();
    printf("Insira a marca:\n");
    gets(input);
    strcpy(produto.marca, input);

    printf("Insira a categoria:\n");
    gets(input);
    strcpy(produto.categoria, input);

    FILE *arquivo = fopen("produtos_extensao.dat", "ab");
    if (!arquivo) 
        arquivo = fopen("produtos_extensao.dat", "wb");

    inserirProduto(arquivo, produto);
    printf("Produto criado com sucesso.\n");

    fclose(arquivo);

    ordenarArquivo();
    return 1;
}

//Função para marcar um produto como removido
void removerProduto(FILE *arquivoProdutos, FILE *arquivoExtensao, FILE *arquivoIndice, int idRemover) {
    Indice indice;
    Produto produto;
    int inicio = 0, fim, meio;
    long numRegistros;

    // Obter o número de registros no arquivo de índice
    fseek(arquivoIndice, 0, SEEK_END);
    numRegistros = ftell(arquivoIndice) / sizeof(Indice);
    fim = numRegistros - 1;

    // Pesquisa binária no arquivo de índice para encontrar o produto
    while (inicio <= fim) {
        meio = (inicio + fim) / 2;

        fseek(arquivoIndice, meio * sizeof(Indice), SEEK_SET);
        fread(&indice, sizeof(Indice), 1, arquivoIndice);

        if (indice.id == idRemover) {
            // Produto encontrado, buscar no arquivo de produtos
            fseek(arquivoProdutos, indice.offset, SEEK_SET);
            fread(&produto, sizeof(Produto), 1, arquivoProdutos);

            if (!produto.removido) {
                // Marcar o produto como removido
                produto.removido = true;

                // Voltar e reescrever o produto atualizado
                fseek(arquivoProdutos, indice.offset, SEEK_SET);
                fwrite(&produto, sizeof(Produto), 1, arquivoProdutos);

                fflush(arquivoProdutos);

                printf("Produto com ID %d foi removido com sucesso.\n", produto.id);
            } else {
                printf("Produto com ID %d ja esta marcado como removido.\n", idRemover);
            }

            return;
        } else if (indice.id < idRemover) {
            inicio = meio + 1;
        } else {
            fim = meio - 1;
        }
    }

    //Procurar produto na área de extensão
    if (arquivoExtensao) {
        fseek(arquivoExtensao, 0, SEEK_SET);
        long long offsetAtual = ftell(arquivoExtensao);
        while (fread(&produto, sizeof(Produto), 1, arquivoExtensao)) {
            if (produto.id == idRemover) {
                if (!produto.removido) {
                    // Marcar o produto como removido
                    produto.removido = true;

                    // Voltar e reescrever o produto atualizado
                    fseek(arquivoExtensao, offsetAtual, SEEK_SET);
                    fwrite(&produto, sizeof(Produto), 1, arquivoExtensao);

                    printf("Produto com ID %d foi removido com sucesso.\n", idRemover);
                } else {
                    printf("Produto com ID %d ja esta marcado como removido.\n", idRemover);
                }

                return;
            }

            offsetAtual = ftell(arquivoExtensao);
        }
    }

    printf("Produto com ID %d nao encontrado.\n", idRemover);
}

//Função para pedir ao usuário o produto a ser removido
void pedirRemocao() {
    FILE *arquivo = fopen("produtos.dat", "ab+");
    FILE *arquivoIndice = fopen("indice_produtos.dat", "ab+");
    FILE *arquivoExtensao = fopen("produtos_extensao.dat", "ab+");

    if (!arquivo || !arquivoIndice) {
        printf("Erro ao abrir os arquivos.\n");
        return;
    }

    printf("Insira o ID do produto que deseja remover:\n");
    int id;
    scanf("%d", &id);

    removerProduto(arquivo, arquivoExtensao, arquivoIndice, id);

    fclose(arquivo);
    fclose(arquivoIndice);
    if (arquivoExtensao)
        fclose(arquivoExtensao);
}

//Função para listar todos os produtos, incluíndo área de extensão
void listarTodosProdutos() {
    FILE *arquivo = fopen("produtos.dat", "rb");
    FILE *arquivoExtensao = fopen("produtos_extensao.dat", "rb");
    listarProdutos(arquivo, arquivoExtensao);
    fclose(arquivo);
    if (arquivoExtensao)
        fclose(arquivoExtensao);
}

//Função para listar todos os acessos
void listarTodosAcessos() {
    FILE *arquivo = fopen("acessos.dat", "rb");
    FILE *arquivoExtensao = fopen("acessos_extensao.dat", "rb");

    if (!arquivo) {
        printf("Erro ao abrir o arquivo.\n");
        return;
    }

    listarAcessos(arquivo, arquivoExtensao);
    fclose(arquivo);
    if (arquivoExtensao)
        fclose(arquivoExtensao);
}

// Função de hash para mapear a marca a um índice
unsigned int hashMarca(const char *str) {
    unsigned int hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash % HASH_SIZE_BUSCA;
}

// Função para adicionar ou atualizar a contagem de uma marca na tabela hash
void adicionarNaTabela(TabelaHash *tabela, const char *marca) {
    unsigned int index = hashMarca(marca);
    StringNode *node = tabela->tabela[index];

    // Percorre a lista encadeada no índice
    while (node != NULL) {
        if (strcmp(node->nome, marca) == 0) {
            node->quantidade += 1; // Atualiza a quantidade
            return;
        }
        node = node->next;
    }

    // Se o nó não foi encontrado, cria um novo nó
    StringNode *novoNode = malloc(sizeof(StringNode));
    strcpy(novoNode->nome, marca);
    novoNode->quantidade = 1;
    novoNode->next = tabela->tabela[index];
    tabela->tabela[index] = novoNode; // Insere na tabela
}

// Função para encontrar a marca com mais produtos à venda
void encontrarMarcaMaisProdutos() {
    FILE *arquivo = fopen("produtos.dat", "rb");
    if (!arquivo) {
        perror("Erro ao abrir o arquivo de produtos");
        return;
    }

    TabelaHash tabela = {0}; // Inicializa a tabela hash
    Produto produto;

    // Lê todos os produtos do arquivo
    while (fread(&produto, sizeof(Produto), 1, arquivo))
        adicionarNaTabela(&tabela, produto.marca);

    fclose(arquivo);

    // Encontra a marca com a maior quantidade
    char marcaMaisProdutos[STRING_LEN];
    int maxQuantidade = 0;

    for (int i = 0; i < HASH_SIZE_BUSCA; i++) {
        StringNode *node = tabela.tabela[i];
        while (node != NULL) {
            if (node->quantidade > maxQuantidade) {
                maxQuantidade = node->quantidade;
                strcpy(marcaMaisProdutos, node->nome);
            }
            node = node->next;
        }
    }

    // Imprime a marca com mais produtos à venda
    if (maxQuantidade > 0) {
        printf("Marca com mais produtos a venda: %s com %d produtos\n",
               marcaMaisProdutos, maxQuantidade);
    } else {
        printf("Nenhum produto encontrado.\n");
    }

    // Libera a memória da tabela hash
    for (int i = 0; i < HASH_SIZE_BUSCA; i++) {
        StringNode *node = tabela.tabela[i];
        while (node != NULL) {
            StringNode *temp = node;
            node = node->next;
            free(temp);
        }
    }
}

//Função para achar a categoria mais acessada pelos usuários
const char* categoriaMaisAcessada(TabelaHash *tabela) {
    int maxQuantidade = 0;
    const char *categoriaComMais = NULL;

    for (int i = 0; i < HASH_SIZE; i++) {
        StringNode *node = tabela->tabela[i];
        while (node != NULL) {
            if (node->quantidade > maxQuantidade) {
                maxQuantidade = node->quantidade;
                categoriaComMais = node->nome;
            }
            node = node->next;
        }
    }

    return categoriaComMais;
}

//Função para encontrar a categoria mais acessada pelos usuários
void encontrarCategoriaMaisAcessada() {
    clock_t inicio, fim;
    double tempo_gasto;

    inicio = clock();
    FILE *arquivo = fopen("acessos.dat", "rb");
    
    TabelaHash tabela = {0};
    if (!arquivo) {
        printf("Erro ao abrir o arquivo de acessos.\n");
        return;
    }

    Acesso acesso;
    while (fread(&acesso, sizeof(Acesso), 1, arquivo))
        adicionarNaTabela(&tabela, acesso.categoria);

    fclose(arquivo);

    // Encontra a categoria com maior acesso
    char categoria[STRING_LEN];
    int maxQuantidade = 0;

    for (int i = 0; i < HASH_SIZE_BUSCA; i++) {
        StringNode *node = tabela.tabela[i];
        while (node != NULL) {
            if (node->quantidade > maxQuantidade) {
                maxQuantidade = node->quantidade;
                strcpy(categoria, node->nome);
            }
            node = node->next;
        }
    }

    // Imprime a marca com mais produtos à venda
    if (maxQuantidade > 0) {
        printf("Categoria com mais acessos: %s com %d acessos\n",
               categoria, maxQuantidade);
    } else {
        printf("Nenhum acesso encontrado.\n");
    }

    // Libera a memória da tabela hash
    for (int i = 0; i < HASH_SIZE_BUSCA; i++) {
        StringNode *node = tabela.tabela[i];
        while (node != NULL) {
            StringNode *temp = node;
            node = node->next;
            free(temp);
        }
    }

    fim = clock();
    tempo_gasto = ((double)(fim - inicio)) / CLOCKS_PER_SEC;
    printf("A funcao demorou %f segundos para executar.\n", tempo_gasto);
}

BPlusTree *arvore;
HashSessaoTable *tabela;

int main() {
    arvore = inicializarArvore();
    tabela = inicializarTabelaHashSessao();
    printf("\nOla, seja bem-vindo!\n");
    printf("O que deseja fazer?\n");

    int opcao, sair = 0;
    while (1) {
        printf("\nOpcoes disponiveis:\n");
        printf("0. Sair do programa;\n");
        printf("1. Criar arquivos atraves do CSV;\n");
        printf("2. Criar arquivo de indice dos acessos;\n");
        printf("3. Reorganizar o arquivo de produtos;\n");
        printf("4. Listar produtos;\n");
        printf("5. Listar acessos;\n");
        printf("6. Adicionar produto;\n");
        printf("7. Remover produto;\n");
        printf("8. Buscar produto pelo ID;\n");
        printf("9. Buscar acesso pelo ID;\n");
        printf("10. Buscar a marca com mais produtos a venda;\n");
        printf("11. Buscar a categoria com maior numero de acessos;\n");
        printf("12. Criar tabela hash para indice de sessoes;\n");
        printf("13. Criar arvore B+ para indice de produtos;\n");
        printf("14. Buscar sessao na tabela hash;\n");
        printf("15. Buscar produto na arvore\n");
        printf("Insira a opcao que deseja:\n");
        scanf("%d", &opcao);
        getchar();

        switch (opcao) {
        case 0:
            sair = 1;
            break;

        case 1:
            criarArquivos();
            break;

        case 2:
            criarIndiceAcessos();
            break;

        case 3:
            ordenarArquivo();
            break;

        case 4:
            listarTodosProdutos();
            break;

        case 5:
            listarTodosAcessos();
            break;

        case 6:
            if (pedirNovoProduto()){
                arvore = inicializarArvore();
                criarIndiceProdutosComArvore(arvore);
            }
            break;

        case 7:
            pedirRemocao();
            break;

        case 8:
            pedirBuscaProduto(tabela);
            break;

        case 9:
            pedirBuscaAcesso();
            break;

        case 10:
            encontrarMarcaMaisProdutos();
            break;
    
        case 11:
            encontrarCategoriaMaisAcessada();
            break;

        case 12:
            criarIndiceHashSessao(tabela);
            break;

        case 13:
            criarIndiceProdutosComArvore(arvore);
            break;

        case 14:
            pedirBuscaSessaoHash(tabela);
            break;

        case 15:
            pedirBuscaProdutoArvore(arvore, tabela);
            break;
        }

        if (sair)
            break;
    }

    printf("\nAte mais!\n");
    return 0;
}