sources = run_command('find', '.', '-type', 'f', '-name', '*[.c,.h]').stdout().strip().split('\n')

executable(
    'client-example',
    sources : sources,
    dependencies : [
        Heaven_Client_dep,
        declare_dependency(link_args : libs_paths),
    ])

