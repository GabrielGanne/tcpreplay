# Host / library / feature detection for the tcpreplay build.
#
#   defined here and consumed afterwards by the root file and src/:
#       config              - populated with every HAVE_*/ENABLE_*/SIZEOF_* entry
#       host                - host_machine.system()
#       static_link         - bool (-Dstatic-link)
#       libpcap, lpcapinc   - libpcap dependency and its include directory
#       system_libs         - extra system libraries linked into every program
#       libdnet_dep, have_libdnet, libdnet_version
#       enable_netmap, enable_fragroute, enable_tcpbridge,
#       enable_tcpliveplay, enable_tcpdump
#       have_bpf, have_pf, have_libbpf, have_libxdp, have_tx_ring,
#       have_pcap_inject, have_pcap_sendpacket, have_pcap_setnonblock, ...
#
# Keep ONLY host/library/feature probing in this file.  User build-option to
# compiler-flag handling (debug, gprof, dmalloc, efence, macOS SDK, sanitizers)
# and the final config.h generation deliberately stay in the root meson.build.

check_headers = [
    'architecture/i386/pio.h',
    'arpa/inet.h',
    'fcntl.h',
    'fts.h',
    'inttypes.h',
    'libintl.h',
    'netinet/in.h',
    'netinet/in_systm.h',
    'poll.h',
    'sched.h',
    'signal.h',
    'stddef.h',
    'stdint.h',
    'string.h',
    'strings.h',
    'sys/file.h',
    'sys/ioctl.h',
    'sys/io.h',
    'sys/param.h',
    'sys/poll.h',
    'sys/select.h',
    'sys/socket.h',
    'sys/systeminfo.h',
    'sys/time.h',
    'sys/types.h',
    'unistd.h',
]
foreach h : check_headers
    if cc.check_header(h)
        config.set('HAVE_' + h.underscorify().to_upper(), 1)
    endif
endforeach

# netmap detection:  netmap ships as a source tree rather than a package
enable_netmap = false
netmap_args = []
netmap_opt = get_option('netmap')
if not netmap_opt.disabled()
    netmap_search_dirs = [
        '/opt/netmap',
        '/usr/src/netmap-release',
        '/usr/src/netmap',
        '/usr/local/src/netmap-release',
        '/usr/local/src/netmap',
        '/usr/include',
        '/usr/local/include',
    ]
    # -Dnetmap-dir takes precedence
    if get_option('netmap-dir') != ''
        netmap_search_dirs = [get_option('netmap-dir')] + netmap_search_dirs
    endif
    netmap_incdir = ''
    foreach d : netmap_search_dirs
        if netmap_incdir == '' and fs.is_file(d / 'sys/net/netmap.h')
            netmap_incdir = d / 'sys'
        elif netmap_incdir == '' and fs.is_file(d / 'net/netmap.h')
            netmap_incdir = d
        endif
    endforeach
    if netmap_incdir != ''
        enable_netmap = true
        # Applied project-wide, exactly as configure.ac appends to CFLAGS.
        netmap_args = ['-DNETMAP_WITH_LIBS', '-DND', '-I' + netmap_incdir]
        config.set('HAVE_NETMAP', 1)
        # Version-specific netmap feature probes (the source #ifdefs on these).
        netmap_prefix = '''#include <stdint.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/netmap.h>
'''
        if cc.compiles(netmap_prefix + 'int main(void){\n#ifndef NR_REG_MASK\n#error no NR_REG_MASK\n#endif\nreturn 0;}', args : netmap_args, name : 'netmap NR_REG_MASK')
            config.set('HAVE_NETMAP_NR_REG', 1)
        endif
        if cc.compiles(netmap_prefix + 'int main(void){ struct nmreq nmr; nmr.nr_flags = 0; return 0; }', args : netmap_args, name : 'netmap struct nmreq nr_flags')
            config.set('HAVE_NETMAP_NR_FLAGS', 1)
        endif
        if cc.compiles(netmap_prefix + 'int main(void){ struct netmap_ring r; r.head = r.tail = 0; return 0; }', args : netmap_args, name : 'netmap ring head/tail')
            config.set('HAVE_NETMAP_RING_HEAD_TAIL', 1)
        endif
    endif
