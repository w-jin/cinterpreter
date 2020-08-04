cmake_minimum_required(VERSION 3.2)
project(cinterpreter)

# set(CMAKE_VERBOSE_MAKEFILE ON)

# execute_process(COMMAND llvm-config --cxxflags OUTPUT_VARIABLE CMAKE_CXX_FLAGS)
# execute_process(COMMAND llvm-config --ldflags OUTPUT_VARIABLE CMAKE_EXE_LINKER_FLAGS)

include_directories(/usr/lib/llvm-6.0/include)
link_directories(/usr/lib/llvm-6.0/lib)

set(SRC_LIST cinterpreter.cpp)
add_executable(cinterpreter ${SRC_LIST})

target_link_libraries(cinterpreter clangFrontend clangTooling clangParse clangSema clangAnalysis clangEdit clangAST clangLex clangBasic clangDriver clangSerialization LLVM clang)
