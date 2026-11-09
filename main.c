/* main.c - Terminal, teclado e laço principal do Jogo da Cobrinha.
 *
 * Funciona em Linux/macOS (termios) e no Windows 10+ (conio + console VT).
 */
#if !defined(_WIN32)
#define _POSIX_C_SOURCE 200809L
#endif

#include "jogo.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ARQUIVO_RECORDE "cobrinha_recorde.txt"

typedef enum {
    TECLA_NADA,
    TECLA_CIMA,
    TECLA_BAIXO,
    TECLA_ESQUERDA,
    TECLA_DIREITA,
    TECLA_PAUSA,
    TECLA_REINICIAR,
    TECLA_SAIR
} Tecla;

static Tecla tecla_de_letra(int c)
{
    switch (c) {
    case 'w': case 'W': return TECLA_CIMA;
    case 's': case 'S': return TECLA_BAIXO;
    case 'a': case 'A': return TECLA_ESQUERDA;
    case 'd': case 'D': return TECLA_DIREITA;
    case 'p': case 'P': case ' ': return TECLA_PAUSA;
    case 'r': case 'R': return TECLA_REINICIAR;
    case 'q': case 'Q': case 27: return TECLA_SAIR;
    default:            return TECLA_NADA;
    }
}

/* ------------------------------------------------------------------ */
/* Camada de plataforma                                                */
/* ------------------------------------------------------------------ */
#if defined(_WIN32)

#include <conio.h>
#include <windows.h>

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

static HANDLE saida_console;
static DWORD  modo_original;

static void plataforma_iniciar(void)
{
    saida_console = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleMode(saida_console, &modo_original);
    SetConsoleMode(saida_console, modo_original | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    SetConsoleOutputCP(CP_UTF8);
}

static void plataforma_finalizar(void)
{
    SetConsoleMode(saida_console, modo_original);
}

static bool sinal_de_saida(void) { return false; }

static Tecla ler_tecla(void)
{
    if (!_kbhit())
        return TECLA_NADA;
    int c = _getch();
    if (c == 0 || c == 224) {  /* teclas especiais (setas) */
        switch (_getch()) {
        case 72: return TECLA_CIMA;
        case 80: return TECLA_BAIXO;
        case 75: return TECLA_ESQUERDA;
        case 77: return TECLA_DIREITA;
        default: return TECLA_NADA;
        }
    }
    return tecla_de_letra(c);
}

static long long agora_ms(void) { return (long long)GetTickCount64(); }
static void dormir_ms(int ms)   { Sleep((DWORD)ms); }

#else /* POSIX */

#include <signal.h>
#include <termios.h>
#include <unistd.h>

static struct termios        termios_original;
static volatile sig_atomic_t recebeu_sinal = 0;

static void ao_receber_sinal(int sinal)
{
    (void)sinal;
    recebeu_sinal = 1;
}

static void plataforma_iniciar(void)
{
    tcgetattr(STDIN_FILENO, &termios_original);
    struct termios cru = termios_original;
    cru.c_lflag &= (tcflag_t)~(ICANON | ECHO);  /* sem eco, sem esperar Enter */
    cru.c_cc[VMIN]  = 0;                          /* read() não bloqueia */
    cru.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &cru);

    signal(SIGINT, ao_receber_sinal);
    signal(SIGTERM, ao_receber_sinal);
}

static void plataforma_finalizar(void)
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &termios_original);
}

static bool sinal_de_saida(void) { return recebeu_sinal != 0; }

static int ler_byte(void)
{
    unsigned char c;
    return read(STDIN_FILENO, &c, 1) == 1 ? c : -1;
}

static Tecla ler_tecla(void)
{
    int c = ler_byte();
    if (c < 0)
        return TECLA_NADA;
    if (c == 27) {  /* ESC: pode ser o início de uma seta (ESC [ A) */
        int c2 = ler_byte();
        if (c2 < 0)
            return TECLA_SAIR;  /* ESC sozinho */
        if (c2 == '[' || c2 == 'O') {
            switch (ler_byte()) {
            case 'A': return TECLA_CIMA;
            case 'B': return TECLA_BAIXO;
            case 'D': return TECLA_ESQUERDA;
            case 'C': return TECLA_DIREITA;
            default:  return TECLA_NADA;
            }
        }
        return TECLA_NADA;
    }
    return tecla_de_letra(c);
}

static long long agora_ms(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (long long)t.tv_sec * 1000 + t.tv_nsec / 1000000;
}

static void dormir_ms(int ms)
{
    struct timespec t = { ms / 1000, (long)(ms % 1000) * 1000000L };
    nanosleep(&t, NULL);
}

#endif

/* ------------------------------------------------------------------ */
/* Recorde                                                             */
/* ------------------------------------------------------------------ */
static int carregar_recorde(void)
{
    int    recorde = 0;
    FILE  *f       = fopen(ARQUIVO_RECORDE, "r");
    if (f) {
        if (fscanf(f, "%d", &recorde) != 1 || recorde < 0)
            recorde = 0;
        fclose(f);
    }
    return recorde;
}

static void salvar_recorde(int recorde)
{
    FILE *f = fopen(ARQUIVO_RECORDE, "w");
    if (f) {
        fprintf(f, "%d\n", recorde);
        fclose(f);
    }
}

/* ------------------------------------------------------------------ */
/* Desenho                                                             */
/* ------------------------------------------------------------------ */
/* Cada casa ocupa 2 colunas do terminal para ficar quadrada. */
#define COR_PAREDE  "\033[47m  "
#define COR_VAZIO   "\033[0m  "
#define COR_CORPO   "\033[42m  "
#define COR_CABECA  "\033[102m  "
#define COR_COMIDA  "\033[41m  "
#define RESET       "\033[0m"
#define LIMPA_LINHA "\033[K"