endif
if netmap_opt.enabled() and not enable_netmap
    error('netmap requested (-Dnetmap=enabled) but netmap headers were not found')
endif
if netmap_args.length() > 0
    add_project_arguments(netmap_args, language : 'c')
endif

config.set('SIZEOF_CHAR', cc.sizeof('char'))
config.set('SIZEOF_INT', cc.sizeof('int'))
config.set('SIZEOF_SHORT', cc.sizeof('short'))
config.set('SIZEOF_LONG', cc.sizeof('long'))
config.set('SIZEOF_SIZE_T', cc.sizeof('size_t'))
config.set('SIZEOF_SSIZE_T', cc.sizeof('ssize_t'))
config.set('SIZEOF_VOID_P', cc.sizeof('void*'))
config.set('SIZEOF_CHARP', cc.sizeof('void*'))

# AC_CHECK_TYPE / check_type_size probes.  defines.h.in and, more importantly,
# libopts/compat/compat.h provide fallback typedefs for these types guarded by
# HAVE_<TYPE>; on a modern host the types already exist, so we must advertise
# them or compat.h emits typedefs that conflict with the system ones.
type_prefix = '''#include <sys/types.h>
#include <stddef.h>
#include <stdint.h>
#include <wchar.h>
#include <unistd.h>'''
check_types = [
    'int8_t', 'uint8_t', 'u_int8_t',
    'int16_t', 'uint16_t', 'u_int16_t',
    'int32_t', 'uint32_t', 'u_int32_t',
    'int64_t', 'uint64_t', 'u_int64_t',
    'size_t', 'ssize_t',
    'intptr_t', 'uintptr_t', 'ptrdiff_t',
    'wchar_t', 'wint_t',
    'u_char', 'u_short',
# uint_t is intentionally omitted: glibc has no such type, so leaving
# HAVE_UINT_T undefined lets compat.h define it harmlessly.
    'u_int',
    'pid_t',
]
foreach t : check_types
    if cc.has_type(t, prefix : type_prefix)
        config.set('HAVE_' + t.underscorify().to_upper(), 1)
    endif
endforeach

# AC_C_BIGENDIAN -> WORDS_BIGENDIAN (drives the ntohll/htonll fallback and the
# checksum/capinfo byte-order code; left undefined on little-endian hosts).
if host_machine.endian() == 'big'
    config.set('WORDS_BIGENDIAN', 1)
endif

# Strict-alignment detection -> FORCE_ALIGN.
# tcpedit/get.c and the DLT plugins use it to decide whether
# packet fields must be copied before dereferencing.  Architectures known to
# fault on unaligned access force it on; macOS never does; elsewhere we run a
# small probe and assume strict alignment when the probe cannot be run (e.g.
# when cross-compiling) which is the safe default.
#
# Force the aligned-copy code paths on whenever a sanitizer is active so UBSan builds are clean.
force_align = false
if get_option('b_sanitize') != 'none'
    force_align = true
elif host_machine.system() == 'darwin'
    force_align = false
elif host_machine.cpu_family() in ['aarch64', 'arm', 'mips', 'mips64', 'sparc', 'sparc64', 'ia64', 'alpha', 'parisc']
    force_align = true
elif meson.can_run_host_binaries()
    align_probe = cc.run('''
#include <stdio.h>
unsigned char a[5] = { 1, 2, 3, 4, 5 };
int main(void) {
    unsigned int i = *(unsigned int *)&a[1];
    printf("%u\n", i);
    return 0;
}
''', name : 'unaligned memory access')
    if not align_probe.compiled() or align_probe.returncode() != 0
        force_align = true
    endif
else
    force_align = true
endif
if force_align
    config.set('FORCE_ALIGN', 1)
endif

