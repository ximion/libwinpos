
protocol_specs = [
  ['xdg-shell',  wayland_proto_datadir / 'stable/xdg-shell/xdg-shell.xml'],
  ['xx-zones-v1', meson.project_source_root() / 'protocol/xx-zones-v1.xml'],
]

qt_wayland_sources = []
foreach spec : protocol_specs
  stem     = spec[0]
  xml_path = spec[1]

  # Qt C++ wrapper header
  qt_wrapper_h = custom_target(stem + '-qt-wrapper-h',
    output:  'qwayland-' + stem + '.h',
    command: [qtwaylandscanner, 'client-header', xml_path],
    capture: true,
  )

  # Qt C++ wrapper implementation
  qt_wayland_sources += custom_target(stem + '-qt-wrapper-cpp',
    output:  'qwayland-' + stem + '.cpp',
    command: [qtwaylandscanner, 'client-code', xml_path],
    capture: true,
    depends: qt_wrapper_h,
  )

  # Raw C protocol header
  c_proto_h = custom_target(stem + '-c-proto-h',
      output:  'wayland-' + stem + '-client-protocol.h',
      command: [wayland_scanner, 'client-header', xml_path, '@OUTPUT@'],
  )

  # Raw C protocol implementation
  qt_wayland_sources += custom_target(stem + '-c-proto-c',
      output:  'wayland-' + stem + '-protocol.c',
      command: [wayland_scanner, 'private-code', xml_path, '@OUTPUT@'],
      depends: c_proto_h,
  )
endforeach

winposqt_hdr = [
    'Winpos/Manager.h',
    'Winpos/Zone.h',
    'Winpos/ZoneItem.h',
    'Winpos/WindowPositioner.h',
    'Winpos/winposqt_export.h',
]

winposqt_src = [
    'manager.cpp',
    'zone.cpp',
    'zoneitem.cpp',
    'windowpositioner.cpp',
]

winposqt_inc = include_directories('.')

winposqt_moc = qt.compile_moc(
  headers: files(
    'Winpos/Manager.h',
    'Winpos/Zone.h',
    'Winpos/ZoneItem.h',
    'Winpos/WindowPositioner.h',
  ),
  include_directories: [winposqt_inc],
  dependencies: [qt6_dep, qt6_priv_dep],
)

winposqt_lib = library('winpos-qt',
  sources: [
    winposqt_src,
    qt_wayland_sources,
    winposqt_moc,
  ],
  include_directories: [winposqt_inc],
  dependencies: [
    qt6_dep,
    qt6_priv_dep,
    wl_client_dep
  ],
  cpp_args: ['-DWINPOSQT_BUILD_LIBRARY'],
  version: meson.project_version(),
  soversion: '0',
  install: true,
)

winpos_qt_dep = declare_dependency(
  link_with: winposqt_lib,
  include_directories: winposqt_inc,
  dependencies: [qt6_dep],
)

install_headers(winposqt_hdr, subdir: 'Winpos')

pkgconfig = import('pkgconfig')
pkgconfig.generate(winposqt_lib,
  name: 'winpos-qt',
  description: 'Wayland/X11 window positioning library for Qt',
  version: meson.project_version(),
  requires: ['Qt6Core', 'Qt6Gui'],
)
