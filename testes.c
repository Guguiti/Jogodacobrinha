/* testes.c - Testes da lógica do jogo (sem terminal). */
#include "jogo.h"

#include <stdio.h>
#include <stdlib.h>

static int falhas = 0;

#define VERIFICA(cond)                                                  \
    do {                                                                \
        if (!(cond)) {                                                  \
            printf("  FALHOU %s:%d: %s\n", __FILE__, __LINE__, #cond);  \
            falhas++;                                                   \
        }                                                               \
    } while (0)

/* Coloca a comida longe do caminho para não interferir no teste. */
static void comida_no_canto(Jogo *j)
{
    j->comida.x = 0;
    j->comida.y = 0;
}

static void teste_inicio(void)
{
    Jogo j;
    jogo_iniciar(&j);
    VERIFICA(j.tamanho == 3);
    VERIFICA(j.estado == ESTADO_JOGANDO);
    VERIFICA(j.pontos == 0);
    VERIFICA(!j.ocupado[j.comida.y][j.comida.x]);
}

static void teste_anda_para_direita(void)
{
    Jogo j;
    jogo_iniciar(&j);
    comida_no_canto(&j);
    Ponto antes = jogo_cabeca(&j);
    jogo_passo(&j);
    Ponto depois = jogo_cabeca(&j);
    VERIFICA(depois.x == antes.x + 1 && depois.y == antes.y);
    VERIFICA(j.tamanho == 3);
}

static void teste_nao_volta_para_tras(void)
{
    Jogo j;
    jogo_iniciar(&j);
    comida_no_canto(&j);
    jogo_virar(&j, ESQUERDA);  /* oposto de DIREITA: ignorado */
    jogo_passo(&j);
    VERIFICA(j.estado == ESTADO_JOGANDO);
    VERIFICA(j.dir == DIREITA);

    /* Duas teclas rápidas: CIMA entra na fila; BAIXO logo depois seria 180° de CIMA. */
    jogo_virar(&j, CIMA);
    jogo_virar(&j, BAIXO);     /* oposto de CIMA: ignorado */
    jogo_passo(&j);
    VERIFICA(j.dir == CIMA);
}

static void teste_come_e_cresce(void)
{
    Jogo j;
    jogo_iniciar(&j);
    Ponto c = jogo_cabeca(&j);
    j.comida.x = c.x + 1;
    j.comida.y = c.y;
    jogo_passo(&j);
    VERIFICA(j.tamanho == 4);
    VERIFICA(j.pontos == 10);
    VERIFICA(!j.ocupado[j.comida.y][j.comida.x]);
}

static void teste_bate_na_parede(void)
{
    Jogo j;
    jogo_iniciar(&j);
    comida_no_canto(&j);
    for (int i = 0; i < LARGURA && j.estado == ESTADO_JOGANDO; i++)
        jogo_passo(&j);
    VERIFICA(j.estado == ESTADO_PERDEU);
}

static void teste_bate_no_proprio_corpo(void)
{
    Jogo j;
    jogo_iniciar(&j);
    comida_no_canto(&j);
    j.tamanho = 3;
    /* Cresce até 5 comendo duas vezes em linha reta. */
    for (int i = 0; i < 2; i++) {
        Ponto c = jogo_cabeca(&j);
        j.comida.x = c.x + 1;
        j.comida.y = c.y;
        jogo_passo(&j);
    }
    comida_no_canto(&j);
    VERIFICA(j.tamanho == 5);
    jogo_virar(&j, BAIXO);    jogo_passo(&j);
    jogo_virar(&j, ESQUERDA); jogo_passo(&j);
    jogo_virar(&j, CIMA);     jogo_passo(&j);  /* entra no próprio corpo */
    VERIFICA(j.estado == ESTADO_PERDEU);
}

static void teste_pode_entrar_onde_a_cauda_estava(void)
{
    /* Cobra de 4 andando em quadrado: a cabeça entra na casa que a cauda acabou de deixar. */
    Jogo j;
    jogo_iniciar(&j);
    Ponto c = jogo_cabeca(&j);
    j.comida.x = c.x + 1;
    j.comida.y = c.y;
    jogo_passo(&j);
    comida_no_canto(&j);
    VERIFICA(j.tamanho == 4);
    jogo_virar(&j, BAIXO);    jogo_passo(&j);
    jogo_virar(&j, ESQUERDA); jogo_passo(&j);
    jogo_virar(&j, CIMA);     jogo_passo(&j);
    VERIFICA(j.estado == ESTADO_JOGANDO);
}

static void teste_pausa(void)
{
    Jogo j;
    jogo_iniciar(&j);
    comida_no_canto(&j);
    Ponto antes = jogo_cabeca(&j);
    jogo_pausar(&j);
    jogo_passo(&j);
    Ponto depois = jogo_cabeca(&j);
    VERIFICA(j.estado == ESTADO_PAUSADO);
    VERIFICA(antes.x == depois.x && antes.y == depois.y);
    jogo_pausar(&j);
    VERIFICA(j.estado == ESTADO_JOGANDO);
}

static void teste_velocidade(void)
{
    Jogo j;
    jogo_iniciar(&j);
    int inicial = jogo_intervalo_ms(&j);
    j.pontos = 100;
    VERIFICA(jogo_intervalo_ms(&j) < inicial);
    j.pontos = 100000;
    VERIFICA(jogo_intervalo_ms(&j) == 60);
}

int main(void)
{
    srand(42);
    teste_inicio();
    teste_anda_para_direita();
    teste_nao_volta_para_tras();
    teste_come_e_cresce();
    teste_bate_na_parede();
    teste_bate_no_proprio_corpo();
    teste_pode_entrar_onde_a_cauda_estava();
    teste_pausa();
    teste_velocidade();

    if (falhas == 0)
        printf("Todos os testes passaram.\n");
    else
        printf("%d verificação(ões) falharam.\n", falhas);
    return falhas == 0 ? 0 : 1;
}
