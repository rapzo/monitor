#include "screener.h"

static volatile sig_atomic_t stop = 0;

/**
 * Lista duplamente ligada de objetos de monitorização
 * Esta lista era uma variável local, relativa ao processo principal, contudo,
 * devido ao processamento de sinais (nomeadamente o SIGINT) não ser em cascata,
 * visto que os processos "filhos" fazem parte de um grupo diferente, não eram
 * executados e ficavam zombies
 */
DLDL *monos;

/**
 * Handler que é executado quando é lançado um sinal SIGINT (ctrl+c)
 * @param signo sinal apanhado, SIGINT neste caso
 */
void catch_sigint(int signo) {
  //char msg[] = "Shutting down...\n";
  //int size = sizeof(msg);
  DLDLNode *node;
  scrnr *element;

  //write(STDOUT_FILENO, msg, size);
  stop = 1;

  for (node = monos->first; node != NULL; node = node->next) {
    element = node->element;
    if (element != NULL && element->pid > 0) {
      kill(-element->pid, SIGQUIT);
      //printf("Killed %d group\n", element->pid);
    }
  }
}

/**
 * Handler que é executado quando é lançado um sinal SIGUSR1 para um processo.
 * @param signo sinal apanhado, SIGUSR1 neste caso
 */
void catch_sigusr(int signo) {
  //char msg[] = "Process signal! File disappeared?\n";
  //int size = sizeof(msg);

  //write(STDOUT_FILENO, msg, size);
  //printf("PID to kill: %d-%d\n", getpid(), getppid());

  kill(-getpid(), SIGQUIT);
}

/**
 * Verifica a existência do ficheiro
 * @param  fs caminho para o ficheiro
 * @return    1 se o ficheiro existir, 0 caso não seja acessível ou não existir.
 */
int file_exists(char *fs) {
  struct stat buf;

  if (stat(fs, &buf) == 0)
    return 1;
  return 0;
}

/**
 * Calcula o número de segundos que a operação vai durar de acordo com o total
 * de segundos atribuido como argumento face ao tempo atual.
 * @param  seconds duração da operação
 * @return         Total de segundos a contar do tempo atual.
 *
unsigned int calculate_op_time(unsigned int seconds) {
  unsigned int total = 0;
  time_t t;
  struct tm *tm, *limit;

  time(&t);
  tm = localtime(&t);

  return total;
}
*/

/**
 * Método utilitário que devolve uma string com data e tempo formatado segundo a
 * especificação YYYY-mm-ddTHH:MM:SS
 * @param  timer Timestamp a usar.
 * @return       String com a data apresentada na forma especificada.
 */
char *get_datetime(const time_t *timer) {
  struct tm *current = localtime(timer);
  char *result;
  result = malloc(sizeof(char) * 21);
  strftime(result, 20, "%Y-%m-%dT%H:%M:%S", current);
  return result;
}

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
int lookup_word(scrnr *entity, char *word, int duration) {
  int process_pd[2], command_pd[2], n = 0, status, i = 0;
  char buffer[MAXBUFFER], *result;
  time_t t;
  pid_t pid;
  struct sigaction actusr;

  actusr.sa_handler = catch_sigusr;
  actusr.sa_flags = 0;
  if (
    (sigemptyset(&actusr.sa_mask)) == -1 ||
    (sigaction(SIGUSR1, &actusr, NULL) == -1)
  ) {
    perror("Failed to catch SIGUSR1.");
    return -1;
  }

  if (pipe(process_pd) == -1 || pipe(command_pd) == -1) {
    perror("Pipe creation error.\n");
    return -1;
  }

  for (i = 0; i < 2; i++) {
    if ((pid = fork()) <= 0)
      break;
  }

  if (pid == -1) {

    perror("Creating control fork.");
    return -1;

  } else if (pid == 0) {
    if (i == 0) {

      setpgid(0, entity->pid);
      if (run_tail(entity, command_pd[1]) == -1) {
        return -1;
      }

    } else if (i == 1) {

      setpgid(0, entity->pid);
      if (run_grep(entity, word, command_pd[0], process_pd[1]) == -1) {
        return -1;
      }

    }
  } else if (pid > 0) {

    while (!stop) {
      n = read(process_pd[0], buffer, MAXBUFFER);

      if (n > 0) {
        // Tempo da leitura
        // ok, pode ser ums milésimos de segundo diferentes da escrita no 
        // ficheiro, mas pouco
        t = time(NULL);

        // Remove o último '\n' comum das linhas de execuções como o tail e o 
        // grep
        buffer[n - 1] = '\0';
        result = malloc(sizeof(char) * n);
        strcpy(result, buffer);

        // É melhor não montar altas strings quando o output pode ser feito aqui
        //sprintf(result, "%s - %s - \"%s\"", get_datetime(&t), entity->name, tmp);

        printf("%s - %s - \"%s\"\n", get_datetime(&t), entity->name, result);
        free(result);
      }

    }
    // Comentar isto porque está a deixar processos defuncts :-\
    // wait(&status);
  }

  return 0;
}

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
int run_tail(scrnr *entity, int pwrite) {
  assert(entity->path);

  char *tail_args[6] = {
    "tail", "-n", "0", "-f", entity->path, NULL
  };

  if (pwrite != STDOUT_FILENO) {
    if (dup2(pwrite, STDOUT_FILENO) != STDOUT_FILENO) {
      perror("Piping tail output to the pipe.");
      return -1;
    }
  }
  close(pwrite);

  if (execvp(tail_args[0], tail_args)) {
    perror("Failed to execute `tail`");
  }
  return 0;
}

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
int run_grep(scrnr *entity, char *word, int pread, int pwrite) {
  assert(word);

  char *grep_args[4] = {
    "grep", "--line-buffered", word, NULL
  };

  if (pread != STDIN_FILENO) {
    if (dup2(pread, STDIN_FILENO) != STDIN_FILENO) {
      perror("Piping grep output to the pipe.");
      return -1;
    }
    close(pread);

    if (dup2(pwrite, STDOUT_FILENO) != STDOUT_FILENO) {
      perror("Piping grep output to the main pipe.");
      return -1;
    }
    close(pwrite);
  }

  if (execvp(grep_args[0], grep_args)) {
    perror("Failed to execute `grep`");
  }

  return 0;
}

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
int check_entities(DLDL *monos) {
  long timedif;
  struct timeval tpstart, tpend;
  DLDLNode *node;
  scrnr *element;

  while (!stop) {
    if (gettimeofday(&tpstart, NULL)) {
      perror("Error getting start time.");
      return -1;
    }

    for (node = monos->first; node != NULL; node = node->next) {
      element = node->element;
      if (!file_exists(element->path)) {
        printf("File deleted: %s\tGonna kill: %d\n", element->name, element->pid);
        kill(element->pid, SIGUSR1);
        dldl_remove(monos, node);
      }
    }

    if (monos->size <= 0) {
      stop = 1;

      // isto é um hack para garantir que o sinal é apanhado antes de sair, para
      // efeitos de beleza de output.
      sleep(1);
      return 0;
    }

    if (gettimeofday(&tpend, NULL)) {
      perror("Error getting end time.");
      return -1;
    }

    /**
     * Este resultado é fruto dos dois pedidos de tempo `gettimeofday` para se 
     * obter o tempo de duranção do script de verificação de ficheiros de forma
     * que o número de ficheiros a monitorizar não influencie o intervalo de
     * verificação. O resultado é interpretado na casa dos microsegundos.
     */
    timedif = tpend.tv_sec - tpstart.tv_sec + tpend.tv_usec - tpstart.tv_usec;
    usleep((MILLION * 5) - timedif);
  }
  return 0;
}

