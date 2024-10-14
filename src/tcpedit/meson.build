# Option parsing is now hand-written using getopt_long in tcpedit_args.c,
# backed by the AutoOpts-compatible accessor macros in tcpedit_stub.h
# (replacing the AutoOpts/cli-codegen generated sources).  The tcpedit options
# are documented in docs/tcpedit_options.adoc, which is included by the
# tcprewrite/tcpbridge/tcpreplay-edit AsciiDoc man pages (see docs/meson.build),
# so there is no standalone tcpedit.1 man page to generate here.
libtcpedit_src = files(
    'checksum.c',
    'dlt.c',
    'edit_packet.c',
    'fuzzing.c',
    'incremental_checksum.c',
    'parse_args.c',
    'portmap.c',
    'rewrite_sequence.c',
    'tcpedit_api.c',
    'tcpedit_args.c',
    'tcpedit.c',
)

# DLT plugins (mirrors src/tcpedit/plugins/Makefile.am and the per-plugin
# Makefile.am fragments).
libtcpedit_src += files(
    'plugins/dlt_plugins.c',
    'plugins/ethernet.c',
    'plugins/dlt_utils.c',
    'plugins/dlt_en10mb/en10mb.c',
    'plugins/dlt_en10mb/en10mb_api.c',
    'plugins/dlt_hdlc/hdlc.c',
    'plugins/dlt_hdlc/hdlc_api.c',
    'plugins/dlt_user/user.c',
    'plugins/dlt_user/user_api.c',
    'plugins/dlt_raw/raw.c',
    'plugins/dlt_null/null.c',
    'plugins/dlt_loop/loop.c',
    'plugins/dlt_linuxsll/linuxsll.c',
    'plugins/dlt_linuxsll2/linuxsll2.c',
    'plugins/dlt_ieee80211/ieee80211.c',
    'plugins/dlt_ieee80211/ieee80211_hdr.c',
    'plugins/dlt_radiotap/radiotap.c',
    'plugins/dlt_jnpr_ether/jnpr_ether.c',
    'plugins/dlt_jnpr_ether/jnpr_ether_api.c',
    'plugins/dlt_pppserial/pppserial.c',
    'plugins/dlt_pppserial/pppserial_api.c',
)

# Mirror tcpedit's AM_CFLAGS include search path: src/tcpedit (tcpedit.h),
# src/tcpedit/plugins (dlt_utils.h, ethernet.h) and src/common.  src and the
# repository root are provided by configuration_inc.
tcpedit_inc = include_directories('.', 'plugins', '../common')

# Produces libtcpedit.a, linked by tcprewrite, tcpbridge and tcpreplay-edit.
# libdnet_dep (empty unless libdnet was found) mirrors @LDNETINC@ in AM_CFLAGS.
libtcpedit = static_library('tcpedit',
    libtcpedit_src,
    include_directories : [configuration_inc, tcpedit_inc],
    dependencies : libdnet_dep,
    install : false,
)
