# generate with
#
# CXX="ccache clang++" meson setup --buildtype release build/clang_release
# CXX="ccache clang++" meson setup build/clang_debug
# meson setup --buildtype release build/g++_release
# meson setup build/g++_debug

project('wordle', 'cpp',
    version: '1.0.0',
    license: 'MIT',
    default_options : ['cpp_std=c++17'])

# add dependency for threads (-lpthread, see https://mesonbuild.com/howtox.html)
thread_dep = dependency('threads')

subdir('src/lib')
subdir('src/app')
subdir('src/test')
