libcommon_src = files(
	'cache.c',
	'cidr.c',
	'dlt_names.c',
	'err.c',
	'fakepcap.c',
	'fakepcapnav.c',
	'fakepoll.c',
	'flows.c',
	'get.c',
	'interface.c',
	'list.c',
	'mac.c',
	'sendpacket.c',
	'services.c',
	'timer.c',
	'txring.c',
	'utils.c',
	'xX.c',
)

private_headers = files(
	'cache.h',
	'cidr.h',
	'dlt_names.h',
	'err.h',
	'fakepcap.h',
	'fakepcapnav.h',
	'fakepoll.h',
	'flows.h',
	'get.h',
	'interface.h',
	'list.h',
	'mac.h',
	'pcap_dlt.h',
	'sendpacket.h',
	'services.h',
	'timer.h',
	'txring.h',
	'utils.h',
	'xX.h',
)

# git_version.c is generated at build time from the committed git_version.c.in
# template (@VCS_TAG@ -> output of `git describe --always`, matching the autotools
# recipe).  The generated git_version.c is gitignored and must never be committed,
# so it is intentionally absent from libcommon_src above.
git_version_src = vcs_tag(command : ['git', 'describe', '--always'],
    input : 'git_version.c.in',
    output : 'git_version.c',
)

if enable_tcpdump
	libcommon_src += files('tcpdump.c')
	private_headers += files('tcpdump.h')
endif

if enable_netmap
	libcommon_src += files('netmap.c')
	private_headers += files('netmap.h')
endif

# The generated git_version.c is built into its own object with coverage
# instrumentation disabled (b_coverage=false).
# It lives in the build tree at build/src/common/git_version.c it otherwise
# mirrors the real src/common source directory; under `meson setup
# -Db_coverage=true` that collision makes genhtml abort with "duplicate merge
# record src/common".  Keeping it out of the coverage data avoids that without
# affecting normal builds.  link_whole pulls the object into libcommon so every
# consumer is unchanged.
libgitversion = static_library('gitversion',
    git_version_src,
    install : false,
    include_directories : configuration_inc,
    override_options : ['b_coverage=false'],
)

# sendpacket.c optionally uses libdnet (HAVE_LIBDNET); libdnet_dep is empty when
# libdnet/libdumbnet was not found.
libcommon = static_library('common',
    libcommon_src + private_headers,
    install : false,
    include_directories : configuration_inc,
    dependencies : libdnet_dep,
    link_whole : libgitversion,
)
