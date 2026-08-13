project('xtd', 'cpp', version : '0.1.0', default_options : ['cpp_std=c++20', 'warning_level=3'])

inc_dir = include_directories('include')

xtd_headers = files(
    #debug
    'include/xtd/debug/Checks.h',

    # containers
    'include/xtd/containers/Traits.h',
    'include/xtd/containers/Array.h',
    'include/xtd/containers/Vector.h',
    'include/xtd/containers/Span.h',
    'include/xtd/containers/Iterators.h',

    # memory
    'include/xtd/memory/Traits.h',
    'include/xtd/memory/UniquePtr.h',
    'include/xtd/memory/SharedPtr.h',
    'include/xtd/memory/WeakPtr.h',
    'include/xtd/memory/StrongPtr.h',

    # allocators
    'include/xtd/allocators/NewAllocator.h',
    'include/xtd/allocators/Traits.h',

    # threading
    'include/xtd/threading/Thread.h',
)

xtd_sources = files(

    # threading
    'src/threading/Thread.cpp',
)

xtd_lib = library('xtd',
    xtd_sources,
    include_directories : inc_dir,
    install : true,
    version : meson.project_version(),
    soversion : '0',
)

xtd_dep = declare_dependency(
    include_directories : inc_dir,
    link_with : xtd_lib,
)

meson.override_dependency('xtd', xtd_dep)

install_headers(xtd_headers, subdir: 'xtd', preserve_path : true)

pkg = import('pkgconfig')
pkg.generate(
    xtd_lib,
    name: 'xtd',
    description: 'An extended std library for c++',
    version: '0.1.0',
    subdirs: ['.'] # Installs include/ directly into the target include path
)

gtest_dep = dependency('gtest', main: true, 
    fallback: ['gtest', 'gtest_main_dep'],
    default_options: ['default_library=static'])

xtd_test_sources = files(
    # #macros
    # 'tests/xtd/debug/Checks.cpp',

    # # containers
    # 'tests/xtd/containers/Traits.cpp',
    # 'tests/xtd/containers/Array.cpp',
    # 'tests/xtd/containers/Vector.cpp',
    # 'tests/xtd/containers/Span.cpp',
    # 'tests/xtd/containers/Iterators.cpp',

    # memory
    # 'tests/xtd/memory/Traits.cpp',
    'tests/xtd/memory/UniquePtr.cpp',
    # 'tests/xtd/memory/SharedPtr.cpp',
    # 'tests/xtd/memory/WeakPtr.cpp',
    # 'tests/xtd/memory/StrongPtr.cpp',

    # # allocators
    # 'tests/xtd/allocators/NewAllocator.cpp',
    # 'tests/xtd/allocators/Traits.cpp',
)

test_exe = executable('xtd_test', xtd_test_sources, 
    include_directories : inc_dir, dependencies: [xtd_dep, gtest_dep])

test('unit_tests', test_exe)