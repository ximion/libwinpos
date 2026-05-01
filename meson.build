project('libwinpos', ['cpp', 'c'],
  version: '0.1.0',
  license: 'MIT',
  default_options: [
    'cpp_std=c++23',
    'warning_level=2',
  ],
  meson_version: '>=1.2.0',
)

qt = import('qt6')

qt6_dep = dependency('qt6',
  modules: [
    'Core',
    'Gui',
    'WaylandClient'
  ],
  version: '>=6.8.2',
)
qt6_priv_dep = dependency('Qt6GuiPrivate', version: '>=6.8.2')
wl_client_dep = dependency('wayland-client')

wayland_proto_dep = dependency('wayland-protocols')
wayland_proto_datadir = wayland_proto_dep.get_variable(pkgconfig: 'pkgdatadir')

qtwaylandscanner = find_program('qtwaylandscanner', dirs: [qt6_dep.get_variable(pkgconfig: 'libexecdir')])
wayland_scanner = find_program('wayland-scanner')

subdir('src')

if get_option('examples')
  subdir('examples')
endif
