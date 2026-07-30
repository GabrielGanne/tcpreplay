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
# The scripts/autoopts emitters (python3, the drop-in replacement for the
# now-EOL GNU autogen, #895) produce the same option parsers autogen did, byte
# for byte, plus the AsciiDoc man-page sources.  Each emitter writes one file
# to stdout; a custom_target captures it into the build tree.  Unlike the
# autotools and CMake builds, meson never writes these into the source tree -
# generation is a normal build step keyed off the .def inputs.
#
# #include directives inside a .def are resolved by the parser relative to the
# including .def's own directory, so passing the top-level .def as @INPUT@ is
# sufficient for generation.  Meson cannot discover that include graph itself,
# so every recursively included fragment is also listed as a dependency to make
# edits retrigger generation.
opts_def_deps = files(
    'tcpedit/tcpedit_opts.def',
    'tcpedit/plugins/dlt_stub.def',
    'tcpedit/plugins/dlt_opts.def',
    'tcpedit/plugins/dlt_en10mb/en10mb_opts.def',
    'tcpedit/plugins/dlt_hdlc/hdlc_opts.def',
    'tcpedit/plugins/dlt_user/user_opts.def',
    'tcpedit/plugins/dlt_raw/raw_opts.def',
    'tcpedit/plugins/dlt_null/null_opts.def',
    'tcpedit/plugins/dlt_loop/loop_opts.def',
    'tcpedit/plugins/dlt_linuxsll/linuxsll_opts.def',
    'tcpedit/plugins/dlt_linuxsll2/linuxsll2_opts.def',
    'tcpedit/plugins/dlt_ieee80211/ieee80211_opts.def',
    'tcpedit/plugins/dlt_radiotap/radiotap_opts.def',
    'tcpedit/plugins/dlt_jnpr_ether/jnpr_ether_opts.def',
    'tcpedit/plugins/dlt_pppserial/pppserial_opts.def',
)
opts_generator_deps = opts_def_deps + opts_emitter_deps

# base name, .def input, -D define ('' = none), man-page name.  Mirrors the
# CASES table in scripts/autoopts/generate.py and the rules in src/Makefile.am:
# tcpreplay and tcpreplay-edit share tcpreplay_opts.def, the latter built with
# -DTCPREPLAY_EDIT.
opts_cases = [
    ['tcpcapinfo_opts',     'tcpcapinfo_opts.def',  '',               'tcpcapinfo'],
    ['tcpliveplay_opts',    'tcpliveplay_opts.def', '',               'tcpliveplay'],
    ['tcpprep_opts',        'tcpprep_opts.def',     '',               'tcpprep'],
    ['tcprewrite_opts',     'tcprewrite_opts.def',  '',               'tcprewrite'],
    ['tcpbridge_opts',      'tcpbridge_opts.def',   '',               'tcpbridge'],
    ['tcpreplay_opts',      'tcpreplay_opts.def',   '',               'tcpreplay'],
    ['tcpreplay_edit_opts', 'tcpreplay_opts.def',   'TCPREPLAY_EDIT', 'tcpreplay-edit'],
]

# Keyed by base name (opts_c/opts_h) and man name (man_adoc) so the program
# definitions below can pull out exactly the artifacts they need.  All three
# are build_by_default:false: the *_opts.c/h are pulled in as program sources
# and the *.adoc by docs/meson.build, so only the artifacts actually consumed
# (i.e. for the tools that are enabled) get built.
opts_c = {}
opts_h = {}
man_adoc = {}
foreach case : opts_cases
    opts_base = case[0]
    opts_def = files(case[1])
    opts_defines = case[2]
    man_name = case[3]

    opts_h += {opts_base : custom_target(opts_base + '.h',
        input : opts_def,
        output : opts_base + '.h',
        command : [python3, emit_h_py, opts_base, '@INPUT@', opts_defines],
        depend_files : opts_generator_deps,
        capture : true,
        build_by_default : false,
    )}
    opts_c += {opts_base : custom_target(opts_base + '.c',
        input : opts_def,
        output : opts_base + '.c',
        command : [python3, emit_c_py, opts_base, '@INPUT@', opts_defines],
        depend_files : opts_generator_deps,
        capture : true,
        build_by_default : false,
    )}
    man_adoc += {man_name : custom_target(man_name + '.adoc',
        input : opts_def,
        output : man_name + '.adoc',
        command : [python3, emit_adoc_py, opts_base, '@INPUT@', opts_defines],
        depend_files : opts_generator_deps,
        capture : true,
        build_by_default : false,
    )}
