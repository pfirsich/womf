project('womf', 'cpp',
  default_options: ['warning_level=3', 'cpp_std=c++20'])

glwx_dep = dependency('glwrap', fallback: ['glwrap', 'glwx_dep'])
gltf_dep = dependency('gltf', fallback: ['gltf', 'gltf_dep'])
luajit_dep = dependency('luajit')

sol2_dep = declare_dependency(include_directories: 'deps/sol_v3.3.0')

womf_src = [
  'src/main.cpp',
  #'src/resource.cpp',
  #'src/scene.cpp',
  #'src/threadpool.cpp',
  #'src/model.cpp',
]

executable('womf', womf_src,
  dependencies: [
    glwx_dep,
    gltf_dep,
    luajit_dep,
    sol2_dep,
  ]
)