/**
 * Temporizador que termina quando acabar o tempo
 * @param  duration tempo de execução em segundos
 * @return         devolve zero quando acabar o tempo.
 */
int countdown_timer(int duration) {
  int elapsed = 0;
  do {
    sleep(1);
    elapsed++;
    // printf("ETA %d\n", duration - elapsed);
  } while (elapsed < duration && !stop);

  return 0;
}

/**
 * Arranca a mota.
 * @param  argc número de argumentos fornecidos.
 * @param  argv Vetor de strings de argumentos.
 * @return      0 em caso de sucesso, -1 em caso de erro.
 */
int main(int argc, char *argv[]) {
  int quit = 0;
  unsigned int i = 0, j = 0, duration = 0;
  struct sigaction actint;
  DLDLNode *node;
  scrnr *element;
  pid_t pid;

  monos = dldl_create();

  actint.sa_handler = catch_sigint;
  actint.sa_flags = 0;
  if (
    (sigemptyset(&actint.sa_mask)) == -1 ||
    (sigaction(SIGINT, &actint, NULL) == -1)
  ) {
    perror("Failed to catch SIGINT.");
    return -1;
  }

  if (argc < 3) {
    printf("Not enough parameters.\n");
    quit = 1;
  } else if( argc > MAXENTRIES + 1) {
    printf("Too much arguments! Hard limit defined as %d.\n", MAXENTRIES);
    quit = 1;
  }

  if (quit) {
    printf(
      "usage: ./monitor <time in seconds> <word to look for> <file #1> \
      <file #2> ... <file #n>\n"
    );
    return -1;
  }

  // Inicia o temporizador
  // limit = (struct tm *)malloc(sizeof(struct tm));
  // limit->tm_sec = atoi(argv[1]);
  // printf("T minus %d to STOP.\n", limit->tm_sec);
  duration = atoi(argv[1]);

  // Reserva espaço para a estrutura de controlo
  for (i = 3, j = 0; i < argc; i++, j++) {
    errno = 0;
    element = malloc(sizeof(scrnr));
    element->id = j;
    element->name = strdup(argv[i]);
    element->path = realpath(argv[i], NULL);
    element->updates = 0;

    if (errno != 0) {
      printf("Problems with file `%s`.\n", element->name);
      perror("File issue");
      continue;
    }

    dldl_push(monos, element);
  }

  for (node = monos->first; node != NULL; node = node->next) {
    element = node->element;
    if ((pid = fork()) <= 0) {
      break;
    } else {
      element->pid = pid;
      continue;
    }
  }

  if (pid < 0) {
    perror("Forking screener slaves.");
  } else if (pid == 0) {
    element->pid = getpid();
    setpgid(0, 0);

    printf(
      "Monitoring `%s` in process %d\nPath: %s - File exists? %s\n",
      element->name,
      getpid(),
      element->path,
      file_exists(element->path) ? "Yes" : "No"
    );

    if (lookup_word(element, argv[2], duration) == -1) {
      return -1;
    }

  } else if (pid > 0) {
    // Aqui só chega o pai, aka processo principal

    if ((pid = fork()) != -1) {
      if (pid == 0) {

        setpgid(0, getppid());
        return check_entities(monos);

      } else {

        if (countdown_timer(duration) == 0) {
          kill(0, SIGINT);
        }

      }
    } else {
      perror("Forking control slave.");
    }

    if (dldl_destroy(monos)) {
      printf("Shutdown successfully. Thank you for chosing us!\n");
    }
  }

  return 0;
}

