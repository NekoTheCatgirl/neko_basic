#include "signals.h"

volatile sig_atomic_t interrupted = 0;

static void handle_sigint(int sig);

void signals_init(void) {
#ifdef _WIN32
	signal(SIGINT, handle_sigint);
#else
	struct sigaction sa;
	sa.sa_handler = handle_sigint;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGINT, &sa, nullptr);
#endif
}

void signals_reset(void) {
	interrupted = 0;
}

static void handle_sigint(const int sig) {
	(void)sig;
	interrupted = 1;
}