typedef struct {
    char   dados[32768];
    size_t usado;
} Buffer;

static void buf_add(Buffer *b, const char *s)
{
    size_t n = strlen(s);
    if (b->usado + n < sizeof b->dados) {
        memcpy(b->dados + b->usado, s, n);
        b->usado += n;
    }
}

static void buf_printf(Buffer *b, const char *fmt, int a, int c)
{
    char tmp[256];
    snprintf(tmp, sizeof tmp, fmt, a, c);
    buf_add(b, tmp);
}

static void desenhar(const Jogo *j, int recorde)
{
    static Buffer b;
    b.usado = 0;

    buf_add(&b, "\033[H");  /* cursor no topo, sem limpar: evita piscar */
    buf_add(&b, RESET "  \033[1;32mJOGO DA COBRINHA" RESET LIMPA_LINHA "\n");
    buf_printf(&b, "  Pontos: \033[1m%d" RESET "    Recorde: \033[1m%d" RESET LIMPA_LINHA "\n",
               j->pontos, recorde > j->pontos ? recorde : j->pontos);

    Ponto cab = jogo_cabeca(j);
    for (int y = -1; y <= ALTURA; y++) {
        buf_add(&b, "  ");
        for (int x = -1; x <= LARGURA; x++) {
            if (x < 0 || x >= LARGURA || y < 0 || y >= ALTURA)
                buf_add(&b, COR_PAREDE);
            else if (x == cab.x && y == cab.y)
                buf_add(&b, COR_CABECA);
            else if (j->ocupado[y][x])
                buf_add(&b, COR_CORPO);
            else if (x == j->comida.x && y == j->comida.y && j->estado != ESTADO_VENCEU)
                buf_add(&b, COR_COMIDA);
            else
                buf_add(&b, COR_VAZIO);
        }
        buf_add(&b, RESET LIMPA_LINHA "\n");
    }

    switch (j->estado) {
    case ESTADO_JOGANDO:
        buf_add(&b, "  Setas/WASD: mover   P/Espaço: pausar   Q: sair" LIMPA_LINHA "\n");
        break;
    case ESTADO_PAUSADO:
        buf_add(&b, "  \033[1;33mPAUSADO\033[0m - aperte P para continuar" LIMPA_LINHA "\n");
        break;
    case ESTADO_PERDEU:
        buf_add(&b, "  \033[1;31mFIM DE JOGO!\033[0m  R: jogar de novo   Q: sair" LIMPA_LINHA "\n");
        break;
    case ESTADO_VENCEU:
        buf_add(&b, "  \033[1;32mVOCÊ VENCEU!\033[0m  R: jogar de novo   Q: sair" LIMPA_LINHA "\n");
        break;
    }

    fwrite(b.dados, 1, b.usado, stdout);
    fflush(stdout);
}

/* ------------------------------------------------------------------ */
/* Laço principal                                                      */
/* ------------------------------------------------------------------ */
int main(void)
{
    srand((unsigned)time(NULL));
    plataforma_iniciar();
    fputs("\033[2J\033[?25l", stdout);  /* limpa a tela e esconde o cursor */

    int  recorde = carregar_recorde();
    Jogo jogo;
    jogo_iniciar(&jogo);

    bool      rodando      = true;
    long long proximo_passo = agora_ms() + jogo_intervalo_ms(&jogo);
    desenhar(&jogo, recorde);

    while (rodando && !sinal_de_saida()) {
        bool  redesenhar = false;
        Tecla t;

        while ((t = ler_tecla()) != TECLA_NADA) {
            switch (t) {
            case TECLA_CIMA:     jogo_virar(&jogo, CIMA);     break;
            case TECLA_BAIXO:    jogo_virar(&jogo, BAIXO);    break;
            case TECLA_ESQUERDA: jogo_virar(&jogo, ESQUERDA); break;
            case TECLA_DIREITA:  jogo_virar(&jogo, DIREITA);  break;
            case TECLA_PAUSA:
                jogo_pausar(&jogo);
                redesenhar = true;
                break;
            case TECLA_REINICIAR:
                if (jogo.estado == ESTADO_PERDEU || jogo.estado == ESTADO_VENCEU) {
                    jogo_iniciar(&jogo);
                    proximo_passo = agora_ms() + jogo_intervalo_ms(&jogo);
                    redesenhar = true;
                }
                break;
            case TECLA_SAIR:
                rodando = false;
                break;
            case TECLA_NADA:
                break;
            }
        }

        long long agora = agora_ms();
        if (jogo.estado != ESTADO_JOGANDO) {
            proximo_passo = agora + jogo_intervalo_ms(&jogo);
        } else if (agora >= proximo_passo) {
            jogo_passo(&jogo);
            proximo_passo = agora + jogo_intervalo_ms(&jogo);
            redesenhar = true;

            if (jogo.estado != ESTADO_JOGANDO && jogo.pontos > recorde) {
                recorde = jogo.pontos;
                salvar_recorde(recorde);
            }
        }

        if (redesenhar)
            desenhar(&jogo, recorde);
        dormir_ms(5);
    }

    if (jogo.pontos > recorde)
        salvar_recorde(jogo.pontos);

    fputs(RESET "\033[?25h\n", stdout);  /* mostra o cursor de novo */
    fflush(stdout);
    plataforma_finalizar();
    printf("Valeu por jogar! Pontuação final: %d\n", jogo.pontos);
    return 0;
}
