project(
    'Heaven',
    'c', 
    default_options : ['c_std=c99'],
    version : '1.0.0')
 
headers = [
	'src/lib/Heaven-Common.h',
	'src/lib/Heaven-Server-Client-Common.h',
	'src/lib/Heaven-Server-Compositor-Common.h',
	'src/lib/Heaven-Server.h',
	'src/lib/Heaven-Client.h',
	'src/lib/Heaven-Compositor.h'
]

install_headers(headers, subdir : 'Heaven')

Heaven_Server = shared_library('Heaven-Server',
    sources : [
    	'src/lib/Heaven-Common.h',
    	'src/lib/Heaven-Common.c',
    	'src/lib/Heaven-Server.h',
    	'src/lib/Heaven-Server.c',
   	'src/lib/Heaven-Server-Client-Common.h',
   	'src/lib/Heaven-Server-Client-Common.c',
	'src/lib/Heaven-Server-Compositor-Common.h',
	'src/lib/Heaven-Server-Compositor-Common.c'
    ],
    install : true)

Heaven_Server_dep = declare_dependency(link_with : Heaven_Server)
    
Heaven_Client = shared_library('Heaven-Client',
    sources : [
    	'src/lib/Heaven-Common.h',
    	'src/lib/Heaven-Common.c',
    	'src/lib/Heaven-Client.h',
    	'src/lib/Heaven-Client.c',
   	'src/lib/Heaven-Server-Client-Common.h',
   	'src/lib/Heaven-Server-Client-Common.c',
    ],
    install : true)

Heaven_Client_dep = declare_dependency(link_with : Heaven_Client)

Heaven_Compositor = shared_library('Heaven-Compositor',
    sources : [
    	'src/lib/Heaven-Common.h',
    	'src/lib/Heaven-Common.c',
    	'src/lib/Heaven-Compositor.h',
    	'src/lib/Heaven-Compositor.c',
   	'src/lib/Heaven-Server-Compositor-Common.h',
   	'src/lib/Heaven-Server-Compositor-Common.c',
    ],
    install : true)

Heaven_Compositor_dep = declare_dependency(link_with : Heaven_Compositor)

subdir('src/examples/server')
subdir('src/examples/client')
subdir('src/examples/compositor')