# AC_HEADER_STDBOOL
if cc.has_header('stdbool.h')
    config.set('HAVE_STDBOOL_H', 1)
endif
if cc.has_type('_Bool')
    config.set('HAVE__BOOL', 1)
endif

# net/route.h needs sys/socket.h + net/if.h as prerequisites.
# HAVE_NET_ROUTE_H is consumed by the interface code.
if cc.check_header('net/route.h', prefix : '#include <sys/socket.h>\n#include <net/if.h>')
    config.set('HAVE_NET_ROUTE_H', 1)
endif

# Function checks (only those the source actually #ifdefs on).  ntohll is absent
# on glibc (HAVE_NTOHLL stays undefined -> defines.h provides the fallback);
# select drives the sleep/timing paths; inet_ntoa is an inet_ntop fallback.
check_functions = [
    'inet_aton',
    'inet_pton',
    'inet_ntop',
    'inet_addr',
    'inet_ntoa',
    'select',
    'ntohll',
]
foreach f : check_functions
    if cc.has_function(f)
        config.set('HAVE_' + f.underscorify().to_upper(), 1)
    endif
endforeach

# strlcpy/strlcat are provided by libc on the BSDs and recent glibc (>= 2.38).
# When they are missing we fall back to the bundled implementation in lib/
# (see lib/meson.build); HAVE_STRLCPY also controls whether defines.h pulls in
# lib/strlcpy.h.
have_strlcpy = cc.has_function('strlcpy')
if have_strlcpy
    config.set('HAVE_STRLCPY', 1)
endif
if cc.has_function('strlcat')
    config.set('HAVE_STRLCAT', 1)
endif

# ---------------------------------------------------------------------------
# Feature probes for the bundled AutoOpts runtime (libopts/).
#
# GNU autogen (the AutoOpts *code generator*) is EOL and replaced by python3
# (scripts/autoopts, see src/meson.build), but the AutoOpts *runtime* library
# is still vendored and compiled from libopts/.  Its compat layer keys on the
# usual HAVE_*_H / HAVE_<func> config.h symbols: where a symbol is undefined it
# falls back to a bundled reimplementation (e.g. libopts/compat/snprintf.c),
# and libopts/compat/compat.h needs HAVE_DIRENT_H to pull in <dirent.h> for the
# directory walk in compat/pathfind.c.  These probes mirror cmake/
# ConfigureChecks.cmake (and the autotools configure.ac / libopts.m4 checks) so
# meson's config.h matches what the CMake and autotools builds produce.
libopts_headers = [
  'dirent.h',
  'dlfcn.h',
  'errno.h',
  'libgen.h',
  'limits.h',
  'memory.h',
  'runetype.h',
  'setjmp.h',
  'stdarg.h',
  'stdio.h',
  'stdlib.h',
  'sysexits.h',
  'sys/limits.h',
  'sys/mman.h',
  'sys/procset.h',
  'sys/stat.h',
  'sys/stropts.h',
  'sys/un.h',
  'sys/wait.h',
  'utime.h',
  'values.h',
  'varargs.h',
  'wchar.h',
]
foreach h : libopts_headers
    if cc.check_header(h)
        config.set('HAVE_' + h.underscorify().to_upper(), 1)
    endif
endforeach

# pathfind() is deliberately absent from this list: it does not exist on glibc,
# so leaving HAVE_PATHFIND undefined lets libopts compile its bundled
# compat/pathfind.c fallback (which HAVE_DIRENT_H above makes buildable).
libopts_functions = [
  'alarm',
  'atexit',
  'bzero',
  'canonicalize_file_name',
  'chmod',
  'ctime',
  'dup2',
  'fchmod',
  'fseeko',
  'fstat',
  'gethostbyname',
  'getpagesize',
  'gettimeofday',
  'ioperm',
  'memmove',
  'memset',
  'mmap',
  'munmap',
  'pow',
  'putenv',
  'realpath',
  'regcomp',
  'snprintf',
  'strcasecmp',
  'strchr',
  'strcspn',
  'strdup',
  'strerror',
  'strftime',
  'strncpy',
  'strpbrk',
  'strrchr',
  'strsignal',
  'strspn',
  'strstr',
  'strtol',
  'strtoul',
  'strtoull',
  'vprintf',
  'vsnprintf',
]
foreach f : libopts_functions
    if cc.has_function(f)
        config.set('HAVE_' + f.underscorify().to_upper(), 1)
    endif
