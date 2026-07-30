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


# AutoOpts code generation (*_opts.def -> *_opts.c/h + *.adoc).
#
# scripts/autoopts/generate.py (python3) is the drop-in replacement for the
# now-EOL GNU autogen (#895); it emits the same option parsers autogen used to
# produce, byte for byte, plus the AsciiDoc man-page sources.
run_command(python3, generate_py, '--top', meson.project_source_root(),
    check : true)


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
# Every tool links the bundled AutoOpts runtime (libopts) that the generated
# *_opts.c parsers call into.  libopts_inc puts <autoopts/options.h> on the
# include path for those generated sources (and for tcpedit_stub.h).
prog_link = [libcommon, libstrl, libopts]
prog_deps = [libpcap] + system_libs + libdnet_dep + extra_deps

prog_inc = [configuration_inc, libopts_inc]
prog_inc_edit = [configuration_inc, tcpedit_inc, libopts_inc]

osx_link_args = []
if host_machine.system() == 'darwin'
    osx_link_args = ['-framework', 'CoreServices', '-framework', 'Carbon']
endif

# The tcpreplay replay engine: everything except main() and the generated
# option parser.  Split out from the command-line front-end so tcpreplay and
# tcpreplay-edit can share the source list (mirrors the SOURCES in
# src/Makefile.am).  The generated *_opts.c is added per-binary below because
# tcpreplay and tcpreplay-edit use different parsers: tcpreplay_opts.c vs the
# TCPREPLAY_EDIT-conditioned tcpreplay_edit_opts.c.
tcpreplay_engine_src = files(
    'replay.c',
    'send_packets.c',
    'signal_handler.c',
    'sleep.c',
    'tcpreplay_api.c',
)

# The command-line front-end is just main().
tcpreplay_cli_src = files(
    'tcpreplay.c',
)

# The engine is compiled directly into the tcpreplay binary.
tcpreplay_exe = executable('tcpreplay',
    tcpreplay_engine_src, tcpreplay_cli_src, files('tcpreplay_opts.c'), defines_src,
    c_args : ['-DTCPREPLAY'],
    include_directories : prog_inc,
    link_with : prog_link,
    dependencies : prog_deps,
    link_args : osx_link_args,
    install : true,
)

# tcpreplay-edit needs the engine recompiled with -DTCPREPLAY_EDIT (libtcpedit
# support) and the matching TCPREPLAY_EDIT variant of the generated parser.
executable('tcpreplay-edit',
    tcpreplay_engine_src, tcpreplay_cli_src, files('tcpreplay_edit_opts.c'), defines_src,
    c_args : ['-DTCPREPLAY', '-DTCPREPLAY_EDIT', '-DHAVE_CACHEFILE_SUPPORT'],
    include_directories : prog_inc_edit,
    link_with : [prog_link, libtcpedit],
    dependencies : prog_deps,
    link_args : osx_link_args,
    install : true,
)

tcpprep_exe = executable('tcpprep',
    files('tcpprep_opts.c', 'tcpprep.c', 'tree.c', 'tcpprep_api.c'), defines_src,
    c_args : ['-DTCPPREP'],
    include_directories : prog_inc,
    link_with : prog_link,
    dependencies : prog_deps,
    install : true,
)

tcprewrite_exe = executable('tcprewrite',
    files('tcprewrite_opts.c', 'tcprewrite.c'), defines_src,
    c_args : ['-DTCPREWRITE', '-DHAVE_CACHEFILE_SUPPORT'],
    include_directories : prog_inc_edit,
    link_with : [prog_link, libtcpedit, libfragroute],
    dependencies : prog_deps,
    install : true,
)

executable('tcpcapinfo',
    files('tcpcapinfo_opts.c', 'tcpcapinfo.c'), defines_src,
    c_args : ['-DTCPCAPINFO'],
    include_directories : prog_inc,
    link_with : prog_link,
    dependencies : prog_deps,
    install : true,
)

if enable_tcpbridge
    executable('tcpbridge',
        files('tcpbridge_opts.c', 'tcpbridge.c', 'bridge.c'), defines_src,
        c_args : ['-DTCPBRIDGE'],
        include_directories : prog_inc_edit,
        link_with : [prog_link, libtcpedit],
        dependencies : prog_deps,
        link_args : osx_link_args,
        install : true,
    )
endif

if enable_tcpliveplay
    executable('tcpliveplay',
        files('tcpliveplay_opts.c', 'tcpliveplay.c'), defines_src,
        c_args : ['-DTCPREPLAY', '-DTCPLIVEPLAY'],
        include_directories : prog_inc,
        link_with : prog_link,
        dependencies : prog_deps,
        install : true,
    )
endif

# Man-page sources (src/*.adoc) generated by scripts/autoopts alongside the
# *_opts.c/h above.  docs/meson.build renders these to *.1 with asciidoctor;
# the mapping lives here because the .adoc files are in this directory and the
# set of tools mirrors the binaries built above.  Each generated .adoc is
# self-contained (the tcpedit options are inlined), so no include list is
# needed - unlike the old hand-written docs/*.adoc.
man_page_defs = [
    ['tcpreplay',      files('tcpreplay.adoc')],
    ['tcpreplay-edit', files('tcpreplay-edit.adoc')],
    ['tcpprep',        files('tcpprep.adoc')],
    ['tcprewrite',     files('tcprewrite.adoc')],
    ['tcpcapinfo',     files('tcpcapinfo.adoc')],
]

if enable_tcpbridge
    man_page_defs += [['tcpbridge', files('tcpbridge.adoc')]]
endif

if enable_tcpliveplay
    man_page_defs += [['tcpliveplay', files('tcpliveplay.adoc')]]
endif
