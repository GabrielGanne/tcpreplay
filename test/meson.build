# Test suite ported from test/Makefile.am.
#
# Each test runs one of the built tools and diffs its output against a
# committed reference file via run_compare.py.  The reference files for
# tcprewrite/tcpreplay differ by host byte order: big-endian uses the
# "test.*" set, little-endian the "test2.*" set (the VARIANT logic in
# the autotools Makefile).  tcpprep cache files are endian-independent.
#
# The NIC-based tcpreplay tests (replay_basic, replay_cache, ...) send traffic
# on a live interface and only check the tool's exit code (no reference diff),
# mirroring the autotools targets.  They require privileges to open the
# interface (CAP_NET_RAW / root) and run on the interfaces configured via the
# 'test-nic1'/'test-nic2' options (default dummy0/dummy1).  They live in the
# 'tcpreplay-nic' suite so they can be selected or skipped independently, e.g.
#   meson test --suite tcpreplay-nic        # run only the NIC tests
#   meson test --no-suite tcpreplay-nic     # skip them

python = find_program('python3', 'python')
diff = find_program('diff')
runner = [files('run_compare.py')[0], '--diff', diff.full_path()]

test_pcap = files('test.pcap')[0]
auto_router = files('test.auto_router')[0]
auto_bridge = files('test.auto_bridge')[0]
cidr_cache = files('test.cidr')[0]
rewrite_config_in = files('test.rewrite_config')[0]

nic1 = get_option('test-nic1')
nic2 = get_option('test-nic2')

variant = host_machine.endian() == 'big' ? '' : '2'

# --- tcpprep: [name, args] (input is always test.pcap, reference test.<name>) ---
tcpprep_cases = [
    ['auto_router',     ['-a', 'router']],
    ['auto_bridge',     ['-a', 'bridge']],
    ['auto_client',     ['-a', 'client']],
    ['auto_server',     ['-a', 'server']],
    ['auto_first',      ['-a', 'first']],
    ['port',            ['-p']],
    ['mac',             ['-e', '00:1f:f3:3c:e1:13']],
    ['cidr',            ['-c', '96.17.211.0/24']],
    ['regex',           ['-r', '96.17.211.*']],
    ['comment',         ['-C', 'This is a comment', '-p']],
    ['mac_reverse',     ['-e', '00:1f:f3:3c:e1:13', '--reverse']],
    ['cidr_reverse',    ['-c', '96.17.211.0/24', '--reverse']],
    ['regex_reverse',   ['-r', '96.17.211.*', '--reverse']],
    ['exclude_packets', ['-c', '96.17.211.0/24', '--exclude', 'P:61-65,88-91']],
    ['include_packets', ['-c', '96.17.211.0/24', '--include', 'P:61-65,88-91']],
    ['include_source',  ['-c', '96.17.211.0/24', '--include', 'S:96.0.0.0/8']],
    ['include_dest',    ['-c', '96.17.211.0/24', '--include', 'D:96.0.0.0/8']],
    ['prep_config',     ['-a', 'bridge', '-m', '31', '-M', '16', '-N',
                         '-C', 'This file was generated via the config file']],
]

foreach case : tcpprep_cases
    name = case[0]
    out = 'test.' + name + '1'
    test(name, python,
        args : [runner, files('test.' + name)[0], out,
                tcpprep_exe, '--no-arg-comment', '-i', test_pcap, '-o', out]
               + case[1],
        suite : 'tcpprep')
endforeach

# --- tcpprep: cache-inspection modes that print to stdout (-P/-I) ---
# These read the committed test.comment cache and dump its embedded comment
# (-P) or per-packet classification (-I).  Output is endian-independent, so a
# single committed reference is used and captured via run_compare's --stdout.
comment_cache = files('test.comment')[0]
tcpprep_print_cases = [
    ['print_comment', ['-P', comment_cache]],
    ['print_info',    ['-I', comment_cache]],
]

foreach case : tcpprep_print_cases
    name = case[0]
    out = 'test.' + name + '1'
    test(name, python,
        args : [runner, '--stdout', files('test.' + name)[0], out,
                tcpprep_exe] + case[1],
        suite : 'tcpprep')
endforeach