endforeach

# HAVE_WORKING_FORK/VFORK gate the fork-based paths in libopts/makeshell.c
# (autoopts.h derives them from fork()/vfork()).
if cc.has_function('fork')
    config.set('HAVE_WORKING_FORK', 1)
endif
if cc.has_function('vfork')
    config.set('HAVE_WORKING_VFORK', 1)
endif

# POSIX_SHELL is the shell libopts/makeshell.c bakes into generated shell
# option parsers.  configure.ac probes for a POSIX sh; /bin/sh is guaranteed to
# exist on every platform meson targets here, matching the CMake fallback.
config.set_quoted('POSIX_SHELL', '/bin/sh')

# libpcap discovery.  Default is pkg-config (the meson default); -Dpcapconfig
# selects the pcap-config tool instead.
# -Dstatic-link prefers the static libpcap.
static_link = get_option('static-link')
if static_link
    config.set('ENABLE_STATIC_LINK', 1)
endif
if get_option('pcapconfig')
    libpcap = dependency('pcap', method : 'config-tool', static : static_link)
else
    libpcap = dependency('libpcap', static : static_link)
endif
lpcapinc = libpcap.get_variable(pkgconfig : 'includedir', configtool : 'includedir', default_value : '/usr/include')
# manually retrieve libpca include dir for compatibility with autotools
# build system
config.set('LPCAPINC', lpcapinc / 'pcap.h')

# Decide how src pulls in libpcap's BPF definitions.
# INCLUDE_PCAP_BPF_HEADER is a @TOKEN@ path substitution in defines.h.in
# (#include <@INCLUDE_PCAP_BPF_HEADER@>), so it must always be the bpf header
# path -- never a bare 1.  When the compiler cannot include <pcap-bpf.h>
# directly we set PCAP_DONT_INCLUDE_PCAP_BPF_H so pcap.h is left to pull it in.
config.set('INCLUDE_PCAP_BPF_HEADER', lpcapinc / 'pcap-bpf.h')
if not cc.has_header('pcap-bpf.h')
    config.set('PCAP_DONT_INCLUDE_PCAP_BPF_H', 1)
endif

have_pcap_inject = cc.has_header_symbol(
    lpcapinc / 'pcap.h',
    'pcap_inject',
)
message('pcap_inject() sending support: @0@'.format(have_pcap_inject))
# Tested with #ifdef in the sources, so define to 1 only when present (matching
# autotools' AC_DEFINE); leaving it unset keeps #ifdef false when absent.
if have_pcap_inject
    config.set('HAVE_PCAP_INJECT', 1)
endif

# Additional libpcap capability probes consumed by the source.
if cc.has_header_symbol(lpcapinc / 'pcap.h', 'pcap_snapshot')
    config.set('HAVE_PCAP_SNAPSHOT', 1)
endif
# pcap_version is an exported variable, not a function, so probe it with a link
# test rather than has_header_symbol.
if cc.links('''
extern char pcap_version[];
int main(void) { return pcap_version[0]; }
''', dependencies : libpcap, name : 'pcap_version[]')
    config.set('HAVE_PCAP_VERSION', 1)
endif

# pcap_datalink_val_to_description().  When absent, fakepcap.c provides its own implementation
if cc.has_header_symbol(lpcapinc / 'pcap.h', 'pcap_datalink_val_to_description')
    config.set('HAVE_DLT_VAL_TO_DESC', 1)
endif

