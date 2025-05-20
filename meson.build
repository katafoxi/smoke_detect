project('segmsmoke_app', 'c',
  version : '1.0',
  default_options : [
    'warning_level=3',
    'c_std=c11',
    'buildtype=release'
  ])

# Определение версий и путей
cuda_ver = '12.2'
nvds_version = '7.0'
lib_install_dir = '/opt/nvidia/deepstream/deepstream-' + nvds_version + '/lib/'
app_install_dir = '/opt/nvidia/deepstream/deepstream-' + nvds_version + '/bin/'

# Определение целевой платформы
target_device = run_command('gcc', '-dumpmachine', check: true).stdout().split('-')[0]
if target_device == 'aarch64'
  add_project_arguments('-DPLATFORM_TEGRA', language: 'c')
endif

# Зависимости через pkg-config
pkgs = ['gstreamer-1.0']
deps = []
foreach pkg : pkgs
  deps += dependency(pkg)
endforeach

# Дополнительные зависимости
math_dep = meson.get_compiler('c').find_library('m', required: true)
cuda_dep = dependency('cuda', version: '==' + cuda_ver, required: true)

# Пути для DeepStream
deepstream_inc_dir = '/opt/nvidia/deepstream/deepstream-' + nvds_version + '/sources/includes'
cuda_inc_dir = '/usr/local/cuda-' + cuda_ver + '/include'

# Проверка существования путей
# if not run_command('[', '-d', deepstream_inc_dir, ']', check: false).returncode() == 0
#   error('DeepStream includes directory not found: ' + deepstream_inc_dir)
# endif

# if not run_command('[', '-d', cuda_inc_dir, ']', check: false).returncode() == 0
#   error('CUDA includes directory not found: ' + cuda_inc_dir)
# endif

# Компиляция приложения
executable('segmsmoke_app',
  sources: files(
    './src/deepstream_segmentation_app.c',  # Добавьте здесь все другие .c файлы, если они есть
    './src/callback.c',
    './src/pipeline.c'
  ),
  include_directories: [
    include_directories(deepstream_inc_dir),
    include_directories(cuda_inc_dir),
    # Добавьте другие include директории, если нужно
  ],
  dependencies: [
    deps,
    math_dep,
    cuda_dep,
  ],
  link_args: [
    '-L' + lib_install_dir,
    '-lnvdsgst_helper',
    '-lnvdsgst_meta',
    '-L/usr/local/cuda-' + cuda_ver + '/lib64/',
    '-lcudart',
    '-lcuda',
    '-Wl,-rpath,' + lib_install_dir
  ],
  install: true,
  install_dir: app_install_dir
)