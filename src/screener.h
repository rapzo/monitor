#ifndef _SCREENER_H_
#define _SCREENER_H_

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include "dldl.h"

#define MILLION 1000000L
#define MAXENTRIES 128
#define MAXBUFFER 1014

typedef struct f_screener {
    unsigned int id;
    char *name;
    char *path;
    int updates;
    pid_t pid;
} scrnr;

/**
 * Handler que é executado quando é lançado um sinal SIGINT (ctrl+c)
 * @param signo sinal apanhado, SIGINT neste caso
 */
void catch_sigint(int signo);

/**
 * Handler que é executado quando é lançado um sinal SIGUSR1 para um processo.
 * @param signo sinal apanhado, SIGUSR1 neste caso
 */
void catch_sigusr(int signo);

/**
 * Verifica a existência do ficheiro
 * @param  fs caminho para o ficheiro
 * @return    1 se o ficheiro existir, 0 caso não seja acessível ou não existir.
 */
int file_exists(char *fs);

/**
 * Calcula o número de segundos que a operação vai durar de acordo com o total
 * de segundos atribuido como argumento face ao tempo atual.
 * @param  seconds duração da operação
 * @return         Total de segundos a contar do tempo atual.
 */
unsigned int calculate_op_time(unsigned int seconds);

/**
 * Método utilitário que devolve uma string com data e tempo formatado segundo a
 * especificação YYYY-mm-ddTHH:MM:SS
 * @param  timer Timestamp a usar.
 * @return       String com a data apresentada na forma especificada.
 */
char *get_datetime(const time_t *timer);

/**
 * Método que executa a procura de uma palavra numa entidade que contém
 * informação sobre o ficheiro a procurar entre outros parametros. Escreve o
 * resultado da execução na pipe referida como último argumento e retorna o
 * ńúmero de bytes escritos na pipe.
 * @param  entity Entidade a usar
 * @param  word   Palavra a procurar
 * @param  pipe   Pipe de escrita
 * @return        Número de bytes escritos na pipe. -1 em caso de erro.
 */
int lookup_word(scrnr *entity, char *word, int duration);

/**
 * Processo que executa a chamada de sistema da familia `exec`, neste caso, a
 * execução do comando `tail -n 0 -f <filepath>` que assume controlo do processo
 * e fica em contínua execução.
 * @param  entity a entidade de monitorização
 * @param  pwrite a `pipe` de escrita dos resultados
 * @return        teoricamente este método só devolve -1 em caso de erro, uma
 * vez que métodos da família `exec` assumem controlo do atual processo e, por 
 * norma, não devolvem valor exceto em caso de erro (o último `return*  é uma 
 * formalidade).
 */
int run_tail(scrnr *entity, int pwrite);

/**
 * Processo que executa a chamada de sistema da familia `exec`, neste caso, a
 * execução do comando `grep --line-buffered <word>` que assume controlo do
 * processo e fica em contínua execução.
 * @param  entity a entidade de monitorização
 * @param  word   a palavra a procurar
 * @param  pread a `pipe` de leitura dos resultados
 * @param  pwrite a `pipe` de escrita dos resultados
 * @return        teoricamente este método só devolve -1 em caso de erro, uma
 * vez que métodos da família `exec` assumem controlo do atual processo e, por 
 * norma, não devolvem valor exceto em caso de erro (o último `return*  é uma 
 * formalidade).
 */
int run_grep(scrnr *entity, char *word, int pread, int pwrite);


/**
 * Método que é executado durante a execução do programa e que tem função
 * verificar se os ficheiros foram removidos ou se desapareceram por algum
 * motivo. Se isso acontecer é enviado um sinal SIGUSR1 ao processo de controlo
 * do ficheiro para este terminar a execução.
 * Se não houver mais entidades de monitorização disponíveis, o método devolve
 * zero e prossegue com a terminação da aplicação.
 * @param  monos Entidades de monitorização
 * @return       Devolve -1 em caso de erro ou 0 em caso de fim de execução.
 */
int check_entities(DLDL *monos);

#endif