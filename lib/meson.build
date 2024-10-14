# Bundled strlcpy/strlcat implementation, built only when the C library does
# not provide them (mirrors the !SYSTEM_STRLCPY conditional in lib/Makefile.am).
# `libstrl` is consumed as a link dependency by libcommon and the programs; it
# is an empty list when the system provides strlcpy/strlcat so it can always be
# referenced unconditionally.
if have_strlcpy
    libstrl = []
else
    libstrl = static_library('strl',
        files('strlcat.c', 'strlcpy.c'),
        include_directories : configuration_inc,
        install : false,
    )
endif
