asciidoctor = find_program('asciidoctor', required : get_option('docs'))

if asciidoctor.found()
    html_targets = []
    foreach doc : man_page_defs
        base = doc[0]
        src = doc[1]

        # Installed man page (built by default).
        custom_target(base + '.1',
            input : src,
            output : base + '.1',
            command : [asciidoctor, '-b', 'manpage', '-o', '@OUTPUT@', '@INPUT@'],
            install : true,
            install_dir : get_option('mandir') / 'man1',
        )

        # Maintainer-only HTML page (not built by default, not installed).
        html_targets += custom_target(base + '.html',
            input : src,
            output : base + '.html',
            command : [asciidoctor, '-b', 'html5', '-o', '@OUTPUT@', '@INPUT@'],
            build_by_default : false,
            install : false,
        )
    endforeach

    alias_target('html', html_targets)
endif
