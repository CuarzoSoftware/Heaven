project(
    'Heaven',
    'c', 
    default_options : ['c_std=c99'],
    version : '1.0.0')
 
if get_option('buildtype').startswith('debug')
    add_project_arguments('-DDEBUG', '-O0', '-g', language : 'c')
else
    add_project_arguments('-Ofast', '-g3',  language : 'c')
endif

fs = import('fs')

include_paths = []

includes = []

foreach p : include_paths
    if fs.is_dir(p)
      includes += [include_directories(p)]
    endif
endforeach


libs_paths = [

]

Heaven_Server = library(
    'Heaven-Server',
    sources : run_command('find', './src/lib/server/', '-type', 'f', '-name', '*.c').stdout().strip().split('\n') +
    		run_command('find', './src/lib/common/', '-type', 'f', '-name', '*.c').stdout().strip().split('\n'),
    include_directories : includes,
    dependencies : [])

Heaven_Server_dep = declare_dependency(
    dependencies: [], 
    include_directories : includes,
    link_with : Heaven_Server)
    
Heaven_Client = library(
    'Heaven-Client',
    sources : run_command('find', './src/lib/client/', '-type', 'f', '-name', '*.c').stdout().strip().split('\n') +
    		run_command('find', './src/lib/common/', '-type', 'f', '-name', '*.c').stdout().strip().split('\n'),
    include_directories : includes,
    dependencies : [])

Heaven_Client_dep = declare_dependency(
    dependencies: [], 
    include_directories : includes,
    link_with : Heaven_Client)

subdir('src/examples/server')
subdir('src/examples/client')



