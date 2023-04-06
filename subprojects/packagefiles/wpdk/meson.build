project('WPDK', 'C', 'CPP',
	version: '0.1.0',
	license: 'BSD',
	default_options: [
		'buildtype=release',
		'default_library=static',
		'b_vscrt=none',
		'warning_level=3'
		],
	meson_version: '>= 0.55.0'
)

cc = meson.get_compiler('c')
cpp = meson.get_compiler('cpp')

cunit = subproject('CUnit')
cunit_dep = cunit.get_variable('cunit_dep')

machine_args = []
link_static_libs = []
link_libs = ['-lws2_32', '-lrpcrt4', '-liphlpapi']

global_includes = include_directories('.', 'inc')

warnings = [
	'-Wdeprecated',
	'-Wformat',
	'-Wformat-security',
	'-Wnested-externs',
	'-Wpointer-arith',
	'-Wsign-compare',
	'-Wundef',
]

foreach warning : warnings
	if cc.has_argument(warning)
		add_project_arguments(warning, language: 'c')
	endif
endforeach

if cc.get_id() == 'gcc'
	link_static_libs += '-l:libssp.a'
endif

subdir('scripts')
subdir('config')

subdir('inc')
subdir('src')
subdir('tools')

wpdk_static_dep = declare_dependency(
	include_directories : global_includes,
	link_args : link_libs,
	link_with : [ static_lib ]
)

wpdk_shared_dep = declare_dependency(
	include_directories : global_includes,
	link_args : link_libs,
	link_with : [ shared_lib ]
)

wpdk_dep = wpdk_static_dep

subdir('scripts/pkg-config')

meson.override_dependency('wpdk', wpdk_dep)