endforeach


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
#
# The generated *_opts.h are added to each program's source list so meson emits
# them before compiling that program's C sources (e.g. tcpreplay_api.c #includes
# tcpreplay_opts.h).  build/src is already on the include path because
# configuration_inc's include_directories('src') covers both the source and the
# build copy of src/.
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
    tcpreplay_engine_src, tcpreplay_cli_src,
    opts_c['tcpreplay_opts'], opts_h['tcpreplay_opts'], defines_src,
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
    tcpreplay_engine_src, tcpreplay_cli_src,
    opts_c['tcpreplay_edit_opts'], opts_h['tcpreplay_edit_opts'], defines_src,
    c_args : ['-DTCPREPLAY', '-DTCPREPLAY_EDIT', '-DHAVE_CACHEFILE_SUPPORT'],
    include_directories : prog_inc_edit,
    link_with : [prog_link, libtcpedit],
    dependencies : prog_deps,
    link_args : osx_link_args,
    install : true,
)

tcpprep_exe = executable('tcpprep',
    opts_c['tcpprep_opts'], opts_h['tcpprep_opts'],
    files('tcpprep.c', 'tree.c', 'tcpprep_api.c'), defines_src,
    c_args : ['-DTCPPREP'],
    include_directories : prog_inc,
    link_with : prog_link,
    dependencies : prog_deps,
    install : true,
)

tcprewrite_exe = executable('tcprewrite',
    opts_c['tcprewrite_opts'], opts_h['tcprewrite_opts'],
    files('tcprewrite.c'), defines_src,
    c_args : ['-DTCPREWRITE', '-DHAVE_CACHEFILE_SUPPORT'],
    include_directories : prog_inc_edit,
    link_with : [prog_link, libtcpedit, libfragroute],
    dependencies : prog_deps,
    install : true,
)

executable('tcpcapinfo',
    opts_c['tcpcapinfo_opts'], opts_h['tcpcapinfo_opts'],
    files('tcpcapinfo.c'), defines_src,
    c_args : ['-DTCPCAPINFO'],
    include_directories : prog_inc,
    link_with : prog_link,
    dependencies : prog_deps,
    install : true,
)

if enable_tcpbridge
    executable('tcpbridge',
        opts_c['tcpbridge_opts'], opts_h['tcpbridge_opts'],
        files('tcpbridge.c', 'bridge.c'), defines_src,
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
        opts_c['tcpliveplay_opts'], opts_h['tcpliveplay_opts'],
        files('tcpliveplay.c'), defines_src,
        c_args : ['-DTCPREPLAY', '-DTCPLIVEPLAY'],
        include_directories : prog_inc,
        link_with : prog_link,
        dependencies : prog_deps,
        install : true,
    )
endif

# Man-page sources are the *.adoc custom_targets generated in the build tree
# above.  docs/meson.build renders these to *.1 with asciidoctor; the mapping
# lives here because the tool set mirrors the binaries built above.  Each
# generated .adoc is self-contained (the tcpedit options are inlined), so no
# include list is needed.
man_page_defs = [
    ['tcpreplay',      man_adoc['tcpreplay']],
    ['tcpreplay-edit', man_adoc['tcpreplay-edit']],
    ['tcpprep',        man_adoc['tcpprep']],
    ['tcprewrite',     man_adoc['tcprewrite']],
    ['tcpcapinfo',     man_adoc['tcpcapinfo']],
]

if enable_tcpbridge
    man_page_defs += [['tcpbridge', man_adoc['tcpbridge']]]
endif

if enable_tcpliveplay
    man_page_defs += [['tcpliveplay', man_adoc['tcpliveplay']]]
endif
