#
# cmake file
#

set (library_name emitters)

include (LLVMSetup)
if(NOT LLVM_FOUND)
  message("LLVM unavailable. Compilers disabled.")
  return()
endif()

set (src
    src/EmitterTypes.cpp
    src/IRAssemblyWriter.cpp
    src/IRAsyncTask.cpp
    src/IRBlockRegion.cpp
    src/IRDiagnosticHandler.cpp
    src/IREmitter.cpp
    src/IRExecutionEngine.cpp
    src/IRFunctionEmitter.cpp
    src/IRHeaderWriter.cpp
    src/IRIfEmitter.cpp
    src/IRLoader.cpp
    src/IRLocalArray.cpp
    src/IRLocalMultidimArray.cpp
    src/IRLocalScalar.cpp
    src/IRLocalValue.cpp
    src/IRLoopEmitter.cpp
    src/IRMath.cpp
    src/IRMetadata.cpp
    src/IRModuleEmitter.cpp
    src/IROptimizer.cpp
    src/IRParallelLoopEmitter.cpp
    src/IRPosixRuntime.cpp
    src/IRProfiler.cpp
    src/IRRuntime.cpp
    src/IRSwigInterfaceWriter.cpp
    src/IRTask.cpp
    src/IRThreadPool.cpp
    src/IRThreadUtilities.cpp
    src/LLVMUtilities.cpp
    src/ModuleEmitter.cpp
    src/TargetDevice.cpp
    src/Variable.cpp
)

set (include
    include/CompilableIRFunction.h
    include/CompilerOptions.h
    include/EmitterException.h
    include/EmitterTypes.h
    include/FunctionDeclaration.h
    include/IRAssemblyWriter.h
    include/IRAsyncTask.h
    include/IRBlockRegion.h
    include/IRDiagnosticHandler.h
    include/IREmitter.h
    include/IRExecutionEngine.h
    include/IRFunctionEmitter.h
    include/IRHeaderWriter.h
    include/IRIfEmitter.h
    include/IRLoader.h
    include/IRLocalArray.h
    include/IRLocalMultidimArray.h
    include/IRLocalScalar.h
    include/IRLocalValue.h
    include/IRLoopEmitter.h
    include/IRMath.h
    include/IRMetadata.h
    include/IRModuleEmitter.h
    include/IROptimizer.h
    include/IRParallelLoopEmitter.h
    include/IRPosixRuntime.h
    include/IRProfiler.h
    include/IRRuntime.h
    include/IRSwigInterfaceWriter.h
    include/IRTask.h
    include/IRThreadPool.h
    include/IRThreadUtilities.h
    include/LLVMInclude.h
    include/LLVMUtilities.h
    include/ModuleEmitter.h
    include/ScalarVariable.h
    include/SymbolTable.h
    include/TargetDevice.h
    include/Variable.h
    include/VectorVariable.h
)

set (templates
    templates/CppPredictWrapper.in
    templates/SwigModule.in
    templates/SwigPredictPython.in
    templates/SwigShapeWrappers.in
)

source_group("src" FILES ${src})
source_group("include" FILES ${include})
source_group("templates" FILES ${templates})

add_library(${library_name} ${src} ${include} ${templates})
target_include_directories(${library_name} PRIVATE include templates ${ELL_LIBRARIES_DIR})
target_include_directories(${library_name} SYSTEM PUBLIC ${LLVM_INCLUDE_DIRS})
target_link_libraries(${library_name} math utilities ${LLVM_LIBS})
target_compile_options(${library_name} PUBLIC ${LLVM_COMPILE_OPTIONS})

set_property(TARGET ${library_name} PROPERTY FOLDER "libraries")

#
# test project
#

set (test_name ${library_name}_test)

if(MSVC)
  set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ignore:4099")
endif()

set (test_src
  test/src/main.cpp
  test/src/AsyncEmitterTest.cpp
  test/src/IREmitterTest.cpp
  test/src/IRFunctionTest.cpp
  test/src/IRProfilerTest.cpp
  test/src/PosixEmitterTest.cpp
  test/src/StdlibEmitterTest.cpp
)

set (test_include
  test/include/AsyncEmitterTest.h
  test/include/IREmitterTest.h
  test/include/IRFunctionTest.h
  test/include/IRProfilerTest.h
  test/include/PosixEmitterTest.h
  test/include/StdlibEmitterTest.h
)

source_group("src" FILES ${test_src})
source_group("include" FILES ${test_include})

add_executable(${test_name} ${test_src} ${test_include})
target_include_directories(${test_name} PRIVATE test/include ${ELL_LIBRARIES_DIR})
target_link_libraries(${test_name} testing utilities emitters)
copy_shared_libraries(${test_name})

set_property(TARGET ${test_name} PROPERTY FOLDER "tests")
add_test(NAME ${test_name} COMMAND ${test_name})
set_test_library_path(${test_name})
