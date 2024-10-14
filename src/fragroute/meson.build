# fragroute module (mirrors src/fragroute/Makefile.am).  Only reached when
# libdnet/libdumbnet was found (enable_fragroute), so libdnet_dep is always
# populated here.  Produces libfragroute.a, linked into tcprewrite.
libfragroute_src = files(
    'fragroute.c',
    'bget.c',
    'mod.c',
    'pkt.c',
    'argv.c',
    'randutil.c',
    'mod_delay.c',
    'mod_drop.c',
    'mod_dup.c',
    'mod_echo.c',
    'mod_ip_chaff.c',
    'mod_ip_frag.c',
    'mod_ip_opt.c',
    'mod_ip_ttl.c',
    'mod_ip_tos.c',
    'mod_order.c',
    'mod_print.c',
    'mod_tcp_chaff.c',
    'mod_tcp_opt.c',
    'mod_tcp_seg.c',
    'iputil.c',
    'mod_ip6_opt.c',
    'mod_ip6_qos.c',
)

# AM_CFLAGS: -I$(srcdir)/.. (src) -I$(srcdir)/../.. (repo root) @LDNETINC@.
# src + root come from configuration_inc; libdnet include/link via libdnet_dep.
libfragroute = static_library('fragroute',
    libfragroute_src,
    include_directories : configuration_inc,
    dependencies : libdnet_dep,
    install : false,
)
