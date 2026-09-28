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
    'include/xtd/allocators/NewAllocators.h',
    'include/xtd/allocators/Traits.h',

    # threading
    'include/xtd/threading/NativeThread.h',
    'include/xtd/threading/Thread.h',
    'include/xtd/threading/Latch.h',
    'include/xtd/threading/Mutex.h',
    'include/xtd/threading/ScopedLock.h',
    'include/xtd/threading/SharedMutex.h',
    'include/xtd/threading/TinyMutex.h',

    # platform
    'include/xtd/platform/Utilities.h',
    'include/xtd/platform/Macros.h',

    # time
    'include/xtd/time/Detail.h',
    'include/xtd/time/Duration.h',
    'include/xtd/time/GpsClock.h',
    'include/xtd/time/HighResolutionClock.h',
    'include/xtd/time/ProcessCpuClock.h',
    'include/xtd/time/Ratio.h',
    'include/xtd/time/SteadyClock.h',
    'include/xtd/time/SystemClock.h',
    'include/xtd/time/TaiClock.h',
    'include/xtd/time/TimePoint.h',
    'include/xtd/time/UtcClock.h',
    'include/xtd/time/Traits.h',

    # utility
    'include/xtd/utility/Functions.h',

    # math
    'include/xtd/math/Utilities.h',
    'include/xtd/math/Uint.h',
    'include/xtd/math/UintOperations.h',
)

xtd_sources = files(
    # threading
    'src/xtd/threading/NativeThread.cpp',

    # time
    'src/xtd/time/Detail.cpp',

    # platform
    'src/xtd/platform/Utilities.cpp',
)

is_windows = host_machine.system() == 'windows'

xtd_deps = []
if is_windows
    # WaitOnAddress / WakeByAddress* live in Synchronization.lib
    xtd_deps += meson.get_compiler('cpp').find_library('synchronization')
endif

xtd_libs = both_libraries('xtd',
    xtd_sources,
    include_directories : inc_dir,
    dependencies : xtd_deps,
    install : true,
    version : meson.project_version(),
    soversion : '0',
    cpp_args : ['-DXTD_BUILD_SHARED'],
)

xtd_dep_shared = declare_dependency(
    include_directories : inc_dir,
    link_with : xtd_libs.get_shared_lib(),
    compile_args : ['-DXTD_SHARED'],
)

# A static lib doesn't link anything itself, so consumers need the
# system lib too. (Meson usually propagates this automatically, but
# being explicit is harmless.)
xtd_dep_static = declare_dependency(
    include_directories : inc_dir,
    link_with : xtd_libs.get_static_lib(),
    dependencies : xtd_deps,
)

xtd_dep = get_option('default_library') == 'static' ? xtd_dep_static : xtd_dep_shared

meson.override_dependency('xtd', xtd_dep)

install_headers(xtd_headers, subdir: 'xtd', preserve_path : true)

pkg = import('pkgconfig')
pkg.generate(
    xtd_libs,
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
    # 'tests/xtd/allocators/NewAllocator.cpp',#
    # 'tests/xtd/allocators/Traits.cpp',

    # threading
    # 'tests/xtd/threading/NativeThread.cpp',
    'tests/xtd/threading/Thread.cpp',
    'tests/xtd/threading/Mutex.cpp',
    'tests/xtd/threading/TinyMutex.cpp',
    # 'tests/xtd/threading/Latch.cpp',

    # math
    'tests/xtd/math/UintOperations.cpp',
)

test_exe = executable('xtd_test', xtd_test_sources, 
    include_directories : inc_dir, dependencies: [xtd_dep, gtest_dep])

test('unit_tests', test_exe)