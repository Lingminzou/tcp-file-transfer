
#include <signal.h>	   /* for signal */
#include <execinfo.h>

static void dump(void)
{
	int j, nptrs;
	void *buffer[16];
	char **strings;

	nptrs = backtrace(buffer, 16);
    
	printf("backtrace() returned %d addresses\n", nptrs);

	strings = backtrace_symbols(buffer, nptrs);
    
	if (strings == NULL) {
		perror("backtrace_symbols");
		exit(EXIT_FAILURE);
	}

	for (j = 0; j < nptrs; j++)
		printf("  [%02d] %s\n", j, strings[j]);

	free(strings);
}

static void handler(int signo)
{
    /*char buff[64] = {0x00};

    sprintf(buff,"cat /proc/%d/maps", getpid());

    system((const char*) buff);*/

	printf("\n=========>>>catch signal %d (%s) <<<=========\n", signo, (char *)strsignal(signo));
    
	printf("Dump stack start...\n");
	dump();
	printf("Dump stack end...\n");

	signal(signo, SIG_DFL);
	raise(signo);
}

static void printSignal(int signo)
{
	static int i = 0;
	
	printf("\n=========>>>catch signal %d (%s) i = %d <<<=========\n",
				signo, (char *)strsignal(signo), i++);
	printf("Dump stack start...\n");
	dump();
	printf("Dump stack end...\n");
}

void RegisterSignalForBacktrace()
{
	signal(SIGINT, handler);
	signal(SIGSEGV, handler);	
	signal(SIGUSR1, printSignal);
}
