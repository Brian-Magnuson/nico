# nico_core CMake configuration

# Front end utility files
set(FRONT_END_UTILS_SRC
    src/frontend/utils/nodes.cpp
    src/frontend/utils/ast_node.cpp
    src/frontend/utils/symbol_node.cpp
    src/frontend/utils/type_node.cpp
    src/frontend/utils/symbol_tree.cpp
    src/frontend/utils/control_stack.cpp
    src/frontend/utils/expression_checker.cpp
    src/frontend/utils/annotation_checker.cpp
    src/frontend/utils/mir.cpp
)

# Front end component files
set(FRONT_END_COMPONENTS_SRC
    src/frontend/components/code_generator.cpp
    src/frontend/components/mir_builder.cpp
    src/frontend/components/global_checker.cpp
    src/frontend/components/lexer.cpp
    src/frontend/components/local_checker.cpp
    src/frontend/components/parser.cpp
)

# Front end files
set(FRONT_END_SRC
    src/frontend/frontend.cpp
    ${FRONT_END_COMPONENTS_SRC}
    ${FRONT_END_UTILS_SRC}
)

# Back end files
set(BACK_END_SRC
    src/backend/emitter.cpp
    src/backend/jit.cpp
    src/backend/optimizer.cpp
)

set(DRIVER_SRC
    src/driver/repl.cpp
    src/driver/jit_runner.cpp
    src/driver/aot_runner.cpp
)

# Shared files
set(SHARED_SRC
    src/shared/diagnostics.cpp
    src/shared/utils.cpp
)

# Core source files
set(CORE_SRC
    ${FRONT_END_SRC}
    ${BACK_END_SRC}
    ${SHARED_SRC}
    ${DRIVER_SRC}
)

# Library target
add_library(nico_core STATIC ${CORE_SRC})
target_compile_definitions(nico_core
    PUBLIC NICO_RT_LIB_PATH="$<TARGET_FILE:nico_rt>"
)
target_include_directories(nico_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(nico_core ${llvm_libs})
