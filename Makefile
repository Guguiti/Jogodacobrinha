CC      ?= cc
CFLAGS  ?= -std=c99 -Wall -Wextra -Wpedantic -O2

all: cobrinha

cobrinha: main.c jogo.c jogo.h
	$(CC) $(CFLAGS) -o $@ main.c jogo.c

testes: testes.c jogo.c jogo.h
	$(CC) $(CFLAGS) -o $@ testes.c jogo.c

test: testes
	./testes

run: cobrinha
	./cobrinha

clean:
	rm -f cobrinha testes cobrinha.exe testes.exe

.PHONY: all test run clean
