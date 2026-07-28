---
title: Using libtcpedit
description: Embed the packet-rewriting engine in your own program.
---

# Using libtcpedit
**libtcpedit** is the packet-rewriting engine shared by `tcprewrite`, `tcpbridge`,
and `tcpreplay-edit`. This page is for embedding it in your own program, rather
than using it through those CLI tools. Based on the public API
(`tcpedit.h`, `tcpedit_api.h`, `tcpedit_types.h`, `parse_args.h`) — note
`tcpedit_post_args()` specifically is declared in `parse_args.h`, pulled in
transitively by `#include "tcpedit/tcpedit.h"`, not in `tcpedit.h` itself.

Companion to [Using libtcpreplay](../developers/libtcpreplay.md) — that library sends
packets, this one rewrites them.

There are two ways to configure a `tcpedit_t` context:

1. Let AutoOpts parse CLI options for you and hand them to libtcpedit
   (`tcpedit_post_args()`).
2. Call the configuration API directly (`tcpedit_set_*()`), if you're not using
   AutoOpts — e.g. building a GUI, or need to run on a platform AutoOpts doesn't
   support.

## AutoOpts-driven configuration

This is the pattern every one of this project's own binaries uses. Simplified from
`tcprewrite.c`'s actual `main()`:

```c
#include "tcprewrite.h"
#include "tcprewrite_opts.h"
#include "tcpedit/tcpedit.h"

tcprewrite_opt_t options;
tcpedit_t *tcpedit;

int main(int argc, char *argv[])
{
    int optct, rcode;

    /* AutoOpts parses tcprewrite's own arguments */
    optct = optionProcess(&tcprewriteOptions, argc, argv);
    argc -= optct;
    argv += optct;
    post_args(argc, argv);

    /* init the tcpedit context with the input pcap's DLT */
    if (tcpedit_init(&tcpedit, pcap_datalink(options.pin)) < 0) {
        errx(1, "Error initializing tcpedit: %s", tcpedit_geterr(tcpedit));
    }

    /* let tcpedit parse the libtcpedit-specific args AutoOpts already collected */
    rcode = tcpedit_post_args(tcpedit);
    if (rcode < 0) {
        errx(1, "Unable to parse args: %s", tcpedit_geterr(tcpedit));
    } else if (rcode == 1) {
        warnx("%s", tcpedit_geterr(tcpedit));
    }

    /* validate the options against the DLT type(s) actually seen */
    if (tcpedit_validate(tcpedit) < 0) {
        errx(1, "Unable to edit packets given options/DLT types:\n%s", tcpedit_geterr(tcpedit));
    }

    /* rewrite_packets() is tcprewrite's own loop -- libtcpedit doesn't drive pcap I/O for you */
    if (rewrite_packets(tcpedit, options.pin, options.pout) != 0)
        errx(1, "Error rewriting packets: %s", tcpedit_geterr(tcpedit));

    tcpedit_close(&tcpedit);
    pcap_dump_close(options.pout);
    pcap_close(options.pin);
    return 0;
}
```

Note `tcpedit_init()` takes the input pcap's DLT directly
(`pcap_datalink(options.pin)`), and `tcpedit_close()` takes a `tcpedit_t **` (it
frees the context and nulls your pointer) — check the current header for exact
signatures before copying old snippets verbatim, as both have changed shape since
this guide was first written in 2009.

libtcpedit itself doesn't read/write pcap files or drive a packet loop — that's
on the caller (`rewrite_packets()` above is `tcprewrite.c`'s own function, not part
of the library). libtcpedit's job is turning one packet's bytes plus your
configured rewrite rules into new packet bytes.

## Configuration API (no AutoOpts)

Skip `tcpedit_post_args()` and instead call the `tcpedit_set_*()` functions in
`tcpedit_api.h` directly, still bracketed by `tcpedit_init()`/`tcpedit_validate()`:

```c
tcpedit_set_fixcsum(tcpedit, true);
tcpedit_set_mtu(tcpedit, 1500);
/* ... */
```

Layer 3+ options are set this way directly. Layer 2/DLT options are per-plugin —
first select an output DLT plugin:

```c
tcpedit_set_encoder_dltplugin_byname(tcpedit, "enet");   /* or _byid() with a DLT_* value */
```

then call that plugin's own setters, declared in its `<name>_api.h` (e.g.
`en10mb_api.h` for Ethernet, `hdlc_api.h`, `user_api.h`, ...).

Every setter returns `TCPEDIT_OK` or `TCPEDIT_ERROR` — on error, call
`tcpedit_geterr()` for details.

See also: [DLT Plugin Developer Guide](../developers/writing-dlt-plugins.md) (writing a
new DLT plugin) and
[Tcpedit Plugin Architecture](../developers/plugin-architecture.md) (the design
rationale behind the plugin system).

---
*Adapted from the [Tcpreplay developer wiki](https://github.com/appneta/tcpreplay/wiki/Using-libtcpedit).*