# DLT type availability
foreach dlt : ['DLT_LINUX_SLL', 'DLT_LINUX_SLL2', 'DLT_C_HDLC']
    if cc.compiles('#include "@0@"\nint main(void) { return @1@; }'.format(lpcapinc / 'pcap.h', dlt), name : dlt + ' in libpcap')
        config.set('HAVE_' + dlt, 1)
    endif
endforeach

# netmap-aware libpcap: detected by the presence of
# pcap-netmap.c alongside the headers in a libpcap source/build tree.
if fs.is_file(lpcapinc / 'pcap-netmap.c')
    config.set('HAVE_LIBPCAP_NETMAP', 1)
endif

if get_option('64bits')
  config.set('ENABLE_64BITS', 1)
endif

# tuntap device support: Linux uses linux/if_tun.h, every
# other OS uses net/if_tun.h.  -Dtuntap=enabled requires it; =disabled skips it.
tuntap_opt = get_option('tuntap')
have_tuntap = false
if not tuntap_opt.disabled()
    if host_machine.system() == 'linux'
        have_tuntap = cc.check_header('linux/if_tun.h')
    else
        have_tuntap = cc.check_header('net/if_tun.h')
    endif
endif
if tuntap_opt.enabled() and not have_tuntap
    error('tuntap requested (-Dtuntap=enabled) but if_tun.h was not found')
endif
if have_tuntap
    config.set('HAVE_TUNTAP', 1)
endif

host = host_machine.system()
if host == 'linux'
  config.set('HAVE_LINUX', 1)
elif host == 'sunos'  #  illumos and Solaris
  config.set('HAVE_SOLARIS', 1)
elif host == 'darwin'
  config.set('HAVE_DARWIN', 1)
elif host == 'openbsd'
  config.set('HAVE_OPENBSD', 1)
elif host == 'freebsd'
  config.set('HAVE_FREEBSD', 1)
elif host == 'cygwin'
  config.set('HAVE_CYGWIN', 1)
else
  error(host + ' is either unknown or unsupported')
endif

# pcap_findalldevs() interface enumeration.  autotools
# only disables it on macOS paired with a pre-0.9.6 libpcap (an old bug where
# pcap_findalldevs() disassociates the Wi-Fi NIC).  Every libpcap shipped today
# is far newer, so enable it unconditionally; the source #ifdefs on
# ENABLE_PCAP_FINDALLDEVS (e.g. tcpreplay_args.c, tcpbridge_args.c).
config.set('ENABLE_PCAP_FINDALLDEVS', 1)

# tcpdump is only needed for the verbose-decode feature; -Dtcpdump-path overrides
# auto-discovery.  A missing tcpdump is non-fatal.
tcpdump_opt = get_option('tcpdump')
tcpdump_found = false
if not tcpdump_opt.disabled()
    tcpdump_path = get_option('tcpdump-path')
    if tcpdump_path != '' and not fs.is_dir(tcpdump_path)
        tcpdump_bin = find_program(tcpdump_path, required : tcpdump_opt)
    else
        # manually add those paths when looking to align with autotools behavior
        tcpdump_dirs = ['/usr/sbin', '/sbin', '/usr/local/sbin']
        if tcpdump_path != ''
            tcpdump_dirs = [tcpdump_path] + tcpdump_dirs
        endif
        tcpdump_bin = find_program('tcpdump', dirs : tcpdump_dirs, required : tcpdump_opt)
    endif
    tcpdump_found = tcpdump_bin.found()
endif
if tcpdump_found
    config.set('HAVE_TCPDUMP', 1)
    config.set('TCPDUMP_BINARY', '"@0@"'.format(tcpdump_bin.full_path()))
elif not tcpdump_opt.disabled()
    warning('tcpdump not found; the verbose-decode feature will be disabled. Use -Dtcpdump-path=PATH to specify it.')
endif

