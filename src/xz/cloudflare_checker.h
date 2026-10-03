// SPDX-License-Identifier: 0BSD

#ifndef XZ_CLOUDFLARE_CHECKER_H
#define XZ_CLOUDFLARE_CHECKER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

_Bool xz_cloudflare_checker_enabled(void);
void xz_cloudflare_checker_reset(void);
void xz_cloudflare_check_output(const uint8_t *data, size_t size);
void xz_cloudflare_check_output_complete(void);

#endif
