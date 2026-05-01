
qt6_widgets_dep = dependency('qt6', modules: ['Widgets'], version: '>=6.8.2')

demo_moc = qt.compile_moc(
  headers: files('demowindow.h'),
  include_directories: include_directories('.'),
  dependencies: [qt6_dep, qt6_widgets_dep],
)

executable('winpos-demo',
  sources: [
    'main.cpp',
    'demowindow.cpp',
    demo_moc,
  ],
  dependencies: [winpos_qt_dep, qt6_widgets_dep],
  install: false,
)
