// SPDX-License-Identifier: 0BSD

///////////////////////////////////////////////////////////////////////////////
//
/// \file       tuklib_exit.c
/// \brief      Close stdout and stderr, and exit
//
//  Author:     Lasse Collin
//
///////////////////////////////////////////////////////////////////////////////

#include "tuklib_common.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if !defined(TUKLIB_DOSLIKE) && !defined(__VMS) \
		&& (!defined(__wasm__) || defined(__EMSCRIPTEN__))
#	include "mythread.h"
#endif

#include "tuklib_gettext.h"
#include "tuklib_progname.h"
#include "tuklib_exit.h"


extern void
tuklib_exit(int status, int err_status, int show_error)
{
	// Closing stdio streams can be interrupted by a signal. We are about to
	// terminate, so block signals while flushing and closing them. In threaded
	// builds this only changes the current thread's mask; liblzma worker threads
	// are created with all signals blocked.
#if !defined(TUKLIB_DOSLIKE) && !defined(__VMS) \
		&& (!defined(__wasm__) || defined(__EMSCRIPTEN__))
	{
		const int saved_errno = errno;
		sigset_t all_signals;
		if (sigfillset(&all_signals) == 0)
			mythread_sigmask(SIG_BLOCK, &all_signals, NULL);
		errno = saved_errno;
	}
#endif

	if (status != err_status) {
		// Close stdout. If something goes wrong,
		// print an error message to stderr.
		const int ferror_err = ferror(stdout);
		const int fclose_err = fclose(stdout);
		if (ferror_err || fclose_err) {
			status = err_status;

			// If it was fclose() that failed, we have the reason
			// in errno. If only ferror() indicated an error,
			// we have no idea what the reason was.
			if (show_error)
				fprintf(stderr, "%s: %s: %s\n", progname,
						_("Writing to standard "
							"output failed"),
						fclose_err ? strerror(errno)
							: _("Unknown error"));
		}
	}

	if (status != err_status) {
		// Close stderr. If something goes wrong, there's
		// nothing where we could print an error message.
		// Just set the exit status.
		const int ferror_err = ferror(stderr);
		const int fclose_err = fclose(stderr);
		if (fclose_err || ferror_err)
			status = err_status;
	}

	exit(status);
}
