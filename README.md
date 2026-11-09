# Jogo da Cobrinha

Jogo da cobrinha (Snake) para o terminal, escrito em **C99** puro, sem bibliotecas externas.
Funciona no Linux, no macOS e no Windows 10+.

## Como compilar e jogar

Linux / macOS:

```bash
make
./cobrinha
```

Windows (MinGW):

```bash
gcc -std=c99 -O2 -o cobrinha.exe main.c jogo.c
cobrinha.exe
```

## Controles

| Tecla              | Ação                    |
|--------------------|-------------------------|
| Setas ou W A S D   | Mover a cobra           |
| P ou Espaço        | Pausar / continuar      |
| R                  | Jogar de novo (no fim)  |
| Q ou Esc           | Sair                    |

Cada comida vale 10 pontos e deixa a cobra um pouco mais rápida.
O recorde fica salvo em `cobrinha_recorde.txt`.

## Organização do código

- `jogo.h` / `jogo.c` — regras do jogo (movimento, colisão, comida, pontuação), sem nada de terminal.
- `main.c` — teclado, desenho na tela e laço principal (termios no Linux/macOS, conio no Windows).
- `testes.c` — testes das regras: `make test`.