# The verbose-decode feature (ENABLE_VERBOSE, src/common/tcpdump.c) pipes packets
# to the tcpdump binary through a pcap dumper, so it needs both tcpdump and
# pcap_dump_fopen().
have_pcap_dump_fopen = cc.has_header_symbol(lpcapinc / 'pcap.h', 'pcap_dump_fopen')
enable_tcpdump = tcpdump_found and have_pcap_dump_fopen
if enable_tcpdump
    config.set('ENABLE_VERBOSE', 1)
elif tcpdump_opt.enabled()
    error('tcpdump requested (-Dtcpdump=enabled) but tcpdump and pcap_dump_fopen() are not both available')
endif

pcap_open_offline_with_tstamp_precision_test_proc = '''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "@0@"

int main(void) {
  pcap_t *pcap;
  char ebuf[PCAP_ERRBUF_SIZE];
  pcap = pcap_open_offline_with_tstamp_precision("fake.pcap", PCAP_TSTAMP_PRECISION_NANO, &ebuf[0]);
  return 0;
}
'''.format(lpcapinc / 'pcap.h')
pcap_open_offline_has_tstamp_precision = cc.compiles(
    pcap_open_offline_with_tstamp_precision_test_proc,
    name : 'pcap_open_offline_with_tstamp_precision_test_proc'
)
if pcap_open_offline_has_tstamp_precision
    config.set('HAVE_PCAP_OPEN_OFFLINE_WITH_TSTAMP_PRECISION', 1)
    config.set('PCAP_TSTAMP_US_TO_NS_MULTIPLIER', 1)
    config.set('PCAP_TSTAMP_US_TO_US_DIVISOR', 1000)
else
    config.set('PCAP_TSTAMP_US_TO_NS_MULTIPLIER', 100)
    config.set('PCAP_TSTAMP_US_TO_US_DIVISOR', 1)
endif

# System libraries.
# On glibc these symbols live in libc, so the libraries are usually absent and
# unneeded; where a platform does ship them (Solaris, older glibc) they are
# linked into every program via system_libs.  Function names match configure.ac
# exactly so the HAVE_LIB* results stay identical (e.g. there is no resolv()
# symbol, so HAVE_LIBRESOLV is not defined, just like autotools).
system_libs = []
foreach probe : [['socket', 'socket'], ['nsl', 'gethostbyname'], ['rt', 'nanosleep'], ['resolv', 'resolv']]
    lib = cc.find_library(probe[0], required : false)
    if lib.found() and cc.has_function(probe[1], dependencies : lib)
        system_libs += lib
        config.set('HAVE_LIB' + probe[0].to_upper(), 1)
    endif
endforeach

# Workaround an under-specified system libsystemd.pc.  When libpcap is linked
# statically it transitively pulls in libsystemd.a (via dbus-1's Libs.private),
# whose capability code references libcap's cap_* symbols. Some distributions
# ship a libsystemd.pc with no `Libs.private: -lcap`, so pkg-config never adds
# libcap to the static link and the final link fails with undefined references
# (cap_get_proc, cap_set_flag, ...).  Pull libcap in explicitly so the static
# chain resolves. It is harmless when libsystemd is not part of the link because
# unreferenced archive members are not extracted.
if static_link and host == 'linux'
    libcap = cc.find_library('cap', required : false)
    if libcap.found()
        system_libs += libcap
    endif
endif

# libdnet (a.k.a. libdumbnet on Debian/Ubuntu, which renames dnet.h -> dumbnet.h
# to avoid a name clash).  Optional: enables libdnet packet injection and the
# fragroute module.
libdnet_opt = get_option('libdnet')
have_libdnet = false
libdnet_dep = []
libdnet_version = ''
if not libdnet_opt.disabled()
    dnet_config = find_program('dumbnet-config', 'dnet-config', required : false)
    if dnet_config.found()
        libdnet_version = run_command(dnet_config, '--version', check : false).stdout().strip()
    endif
    if cc.has_header('dumbnet.h')
        lib = cc.find_library('dumbnet', required : false)
        if lib.found()
            have_libdnet = true
            libdnet_dep = [lib]
            config.set('HAVE_DUMBNET_H', 1)
        endif
    endif
    if not have_libdnet and cc.has_header('dnet.h')
        lib = cc.find_library('dnet', required : false)
        if lib.found()
            have_libdnet = true
            libdnet_dep = [lib]
            config.set('HAVE_DNET_H', 1)
        endif
    endif