# --- tcprewrite: [name, input, args] (reference test<variant>.<name>) ---
tcprewrite_cases = [
    ['rewrite_portmap',       test_pcap, ['-r', '80:8080']],
    ['rewrite_range_portmap', test_pcap, ['-r', '1-100:49148']],
    ['rewrite_sequence',      test_pcap, ['--tcp-sequence', '42']],
    ['rewrite_endpoint',      test_pcap, ['-e', '10.10.0.1:10.10.0.2',
                                          '-c', auto_router]],
    ['rewrite_pnat',          test_pcap, ['-N', '96.17.211.0/24:172.16.0.0/24']],
    ['rewrite_pad',           test_pcap, ['-F', 'pad']],
    ['rewrite_trunc',         test_pcap, ['-F', 'trunc']],
    ['rewrite_mac',           test_pcap, ['--enet-dmac=00:12:13:14:15:16,00:22:33:44:55:66',
                                          '--enet-smac=00:22:33:44:55:66,00:12:13:14:15:16',
                                          '-c', auto_router]],
    ['rewrite_enet_subsmac',  test_pcap, ['--enet-subsmac=00:1f:f3:3c:e1:13,00:22:33:44:55:66',
                                          '--enet-subsmac=f8:1e:df:e5:84:3a,00:66:55:44:33:22']],
    ['rewrite_mac_seed',      test_pcap, ['--enet-mac-seed=42']],
    ['rewrite_mac_seed_keep', test_pcap, ['--enet-mac-seed=42',
                                          '--enet-mac-seed-keep-bytes=3']],
    ['rewrite_layer2',        test_pcap, ['--dlt=user',
                                          '--user-dlink=00,50,da,5d,46,55,0,7,eb,30,a4,c3,08,0']],
    ['rewrite_config',        test_pcap, ['--enet-vlan=add', '--enet-vlan-tag=45',
                                          '--enet-vlan-cfi=1', '--enet-vlan-pri=5']],
    ['rewrite_skip',          test_pcap, ['--skipbroadcast', '--skipl2broadcast',
                                          '--skip-soft-errors', '--seed', '55',
                                          '--enet-dmac=00:12:13:14:15:16,00:22:33:44:55:66',
                                          '--enet-smac=00:22:33:44:55:66,00:12:13:14:15:16',
                                          '-c', auto_router]],
    ['rewrite_dltuser',       test_pcap, ['--dlt=user',
                                          '--user-dlink=0x0f,0x00,0x08,0x00',
                                          '--user-dlt=104']],
    ['rewrite_dlthdlc',       test_pcap, ['--dlt=hdlc', '--hdlc-control=0',
                                          '--hdlc-address=0x0F']],
    ['rewrite_vlan802.1ad',   test_pcap, ['--enet-vlan=add', '--enet-vlan-tag=42',
                                          '--enet-vlan-cfi=1', '--enet-vlan-pri=2',
                                          '--enet-vlan-proto=802.1ad']],
    ['rewrite_vlandel',       rewrite_config_in, ['--enet-vlan=del']],
    ['rewrite_efcs',          test_pcap, ['--efcs']],
    ['rewrite_1ttl',          test_pcap, ['--ttl=58']],
    ['rewrite_2ttl',          test_pcap, ['--ttl=+58']],
    ['rewrite_3ttl',          test_pcap, ['--ttl=-58']],
    ['rewrite_1ttl-hdrfix',   test_pcap, ['--ttl=59', '--fixhdrlen']],
    ['rewrite_2ttl-hdrfix',   test_pcap, ['--ttl=+59', '--fixhdrlen']],
    ['rewrite_3ttl-hdrfix',   test_pcap, ['--ttl=-59', '--fixhdrlen']],
    ['rewrite_tos',           test_pcap, ['--tos=50']],
    ['rewrite_mtutrunc',      test_pcap, ['--mtu-trunc', '--mtu=300']],
    ['rewrite_l7fuzzing',     test_pcap, ['--fuzz-seed=42', '--fuzz-factor=2']],
    ['rewrite_fixcsum',       test_pcap, ['--fixcsum']],
    ['rewrite_fixlen_pad',    test_pcap, ['--fixlen=pad']],
    ['rewrite_fixlen_trunc',  test_pcap, ['--fixlen=trunc']],
    ['rewrite_fixlen_del',    test_pcap, ['--fixlen=del']],
    ['rewrite_seed',          test_pcap, ['-s', '55']],
]

foreach case : tcprewrite_cases
    name = case[0]
    out = 'test.' + name + '1'
    test(name, python,
        args : [runner, files('test' + variant + '.' + name)[0], out,
                tcprewrite_exe, '-i', case[1], '-o', out] + case[2],
        suite : 'tcprewrite')
endforeach

# --- tcpreplay: file-writing tests (no NIC required) ---
tcpreplay_cases = [
    ['replay_include',   ['--include=7,11,20-23,174-']],
    ['replay_exclude',   ['--exclude=23-,11-20,2,3']],
    ['replay_unique_ip', ['--unique-ip', '--loop=2']],
]

foreach case : tcpreplay_cases
    name = case[0]
    out = 'test.' + name + '1'
    test(name, python,
        args : [runner, files('test' + variant + '.' + name)[0], out,
                tcpreplay_exe, '-w', out, '-t'] + case[1] + [test_pcap],
        suite : 'tcpreplay')
endforeach

# --- tcpreplay: NIC-based tests (send on an interface, check exit code only) ---
# Mirrors the autotools targets; requires CAP_NET_RAW/root and the interfaces
# configured via the 'test-nic1'/'test-nic2' options (default dummy0/dummy1).
tcpreplay_nic_cases = [
    ['replay_basic',      ['-i', nic1, '-t', test_pcap]],
    ['replay_nano_timer', ['-i', nic1, '--loop=2', '--loopdelay-ns=125000000',
                           '-t', test_pcap]],
    ['replay_cache',      ['-c', cidr_cache, '-i', nic1, '-I', nic2,
                           '-t', test_pcap]],
    ['replay_pps',        ['-i', nic1, '-p', '25', test_pcap]],
    ['replay_rate',       ['-i', nic1, '-M', '25.0', test_pcap]],
    ['replay_top',        ['-i', nic1, '-t', test_pcap]],
    ['replay_config',     ['-c', auto_bridge, '-i', nic1, '-I', nic2,
                           '-t', test_pcap]],
    ['replay_multi',      ['-i', nic1, '-x', '25.0', test_pcap]],
    ['replay_pps_multi',  ['-i', nic1, '-p', '100', '--pps-multi=5', test_pcap]],
    ['replay_precache',   ['-i', nic1, '--preload-pcap', test_pcap]],
    ['replay_stats',      ['--stats=1', '-i', nic1, test_pcap]],
    ['replay_dualfile',   ['-i', nic1, '-I', nic2, '--dualfile',
                           test_pcap, test_pcap]],
    ['replay_maxsleep',   ['-i', nic1, '--maxsleep=20', test_pcap, test_pcap]],
]

foreach case : tcpreplay_nic_cases
    test(case[0], tcpreplay_exe,
        args : case[1],
        suite : 'tcpreplay-nic')
endforeach
