/*
 *   Copyright (c) 2026 Fred Klassen <tcpreplay.dev at gmail dot com> - AppNeta by Broadcom
 *
 *   The Tcpreplay Suite of tools is free software: you can redistribute it
 *   and/or modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation, either version 3 of the
 *   License, or with the authors permission any later version.
 *
 *   The Tcpreplay Suite is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with the Tcpreplay Suite.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Globals that libcommon.a references but does not define, so a unit test can
 * link against it the same way the tools do.
 *
 * Each binary in src/ defines these in its own main() file, and
 * src/libtcpreplay_globals.c does the same for the installed library; this is
 * the equivalent for the test binaries. Without it the link fails with
 * "undefined reference to `debug'" - but only in a --enable-debug build,
 * since dbg()/dbgx() compile to nothing otherwise. That is exactly the
 * discrepancy that made this pass locally and fail in CI, which builds with
 * -DENABLE_DEBUG=ON.
 */

#include "defines.h"
#include "config.h"

#ifdef DEBUG
/* level used by the dbg()/dbgx() macros in --enable-debug builds */
int debug = 0;
#endif