endif
if libdnet_opt.enabled() and not have_libdnet
    error('libdnet/libdumbnet requested (-Dlibdnet=enabled) but not found')
endif
if have_libdnet
    config.set('HAVE_LIBDNET', 1)
endif
config.set_quoted('LIBDNET_VERSION', libdnet_version)

# fragroute requires libdnet
fragroute_opt = get_option('fragroute')
enable_fragroute = have_libdnet and not fragroute_opt.disabled()
if fragroute_opt.enabled() and not have_libdnet
    error('fragroute requested (-Dfragroute=enabled) but libdnet/libdumbnet was not found')
endif
if enable_fragroute
    config.set('ENABLE_FRAGROUTE', 1)
endif

# Determine which optional programs can be built
# tcpbridge requires pcap_sendpacket(); on BPF systems it also needs
# pcap_setnonblock().  -Dtcpbridge can force-disable (or require) it.
have_pcap_sendpacket = cc.has_header_symbol(lpcapinc / 'pcap.h', 'pcap_sendpacket')
# sendpacket.c #ifdefs on HAVE_PCAP_SENDPACKET for its preferred injection path,
# so define to 1 only when present (leave unset otherwise).
if have_pcap_sendpacket
    config.set('HAVE_PCAP_SENDPACKET', 1)
endif
have_pcap_setnonblock = cc.has_header_symbol(lpcapinc / 'pcap.h', 'pcap_setnonblock')
have_bpf = cc.has_header('net/bpf.h')
if have_bpf
    config.set('HAVE_BPF', 1)
endif
tcpbridge_opt = get_option('tcpbridge')
tcpbridge_capable = have_pcap_sendpacket and (not have_bpf or have_pcap_setnonblock)
if tcpbridge_opt.enabled() and not tcpbridge_capable
    error('tcpbridge requested (-Dtcpbridge=enabled) but pcap_sendpacket()/pcap_setnonblock() are unavailable')
endif
enable_tcpbridge = tcpbridge_capable and not tcpbridge_opt.disabled()
if enable_tcpbridge
    config.set('ENABLE_TCPBRIDGE', 1)
endif

# tcpliveplay only works on Linux; -Dtcpliveplay can force-disable it.
tcpliveplay_opt = get_option('tcpliveplay')
if tcpliveplay_opt.enabled() and host != 'linux'
    error('tcpliveplay requested (-Dtcpliveplay=enabled) but is only supported on Linux')
endif
enable_tcpliveplay = host == 'linux' and not tcpliveplay_opt.disabled()
if enable_tcpliveplay
    config.set('ENABLE_TCPLIVEPLAY', 1)
endif

# Linux PF_PACKET injection support
have_pf = cc.compiles('''
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <netinet/in.h>
int main(void) { return socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL)); }
''', name : 'PF_PACKET socket sending support')
if have_pf
    config.set('HAVE_PF_PACKET', 1)
endif

# libbpf / AF_XDP (libxdp) injection backends.
# autotools probes libbpf with AC_CHECK_LIB(bpf, bpf_object__open_file) and
# libxdp with AC_CHECK_LIB(xdp, xsk_umem__delete), prepending -lbpf/-lxdp to
# the global LIBS so every program links them; here they are appended to
# system_libs for the same effect.  HAVE_LIBBPF makes defines.h pull in
# <bpf/bpf.h>/<bpf/libbpf.h>; HAVE_LIBXDP enables the AF_XDP sending path in
# sendpacket.c (which uses the xsk_* API from <xdp/xsk.h>).
have_libbpf = false
libbpf = cc.find_library('bpf', has_headers : ['bpf/bpf.h', 'bpf/libbpf.h'], required : false)
if libbpf.found() and cc.has_function('bpf_object__open_file', dependencies : libbpf)
    have_libbpf = true
    config.set('HAVE_LIBBPF', 1)
    system_libs += libbpf
