#define _GNU_SOURCE

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>

#include "driver.h"
#include "log.h"
#include "process.h"

extern char log_buf[];
bool fatal = false;

void usage(void) {
	log_info("Usage: %s -r <resolution> -f <framerate>", program_invocation_short_name);
	log_info("Example:\n\t%s -r 1366x768 -f 10", program_invocation_short_name);
	return;
}

int main(int argc, char *argv[])
{
	int r;

	r = mlockall(MCL_FUTURE);
	if (r < 0) {
		log_warn("Failed to lock pages in memory, ignoring: %m");
	}

	memset(log_buf, 0, LOG_BUF_SIZE);
	r = setvbuf(stderr, log_buf, _IOFBF, LOG_BUF_SIZE);
	if (r) {
		log_warn("Failed to set full buffering, ignoring: %m");
	}

	int opt;
	char *res = NULL, *framerate = NULL;

	while ((opt = getopt(argc, argv, "r:f:")) != -1) {
		switch (opt) {
		case 'r':
			res = strdup(optarg);
			break;
		case 'f':
			framerate = strdup(optarg);
			break;
		default:
			usage();
			goto end;
		}
	}

	Camera c;
	r = init_cam(&c);
	if (r != IS_SUCCESS) {
		log_error("Error: Failed to initialize camera");
		goto end;
	}

	r = capture_img(&c);
	if (r != IS_SUCCESS) {
		log_error("Error: Failed to capture image");
		goto end_unref;
	}

	r = stream_loop(&c, res ? res : "1366x768", framerate ? framerate : "10");
	if (r < 0) {
		log_error("Failure in transmission of frames to worker, exiting");
		goto end_unref;
	}

end_unref:
	unref_cam(&c);
end:
	free(res);
	free(framerate);
	return r < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
