subdir('common')
subdir('tcpedit')

# defines.h.in only needs a couple of @TOKEN@ substitutions (the rest of its
# configuration comes from the #include "config.h" at its top), so it gets its
# own configuration object rather than the global one.
#
# It also carries a splint annotation "/*@unused@*/"; configure_file() would
# otherwise treat @unused@ as an undefined substitution, strip it, and warn.
# Mapping it to the literal token preserves the annotation verbatim.
defines_config = configuration_data()
defines_config.set('LPCAPINC', config.get('LPCAPINC'))
defines_config.set('INCLUDE_PCAP_BPF_HEADER', config.get('INCLUDE_PCAP_BPF_HEADER'))
defines_config.set('unused', '@unused@')
defines_src = configure_file(
        output : 'defines.h',
        input : files('defines.h.in'),
        configuration : defines_config,
)


# fragroute is only built when libdnet/libdumbnet is available; tcprewrite links
# it (mirrors $(LIBFRAGROUTE) in src/Makefile.am).
libfragroute = []
if enable_fragroute
    subdir('fragroute')
endif

# Programs (mirrors bin_PROGRAMS in src/Makefile.am).  All binaries depend on
# the generated defines.h and link against libcommon (+ the bundled libstrl
# fallback) and libpcap; the tcpedit-based tools additionally link libtcpedit.
# system_libs (socket/nsl/rt/resolv) and libdnet_dep mirror the @LDNETLIB@ and
# AC_CHECK_LIB additions that autotools appends to every program's LDADD.
prog_link = [libcommon, libstrl]
prog_deps = [libpcap] + system_libs + libdnet_dep + extra_deps

osx_link_args = []
if host_machine.system() == 'darwin'
    osx_link_args = ['-framework', 'CoreServices', '-framework', 'Carbon']
endif

# The tcpreplay replay engine: everything except main().  Split out from the
# command-line front-end so tcpreplay and tcpreplay-edit can share the source
# list (mirrors the SOURCES in src/Makefile.am).
tcpreplay_engine_src = files(
    'replay.c',
    'send_packets.c',
    'signal_handler.c',
    'sleep.c',
    'tcpreplay_api.c',
    'tcpreplay_args.c',
)

# The command-line front-end is just main().
tcpreplay_cli_src = files(
    'tcpreplay.c',
)

# The engine is compiled directly into the tcpreplay binary.
tcpreplay_exe = executable('tcpreplay',
    tcpreplay_engine_src, tcpreplay_cli_src, defines_src,
    c_args : ['-DTCPREPLAY'],
    include_directories : configuration_inc,
    link_with : prog_link,
    dependencies : prog_deps,
    link_args : osx_link_args,
    install : true,
)

# tcpreplay-edit needs the engine recompiled with -DTCPREPLAY_EDIT (libtcpedit
# support); it builds all the sources directly, exactly as before.
executable('tcpreplay-edit',
    tcpreplay_engine_src, tcpreplay_cli_src, defines_src,
    c_args : ['-DTCPREPLAY', '-DTCPREPLAY_EDIT', '-DHAVE_CACHEFILE_SUPPORT'],
    include_directories : [configuration_inc, tcpedit_inc],
    link_with : [prog_link, libtcpedit],
    dependencies : prog_deps,
    link_args : osx_link_args,
    install : true,
)

tcpprep_exe = executable('tcpprep',
    files('tcpprep_args.c', 'tcpprep.c', 'tree.c', 'tcpprep_api.c'), defines_src,
    c_args : ['-DTCPPREP'],
    include_directories : configuration_inc,
    link_with : prog_link,
    dependencies : prog_deps,
    install : true,
)

tcprewrite_exe = executable('tcprewrite',
    files('tcprewrite_args.c', 'tcprewrite.c'), defines_src,
    c_args : ['-DTCPREWRITE', '-DHAVE_CACHEFILE_SUPPORT'],
    include_directories : [configuration_inc, tcpedit_inc],
    link_with : [prog_link, libtcpedit, libfragroute],
    dependencies : prog_deps,
    install : true,
)

executable('tcpcapinfo',
    files('tcpcapinfo_args.c', 'tcpcapinfo.c'), defines_src,
    c_args : ['-DTCPCAPINFO'],
    include_directories : configuration_inc,
    link_with : prog_link,
    dependencies : prog_deps,
    install : true,
)

if enable_tcpbridge
    executable('tcpbridge',
        files('tcpbridge_args.c', 'tcpbridge.c', 'bridge.c'), defines_src,
        c_args : ['-DTCPBRIDGE'],
        include_directories : [configuration_inc, tcpedit_inc],
        link_with : [prog_link, libtcpedit],
        dependencies : prog_deps,
        link_args : osx_link_args,
        install : true,
    )
endif

if enable_tcpliveplay
    executable('tcpliveplay',
        files('tcpliveplay_args.c', 'tcpliveplay.c'), defines_src,
        c_args : ['-DTCPREPLAY', '-DTCPLIVEPLAY'],
        include_directories : configuration_inc,
        link_with : prog_link,
        dependencies : prog_deps,
        install : true,
    )
endif