endif

# AF_XDP support is a real compile/link test, not just a header probe.
# It needs libxdp (and libbpf, which provides the xsk ring helpers on older toolchains).
have_libxdp = false
libxdp = cc.find_library('xdp', has_headers : ['xdp/libxdp.h', 'xdp/xsk.h'], required : false)
if libxdp.found()
    libxdp_deps = [libxdp]
    if have_libbpf
        libxdp_deps += libbpf
    endif
    have_libxdp = cc.links('''
#include <stdlib.h>
#include <xdp/xsk.h>
#include <sys/socket.h>
int main(void) {
    struct xsk_socket *xsk = NULL;
    struct xsk_ring_cons *rxr = NULL;
    struct xsk_ring_prod *txr = NULL;
    int queue_id = 0;
    xsk_socket__create(&xsk, "lo", queue_id, NULL, rxr, txr, NULL);
    return socket(AF_XDP, SOCK_RAW, 0);
}
''', dependencies : libxdp_deps, name : 'LIBXDP AF_XDP packet sending support')
endif
if have_libxdp
    config.set('HAVE_LIBXDP', 1)
    system_libs += libxdp
    # libbpf, when present, is already in system_libs (added above); HAVE_LIBBPF
    # being set independently is what defines.h keys off, so nothing else to do.
endif

# Older Linux TX_RING injection. Gates the PACKET_TX_RING backend in txring.c.
have_tx_ring = cc.compiles('''
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <linux/if_packet.h>
int main(void) { return TP_STATUS_WRONG_FORMAT; }
''', name : 'TX_RING socket sending support')
if have_tx_ring
    config.set('HAVE_TX_RING', 1)
endif

# Make sure we have at least one supported packet-injection mechanism
if not (have_bpf or have_pcap_inject or have_pcap_sendpacket or have_libdnet or have_pf)
    error('Unable to find a supported method to send packets.  Please upgrade your libpcap or enable libdnet.')
endif

# Validate the forced packet-injection backend.  -Dforce-send pins sendpacket.c
# to a single backend (FORCE_INJECT_*) and undefines every other HAVE_* method.
# sendpacket_open() then resolves its backend through this ordered #if/#elif
# chain:
#     HAVE_PF_PACKET -> HAVE_BPF -> HAVE_LIBDNET ->
#     (HAVE_PCAP_INJECT || HAVE_PCAP_SENDPACKET) -> HAVE_LIBXDP
# The combo option makes the backends mutually exclusive by construction (at
# most one can be selected), so only the availability check remains: if the
# forced backend was not detected on this host the build would otherwise fail
# late and cryptically with sendpacket.c's
# '#error "No defined packet injection method for sendpacket_open()"', so catch
# it here at configure time with an actionable message instead.
force_send = get_option('force-send')
forced_inject_backends = {
    'bpf':        ['FORCE_INJECT_BPF',             have_bpf,             'BPF (net/bpf.h)'],
    'pf':         ['FORCE_INJECT_PF_PACKET',       have_pf,              'PF_PACKET'],
    'libdnet':    ['FORCE_INJECT_LIBDNET',         have_libdnet,         'libdnet/libdumbnet'],
    'libxdp':     ['FORCE_INJECT_LIBXDP',          have_libxdp,          'libxdp (AF_XDP)'],
    'inject':     ['FORCE_INJECT_PCAP_INJECT',     have_pcap_inject,     'pcap_inject()'],
    'sendpacket': ['FORCE_INJECT_PCAP_SENDPACKET', have_pcap_sendpacket, 'pcap_sendpacket()'],
}
if force_send != 'auto'
    backend = forced_inject_backends[force_send]
    if not backend[1]
        error('-Dforce-send=@0@ was requested but @1@ is not available on this host'.format(force_send, backend[2]))
    endif
    config.set(backend[0], 1)
endif
