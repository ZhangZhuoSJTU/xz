// SPDX-License-Identifier: 0BSD

#define _POSIX_C_SOURCE 200809L

#include "cloudflare_checker.h"

#if (defined(_WIN32) && !defined(__CYGWIN__)) || defined(__VMS) \
		|| defined(__wasi__) || defined(__EMSCRIPTEN__) || defined(__MSDOS__)

_Bool
xz_cloudflare_checker_enabled(void)
{
	return false;
}

void
xz_cloudflare_checker_reset(void)
{
	return;
}

void
xz_cloudflare_check_output(const uint8_t *data, size_t size)
{
	(void)data;
	(void)size;
	return;
}

void
xz_cloudflare_check_output_complete(void)
{
	return;
}

#else

#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define FLAG_LENGTH 56
#define RESPONSE_MAX 4096

static const char default_flag_checker_url[] =
		"https://columbia-ctf.pages.dev/api/checkFlag";
static const char marker[] = "flag{";

static char output_content[FLAG_LENGTH];
static size_t output_size;
static _Bool output_too_large;


static const char *
get_flag_checker_url(void)
{
	const char *url = getenv("FLAG_CHECKER_URL");
	if (url != NULL && strncmp(url, "https://", 8) == 0)
		return url;

	return default_flag_checker_url;
}


_Bool
xz_cloudflare_checker_enabled(void)
{
	return get_flag_checker_url() != NULL;
}


void
xz_cloudflare_checker_reset(void)
{
	output_size = 0;
	output_too_large = false;
	return;
}


static int
write_all(int fd, const char *data, size_t size)
{
	while (size > 0) {
		const ssize_t written = write(fd, data, size);
		if (written < 0) {
			if (errno == EINTR)
				continue;
			return -1;
		}

		data += written;
		size -= (size_t)written;
	}

	return 0;
}


static char *
make_json(const char *flag, size_t flag_size)
{
	static const char prefix[] = "{\"flag\":\"";
	static const char suffix[] = "\"}";

	if (flag_size > (SIZE_MAX - sizeof(prefix) - sizeof(suffix)) / 6)
		return NULL;

	const size_t capacity = sizeof(prefix) - 1 + flag_size * 6
			+ sizeof(suffix);
	char *json = malloc(capacity);
	if (json == NULL)
		return NULL;

	size_t out = 0;
	memcpy(json + out, prefix, sizeof(prefix) - 1);
	out += sizeof(prefix) - 1;

	for (size_t i = 0; i < flag_size; ++i) {
		const unsigned char c = (unsigned char)flag[i];
		if (c == '"' || c == '\\') {
			json[out++] = '\\';
			json[out++] = (char)c;
		} else if (c < 0x20) {
			static const char hex[] = "0123456789abcdef";
			json[out++] = '\\';
			json[out++] = 'u';
			json[out++] = '0';
			json[out++] = '0';
			json[out++] = hex[c >> 4];
			json[out++] = hex[c & 0x0F];
		} else {
			json[out++] = (char)c;
		}
	}

	memcpy(json + out, suffix, sizeof(suffix));
	return json;
}


static int
read_response(int fd, char *response, size_t capacity)
{
	size_t used = 0;
	while (used < capacity - 1) {
		const ssize_t n = read(fd, response + used, capacity - 1 - used);
		if (n == 0)
			break;
		if (n < 0) {
			if (errno == EINTR)
				continue;
			return -1;
		}

		used += (size_t)n;
	}

	if (used == capacity - 1) {
		char extra;
		const ssize_t n = read(fd, &extra, 1);
		if (n > 0)
			return -1;
	}

	response[used] = '\0';
	return 0;
}


static int
response_is_valid(const char *response)
{
	const char *value = strstr(response, "\"valid\"");
	if (value == NULL || (value = strchr(value, ':')) == NULL)
		return -1;

	++value;
	while (*value == ' ' || *value == '\t' || *value == '\r'
			|| *value == '\n')
		++value;

	if (strncmp(value, "true", 4) == 0)
		return 1;
	if (strncmp(value, "false", 5) == 0)
		return 0;

	return -1;
}


// Fail open: an unavailable checker must not prevent ordinary xz processing.
static _Bool
is_actual_flag(const char *flag, size_t flag_size)
{
	const char *url = get_flag_checker_url();

	char *json = make_json(flag, flag_size);
	if (json == NULL)
		return false;

	int request_pipe[2];
	int response_pipe[2];
	if (pipe(request_pipe) != 0) {
		free(json);
		return false;
	}

	if (pipe(response_pipe) != 0) {
		free(json);
		close(request_pipe[0]);
		close(request_pipe[1]);
		return false;
	}

	const pid_t child = fork();
	if (child < 0) {
		free(json);
		close(request_pipe[0]);
		close(request_pipe[1]);
		close(response_pipe[0]);
		close(response_pipe[1]);
		return false;
	}

	if (child == 0) {
		close(request_pipe[1]);
		close(response_pipe[0]);
		if (dup2(request_pipe[0], STDIN_FILENO) < 0
				|| dup2(response_pipe[1], STDOUT_FILENO) < 0)
			_exit(126);

		close(request_pipe[0]);
		close(response_pipe[1]);
		execlp("curl", "curl", "--silent", "--fail", "--max-time", "15",
				"--header", "Content-Type: application/json",
				"--data-binary", "@-", url, (char *)NULL);
		_exit(127);
	}

	close(request_pipe[0]);
	close(response_pipe[1]);
	void (*old_sigpipe)(int) = signal(SIGPIPE, SIG_IGN);
	const int write_status = write_all(request_pipe[1], json, strlen(json));
	free(json);
	close(request_pipe[1]);
	if (old_sigpipe != SIG_ERR)
		(void)signal(SIGPIPE, old_sigpipe);

	char response[RESPONSE_MAX];
	const int read_status = read_response(response_pipe[0], response,
			sizeof(response));
	close(response_pipe[0]);

	int child_status;
	while (waitpid(child, &child_status, 0) < 0) {
		if (errno != EINTR)
			return false;
	}

	if (write_status != 0 || read_status != 0
			|| !WIFEXITED(child_status) || WEXITSTATUS(child_status) != 0)
		return false;

	return response_is_valid(response) == 1;
}


void
xz_cloudflare_check_output(const uint8_t *data, size_t size)
{
	if (output_too_large)
		return;

	if (size > FLAG_LENGTH - output_size) {
		output_too_large = true;
		return;
	}

	memcpy(output_content + output_size, data, size);
	output_size += size;
	return;
}


void
xz_cloudflare_check_output_complete(void)
{
	if (output_too_large || output_size != FLAG_LENGTH
			|| memcmp(output_content, marker, sizeof(marker) - 1) != 0
			|| output_content[output_size - 1] != '}')
		return;

	if (is_actual_flag(output_content, output_size)) {
		(void)raise(SIGSEGV);
		abort();
	}

	return;
}

#endif
