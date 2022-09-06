sources = run_command('find', '.', '-type', 'f', '-name', '*[.c,.h]').stdout().strip().split('\n')

executable(
    'server-example',
    sources : sources,
    dependencies : [
        Heaven_Server_dep,
        declare_dependency(link_args : libs_paths),
    ])

