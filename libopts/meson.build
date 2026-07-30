libopts_inc = include_directories('.', 'autoopts')

libopts = static_library('opts',
    files('libopts.c'),
    c_args : ['-w'],
    include_directories : [configuration_inc, libopts_inc],
    install : false,
)
