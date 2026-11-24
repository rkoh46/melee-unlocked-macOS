// Calls from the game image (a Linux AArch64 ELF) into host code built for macOS.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "mu_host.h"

// Apple's arm64 ABI expects the caller to extend arguments narrower than 32 bits; AAPCS64 (the
// game's) leaves their upper bits undefined. Replaces the host table's entries that take such an
// argument with adapters that take the register whole and narrow it. Call last, after everything
// else has put its handlers into the table.
void mu_abi_adapt_host_table(MuHostApi* host);
// Prints the fault address, pc and lr (also relative to the image) on SIGSEGV/SIGBUS, then lets the
// fault take its course. Handlers installed later (write tracking) chain to it.
void mu_abi_install_crash_report(void);
