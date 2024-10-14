project('tcpreplay', 'c',
    version : '4.5.2',
    default_options : ['warning_level=2'], # TODO increase to warning_level=3
    license : 'GPLv3',
    # 0.58.0 (May 2021) is shipped by every currently supported distribution;
    # it is the floor required for passing feature options to summary().
    meson_version: '>= 0.58.0',
)

cc = meson.get_compiler('c')
fs = import('fs')
configuration_inc = include_directories('src', '.')


config = configuration_data()
# package version, used by the per-tool --version output (config.h in autotools)
config.set_quoted('VERSION', meson.project_version())

# Host / library / feature detection (headers, type sizes, libpcap, libdnet,
# netmap, the packet-injection backends, ...).
subdir('meson')

# Build / debugging options (mirror the configure.ac AC_ARG_ENABLE switches).
extra_deps = []
build_c_args = []
build_link_args = []

if get_option('debug-code')
    config.set('DEBUG', 1)
    build_c_args += ['-g', '-O0', '-Wall']
endif
if get_option('extra-debug')
    config.set('EXTRA_DEBUG', 1)
endif
if get_option('timestamp-trace')
    config.set('TIMESTAMP_TRACE', 1)
endif
if get_option('dmalloc')
    config.set('ENABLE_DMALLOC', 1)
    config.set('DMALLOC_FUNC_CHECK', 1)
    extra_deps += cc.find_library('dmalloc', required : true)
endif
if get_option('efence')
    config.set('EFENCE', 1)
    extra_deps += cc.find_library('efence', required : true)
endif
if get_option('gprof')
    config.set('GPROF', 1)
    # mirror configure.ac: -pg alone under --enable-debug, otherwise add coverage.
    gprof_args = get_option('debug-code') ? ['-pg'] : ['-ggdb', '-pg', '-fprofile-arcs']
    build_c_args += gprof_args
    build_link_args += gprof_args
endif
if get_option('pedantic')
    build_c_args += '-pedantic'
endif

# macOS SDK selection (Darwin only; mirrors --with-macos-sdk).
macos_sdk = get_option('macos-sdk')
if host == 'darwin'
    xcrun = find_program('xcrun', required : false)
    if xcrun.found()
        if macos_sdk != ''
            sdk_res = run_command(xcrun, '--sdk', 'macosx' + macos_sdk, '--show-sdk-path', check : false)
        else
            sdk_res = run_command(xcrun, '--show-sdk-path', check : false)
        endif
        sdk_path = sdk_res.stdout().strip()
        if sdk_path != ''
            build_c_args += ['-isysroot', sdk_path]
            build_link_args += ['-isysroot', sdk_path]
        elif macos_sdk != ''
            error('could not find macOS SDK ' + macos_sdk)
        endif
    endif
endif

if build_c_args.length() > 0
    add_project_arguments(build_c_args, language : 'c')
endif
if build_link_args.length() > 0
    add_project_link_arguments(build_link_args, language : 'c')
endif

# define HAVE_CONFIG_H with compiler command line to include the generated
# config.h file (same as autotools)
add_project_arguments('-DHAVE_CONFIG_H', language : 'c')

config_h = configure_file(output : 'config.h', configuration : config)

if cc.get_id() == 'gcc'
    add_project_arguments('-D_U_=__attribute__((unused))', language: 'c')
else
    add_project_arguments('-D_U_=', language: 'c')
endif

subdir('lib')
subdir('src')

if not get_option('docs').disabled()
  subdir('docs')
endif

if not get_option('tests').disabled()
  subdir('test')
endif


summary({
        'tests'                 : get_option('tests'),
        'man pages (asciidoctor)': get_option('docs'),
        'static link'           : static_link,
        'debug code'            : get_option('debug-code'),
        'extra debug'           : get_option('extra-debug'),
        'gprof'                 : get_option('gprof'),
        'dmalloc'               : get_option('dmalloc'),
        'efence'                : get_option('efence'),
    },
    section : 'Build options',
)

summary({
        'tcpbridge'             : enable_tcpbridge,
        'tcpliveplay'           : enable_tcpliveplay,
        'tcpdump (verbose)'     : enable_tcpdump,
        'libdnet'               : have_libdnet,
        'fragroute'             : enable_fragroute,
        'netmap'                : enable_netmap,
        'libbpf'                : have_libbpf,
        'libxdp (AF_XDP)'       : have_libxdp,
        'tx-ring'               : have_tx_ring,
    },
    section : 'Features',
)
