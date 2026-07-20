project(
    'cz-heaven',
    'c','cpp',
    version : run_command('cat', files('VERSION'), check : false).stdout().strip(),
    meson_version: '>= 0.60.0',
    default_options: [
        'warning_level=3',
        'buildtype=release',
        'cpp_std=c++23'
    ]
)

cpp = meson.get_compiler('cpp')
NAME='Heaven'
VERSION_MAJOR = meson.project_version().split('.')[0]
VERSION_MINOR = meson.project_version().split('.')[1]
VERSION_PATCH = meson.project_version().split('.')[2]
VERSION_BUILD = run_command('cat', './BUILD', check : false).stdout()
HEADERS_INSTALL_DIR = join_paths(get_option('prefix'), get_option('includedir'), 'CZ/', NAME)
CONF_FILE_NAME = 'CZ' + NAME + 'Version.h'

conf = configuration_data()
conf.set('VERSION_MAJOR', VERSION_MAJOR)
conf.set('VERSION_MINOR', VERSION_MINOR)
conf.set('VERSION_PATCH', VERSION_PATCH)
conf_file = configure_file(
    input : 'src/CZ/' + NAME + '/' + CONF_FILE_NAME +'.in',
    output : CONF_FILE_NAME,
    configuration : conf)

install_data(
    conf_file,
    install_dir: HEADERS_INSTALL_DIR)

# -------------- DEPENDENCIES --------------

pkg                 = import('pkgconfig')
cz_core_dep         = dependency('cz-core', fallback:['cz-core', 'cz_core_dep'])
sdbus_dep           = dependency(['libsystemd', 'libelogind', 'basu'])

# -------------- HEADERS --------------

include_paths = ['src/', 'src/CZ']

# All headers
headers = run_command('find', 'src/CZ', '-type', 'f', '-name', '*.h', check : false).stdout().strip().split('\n')

# Public and private API headers
header_dirs = [
    ['src/CZ/Heaven', ''],
    ['src/CZ/Heaven/Bar', 'Bar'],
    ['src/CZ/Heaven/Client', 'Client'],
    ['src/CZ/Heaven/Compositor', 'Compositor'],
]

foreach header_dir : header_dirs
    header_files = run_command('find', header_dir[0], '-type', 'f', '-name', '*.h', '-maxdepth', '1', check : false).stdout().strip().split('\n')
    install_headers(header_files, install_dir : join_paths(HEADERS_INSTALL_DIR, header_dir[1]))
    include_paths += include_directories(header_dir[0])
endforeach

deps = [
    cz_core_dep,
    sdbus_dep
]

# -------------- BAR --------------

cz_heaven_bar = library(
    'cz-heaven-bar',
    sources : run_command('find', 'src/CZ/Heaven/Bar', '-type', 'f', '-name', '*[.cpp,.c]', check : false).stdout().strip().split('\n'),
    include_directories : include_paths,
    dependencies : deps,
    soversion: VERSION_MAJOR,
    install : true)

cz_heaven_bar_dep = declare_dependency(
    dependencies: deps,
    include_directories : include_paths,
    link_with : cz_heaven_bar)

pkg.generate(
    cz_heaven_bar,
    name: 'cz-heaven-bar',
    description: 'Global menu library',
    version: meson.project_version(),
    libraries: deps,
    subdirs: ['CZ','CZ/Heaven'],
    requires: ['cz-core'],
    filebase: 'cz-heaven-bar')


# -------------- COMPOSITOR --------------

cz_heaven_compositor = library(
    'cz-heaven-compositor',
    sources : run_command('find', 'src/CZ/Heaven/Compositor', '-type', 'f', '-name', '*[.cpp,.c]', check : false).stdout().strip().split('\n'),
    include_directories : include_paths,
    dependencies : deps,
    soversion: VERSION_MAJOR,
    install : true)

cz_heaven_compositor_dep = declare_dependency(
    dependencies: deps,
    include_directories : include_paths,
    link_with : cz_heaven_compositor)

pkg.generate(
    cz_heaven_compositor,
    name: 'cz-heaven-compositor',
    description: 'Global menu library',
    version: meson.project_version(),
    libraries: deps,
    subdirs: ['CZ','CZ/Heaven'],
    requires: ['cz-core'],
    filebase: 'cz-heaven-compositor')

# -------------- CLIENT --------------

cz_heaven_client = library(
    'cz-heaven-client',
    sources : run_command('find', 'src/CZ/Heaven/Client', '-type', 'f', '-name', '*[.cpp,.c]', check : false).stdout().strip().split('\n'),
    include_directories : include_paths,
    dependencies : deps,
    soversion: VERSION_MAJOR,
    install : true)

cz_heaven_client_dep = declare_dependency(
    dependencies: deps,
    include_directories : include_paths,
    link_with : cz_heaven_client)

pkg.generate(
    cz_heaven_client,
    name: 'cz-heaven-client',
    description: 'Global menu library',
    version: meson.project_version(),
    libraries: deps,
    subdirs: ['CZ','CZ/Heaven'],
    requires: ['cz-core'],
    filebase: 'cz-heaven-client')

# -------------- SUBDIRS --------------

subdir('examples/bar')
subdir('examples/compositor')
subdir('examples/client')

