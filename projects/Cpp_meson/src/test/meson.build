test_sources = [
    'main.cpp',
    'parseDict.cpp',
]
test_exe = executable(
    'test-wordle',
    test_sources,
    include_directories: inc,
    dependencies: thread_dep)

test('wordle test', test_exe)

