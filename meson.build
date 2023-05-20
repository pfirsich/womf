project('womf', 'cpp',
  default_options: ['warning_level=3', 'cpp_std=c++20'])

glwx_dep = dependency('glwrap', fallback: ['glwrap', 'glwx_dep'])
gltf_dep = dependency('gltf', fallback: ['gltf', 'gltf_dep'])

womf_src = [
  'src/main.cpp',
  'src/threadpool.cpp',
]

executable('womf', womf_src,
  dependencies: [
    glwx_dep,
    gltf_dep,
  ]
)
