asciidoctor = find_program('asciidoctor', required : get_option('docs'))

if asciidoctor.found()
    # Each entry: the tool's base name, and any extra .adoc files it includes
    doc_sources = [
        ['tcpreplay',      []],
        ['tcpprep',        []],
        ['tcprewrite',     ['tcpedit_options.adoc']],
        ['tcpreplay-edit', ['tcpedit_options.adoc']],
        ['tcpcapinfo',     []],
    ]

    if enable_tcpbridge
        doc_sources += [['tcpbridge', ['tcpedit_options.adoc']]]
    endif

    if enable_tcpliveplay
        doc_sources += [['tcpliveplay', []]]
    endif

    html_targets = []
    foreach doc : doc_sources
        base = doc[0]
        src = base + '.1.adoc'
        includes = files(doc[1])

        # Installed man page (built by default).
        custom_target(base + '.1',
            input : src,
            output : base + '.1',
            command : [asciidoctor, '-b', 'manpage', '-o', '@OUTPUT@', '@INPUT@'],
            depend_files : includes,
            install : true,
            install_dir : get_option('mandir') / 'man1',
        )

        # Maintainer-only HTML page (not built by default, not installed).
        html_targets += custom_target(base + '.html',
            input : src,
            output : base + '.html',
            command : [asciidoctor, '-b', 'html5', '-o', '@OUTPUT@', '@INPUT@'],
            depend_files : includes,
            build_by_default : false,
            install : false,
        )
    endforeach

    alias_target('html', html_targets)
endif
