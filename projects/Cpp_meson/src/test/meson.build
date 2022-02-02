test_sources = [
    'main.cpp',
    'parseDict.cpp',
]

wordle_data_dir = '-DWORDLE_DATA_DIR="@0@"'.format(meson.global_source_root())
test_exe = executable(
    'test-wordle',
    test_sources,
    include_directories: inc,
    dependencies: thread_dep,
    cpp_args: wordle_data_dir
)

test('wordle test', test_exe)

