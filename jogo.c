/* jogo.c - Lógica do Jogo da Cobrinha. */
#include "jogo.h"

#include <stdlib.h>
#include <string.h>

#define PONTOS_POR_COMIDA 10

static int indice_cauda(const Jogo *j)
{
    return (j->cabeca - (j->tamanho - 1) + MAX_COBRA) % MAX_COBRA;
}

static bool opostas(Direcao a, Direcao b)
{
    return (a == CIMA && b == BAIXO) || (a == BAIXO && b == CIMA) ||
           (a == ESQUERDA && b == DIREITA) || (a == DIREITA && b == ESQUERDA);
}

/* Sorteia uma célula livre para a comida. Retorna false se não houver nenhuma. */
static bool posicionar_comida(Jogo *j)
{
    int livres = MAX_COBRA - j->tamanho;
    if (livres <= 0)
        return false;

    int alvo = rand() % livres;
    for (int y = 0; y < ALTURA; y++) {
        for (int x = 0; x < LARGURA; x++) {
            if (j->ocupado[y][x])
                continue;
            if (alvo-- == 0) {
                j->comida.x = x;
                j->comida.y = y;
                return true;
            }
        }
    }
    return false;
}

void jogo_iniciar(Jogo *j)
{
    memset(j, 0, sizeof *j);
    j->dir     = DIREITA;
    j->tamanho = 3;
    j->estado  = ESTADO_JOGANDO;

    /* Cobra horizontal no centro, olhando para a direita. */
    int cx = LARGURA / 2, cy = ALTURA / 2;
    for (int i = 0; i < j->tamanho; i++) {
        Ponto p = { cx - (j->tamanho - 1) + i, cy };
        j->corpo[i] = p;
        j->ocupado[p.y][p.x] = true;
    }
    j->cabeca = j->tamanho - 1;

    posicionar_comida(j);
}

void jogo_virar(Jogo *j, Direcao d)
{
    if (j->nfila >= FILA_MAX)
        return;
    Direcao ultima = j->nfila > 0 ? j->fila[j->nfila - 1] : j->dir;
    if (d == ultima || opostas(d, ultima))
        return;
    j->fila[j->nfila++] = d;
}

void jogo_pausar(Jogo *j)
{
    if (j->estado == ESTADO_JOGANDO)
        j->estado = ESTADO_PAUSADO;
    else if (j->estado == ESTADO_PAUSADO)
        j->estado = ESTADO_JOGANDO;
}

Ponto jogo_cabeca(const Jogo *j)
{
    return j->corpo[j->cabeca];
}

void jogo_passo(Jogo *j)
{
    if (j->estado != ESTADO_JOGANDO)
        return;

    /* Aplica a próxima curva pendente, se houver. */
    if (j->nfila > 0) {
        j->dir = j->fila[0];
        memmove(j->fila, j->fila + 1, (size_t)(j->nfila - 1) * sizeof j->fila[0]);
        j->nfila--;
    }

    Ponto nova = jogo_cabeca(j);
    switch (j->dir) {
    case CIMA:     nova.y--; break;
    case BAIXO:    nova.y++; break;
    case ESQUERDA: nova.x--; break;
    case DIREITA:  nova.x++; break;
    }

    if (nova.x < 0 || nova.x >= LARGURA || nova.y < 0 || nova.y >= ALTURA) {
        j->estado = ESTADO_PERDEU;
        return;
    }

    bool  come  = nova.x == j->comida.x && nova.y == j->comida.y;
    Ponto cauda = j->corpo[indice_cauda(j)];

    /* Se não comer, a cauda sai do lugar neste mesmo passo,
     * então a cabeça pode ocupar a casa onde a cauda estava. */
    if (!come)
        j->ocupado[cauda.y][cauda.x] = false;

    if (j->ocupado[nova.y][nova.x]) {
        if (!come)
            j->ocupado[cauda.y][cauda.x] = true;  /* mantém o desenho intacto */
        j->estado = ESTADO_PERDEU;
        return;
    }

    j->cabeca = (j->cabeca + 1) % MAX_COBRA;
    j->corpo[j->cabeca] = nova;
    j->ocupado[nova.y][nova.x] = true;

    if (come) {
        j->tamanho++;
        j->pontos += PONTOS_POR_COMIDA;
        if (!posicionar_comida(j))
            j->estado = ESTADO_VENCEU;  /* tabuleiro cheio */
    }
}

int jogo_intervalo_ms(const Jogo *j)
{
    int ms = 160 - (j->pontos / PONTOS_POR_COMIDA) * 4;
    return ms < 60 ? 60 : ms;
}
