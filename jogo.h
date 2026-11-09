/* jogo.h - Lógica do Jogo da Cobrinha (independente do terminal). */
#ifndef JOGO_H
#define JOGO_H

#include <stdbool.h>

#define LARGURA   24
#define ALTURA    16
#define MAX_COBRA (LARGURA * ALTURA)
#define FILA_MAX  3   /* curvas guardadas entre um passo e outro */

typedef enum { CIMA, BAIXO, ESQUERDA, DIREITA } Direcao;

typedef enum {
    ESTADO_JOGANDO,
    ESTADO_PAUSADO,
    ESTADO_PERDEU,
    ESTADO_VENCEU
} Estado;

typedef struct {
    int x, y;
} Ponto;

typedef struct {
    Ponto   corpo[MAX_COBRA];          /* buffer circular; corpo[cabeca] é a cabeça */
    int     cabeca;
    int     tamanho;
    bool    ocupado[ALTURA][LARGURA];  /* células ocupadas pela cobra */
    Direcao dir;
    Direcao fila[FILA_MAX];            /* curvas pendentes */
    int     nfila;
    Ponto   comida;
    int     pontos;
    Estado  estado;
} Jogo;

/* Prepara uma nova partida. Usa rand() para posicionar a comida. */
void  jogo_iniciar(Jogo *j);

/* Pede uma curva. Curvas para trás (180°) ou repetidas são ignoradas. */
void  jogo_virar(Jogo *j, Direcao d);

/* Avança a cobra uma casa. Não faz nada se o jogo não estiver em andamento. */
void  jogo_passo(Jogo *j);

/* Alterna entre jogando e pausado. */
void  jogo_pausar(Jogo *j);

Ponto jogo_cabeca(const Jogo *j);

/* Tempo entre passos, em milissegundos: diminui conforme a pontuação. */
int   jogo_intervalo_ms(const Jogo *j);

#endif /* JOGO_H */
