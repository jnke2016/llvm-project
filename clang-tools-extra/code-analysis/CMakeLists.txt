set(LLVM_LINK_COMPONENTS support)

add_clang_executable(code-analysis
  CodeAnalysis.cpp
  )
target_link_libraries(code-analysis
  PRIVATE
  clangAST
  clangASTMatchers
  clangBasic
  clangFrontend
  clangSerialization
  clangTooling
  )