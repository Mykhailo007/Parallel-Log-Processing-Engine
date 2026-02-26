# CMake generated Testfile for 
# Source directory: /home/runner/work/Parallel-Log-Processing-Engine/Parallel-Log-Processing-Engine
# Build directory: /home/runner/work/Parallel-Log-Processing-Engine/Parallel-Log-Processing-Engine/_codeql_build_dir
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(UnitTests "/home/runner/work/Parallel-Log-Processing-Engine/Parallel-Log-Processing-Engine/_codeql_build_dir/logengine_tests")
set_tests_properties(UnitTests PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/Parallel-Log-Processing-Engine/Parallel-Log-Processing-Engine/CMakeLists.txt;57;add_test;/home/runner/work/Parallel-Log-Processing-Engine/Parallel-Log-Processing-Engine/CMakeLists.txt;0;")
subdirs("_deps/doctest-build")
