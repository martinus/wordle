lib_sources = [
    'wordle/parseDict.cpp',
    'wordle/State.cpp',
    'wordle/Word.cpp',
]

link_args = '-lpthread'

lib_inc = include_directories('.')
wordle_lib = static_library(
    'wordle',
    lib_sources,
    dependencies: thread_dep,
)

