cmake_minimum_required(VERSION 3.20)
project(hwc LANGUAGES CXX)

set(HWC_EXTERNAL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../external")

option(HWC_STATIC_PACKAGE "Build a fully static formal-tool executable" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SLANG_SOURCE "${HWC_EXTERNAL_ROOT}/slang" CACHE PATH
    "Slang source tree")
set(SLANG_BUILD "${SLANG_SOURCE}/build" CACHE PATH "Slang build tree")
if(NOT EXISTS "${SLANG_BUILD}/lib/libsvlang.a")
    message(FATAL_ERROR
        "Build Slang first; missing ${SLANG_BUILD}/lib/libsvlang.a")
endif()

add_library(slang_prebuilt STATIC IMPORTED)
set_target_properties(slang_prebuilt PROPERTIES
    IMPORTED_LOCATION "${SLANG_BUILD}/lib/libsvlang.a"
    INTERFACE_COMPILE_DEFINITIONS "SLANG_STATIC_DEFINE;SLANG_USE_THREADS;SLANG_DEBUG"
    INTERFACE_INCLUDE_DIRECTORIES
        "${SLANG_SOURCE}/include;${SLANG_SOURCE}/external;${SLANG_BUILD}/source;${SLANG_BUILD}/_deps/boost_regex-src/include;${SLANG_BUILD}/_deps/tomlplusplus-src/include")

add_library(fmt_prebuilt STATIC IMPORTED)
set_target_properties(fmt_prebuilt PROPERTIES
    IMPORTED_LOCATION "${SLANG_BUILD}/lib/libfmtd.a"
    INTERFACE_INCLUDE_DIRECTORIES "${SLANG_BUILD}/_deps/fmt-src/include")

file(GLOB_RECURSE EIR_SOURCES CONFIGURE_DEPENDS eir/lib/*.cpp)
list(FILTER EIR_SOURCES EXCLUDE REGEX "CodeGen/Emit/Program(Reader|Writer|Disassembler)\\.cpp$")
file(GLOB_RECURSE VIR_SOURCES CONFIGURE_DEPENDS vir/lib/*.cpp)
file(GLOB_RECURSE FRONTEND_SVA_SOURCES CONFIGURE_DEPENDS
    frontend/lib/SVA/Lowering/*.cpp)
list(FILTER FRONTEND_SVA_SOURCES EXCLUDE REGEX
    "frontend/lib/SVA/Lowering/property\\.cpp$")
file(GLOB_RECURSE VIR_ABI_SOURCES CONFIGURE_DEPENDS vir/runtime/abi/lib/*.cpp)
file(GLOB_RECURSE BTOR2_SOURCES CONFIGURE_DEPENDS btor2/lib/*.cpp)
list(FILTER BTOR2_SOURCES EXCLUDE REGEX "btor2/lib/(Formal|Lowering/Formal|Transform/HIR)/.*\\.cpp$")
list(FILTER BTOR2_SOURCES EXCLUDE REGEX "btor2/lib/Conversion/HIRToIR\\.cpp$")
file(GLOB_RECURSE FORMAL_SOURCES CONFIGURE_DEPENDS
    btor2/lib/Formal/*.cpp
    btor2/lib/Lowering/Formal/*.cpp
    btor2/lib/Conversion/HIRToIR.cpp
    btor2/lib/Transform/*.cpp
    frontend/lib/SVA/Lowering/property.cpp
    formal/*.cpp
    )
list(FILTER FORMAL_SOURCES EXCLUDE REGEX "btor2/lib/Transform/IR/.*\\.cpp$")

add_library(btor2 ${BTOR2_SOURCES})
target_include_directories(btor2 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/btor2/include
    ${CMAKE_CURRENT_SOURCE_DIR}/frontend/include)
target_include_directories(btor2 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(vir_ir ${VIR_SOURCES})
target_include_directories(vir_ir PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/vir/include
    ${CMAKE_CURRENT_SOURCE_DIR}/vir)
target_include_directories(vir_ir PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(vir_ir PRIVATE ${CMAKE_DL_LIBS})

if(HWC_STATIC_PACKAGE)
    set(HWC_RUNTIME_LIBRARY_TYPE STATIC)
else()
    set(HWC_RUNTIME_LIBRARY_TYPE SHARED)
endif()

add_library(vir_runtime ${HWC_RUNTIME_LIBRARY_TYPE}
    ${VIR_ABI_SOURCES})
target_include_directories(vir_runtime PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/vir/include
    ${CMAKE_CURRENT_SOURCE_DIR}/vir/runtime/abi/include
    ${CMAKE_CURRENT_SOURCE_DIR}/vir)
target_include_directories(vir_runtime PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(vir_runtime PUBLIC vir_ir)

# Transport implementations are emulation-platform code. The VIR runtime
# exposes only runtime/transport/transport.h and does not depend on a concrete
# CPU or FPGA transport.
add_library(emul_transport STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../emulation/transport/cpu/CpuTransport.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../emulation/transport/fpga/FpgaTransport.cpp)
target_include_directories(emul_transport PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/vir)
target_link_libraries(emul_transport PUBLIC vir_runtime)

add_library(eirc_core
    frontend/lib/frontend.cpp
    frontend/lib/elaborated_design.cpp
    frontend/lib/semantic_tree.cpp
    ${FRONTEND_SVA_SOURCES}
    ${EIR_SOURCES}
    target/lib/cpu/cpu_target.cpp
    target/lib/emul/emul_target.cpp
    target/lib/emul/disassembler.cpp)
target_include_directories(eirc_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/eir/include
    ${CMAKE_CURRENT_SOURCE_DIR}/frontend/include
    ${CMAKE_CURRENT_SOURCE_DIR}/target/include)
target_link_libraries(eirc_core PUBLIC slang_prebuilt fmt_prebuilt)
target_link_libraries(eirc_core PRIVATE json-c)
target_link_libraries(vir_ir PRIVATE eirc_core)

find_path(Z3_INCLUDE_DIR z3++.h REQUIRED)
find_library(Z3_LIBRARY z3 REQUIRED)

set(SPOT_ROOT "${HWC_EXTERNAL_ROOT}/spot" CACHE PATH
    "Spot LTL and omega-automata installation")
find_path(SPOT_INCLUDE_DIR spot/twaalgos/translate.hh
    PATHS "${SPOT_ROOT}/include" REQUIRED NO_DEFAULT_PATH)
find_library(SPOT_LIBRARY spot
    PATHS "${SPOT_ROOT}/lib" REQUIRED NO_DEFAULT_PATH)
find_library(BDDX_LIBRARY bddx
    PATHS "${SPOT_ROOT}/lib" REQUIRED NO_DEFAULT_PATH)

set(PONO_EXECUTABLE
    "${HWC_EXTERNAL_ROOT}/pono/build/pono"
    CACHE FILEPATH "Pono model checker executable")
set(BTORMC_EXECUTABLE
    "${HWC_EXTERNAL_ROOT}/boolector/build/bin/btormc"
    CACHE FILEPATH "BtorMC model checker executable")
set(RIC3_EXECUTABLE
    "${HWC_EXTERNAL_ROOT}/rIC3/target/release/ric3"
    CACHE FILEPATH "rIC3 model checker executable")
set(AVR_EXECUTABLE
    "${HWC_EXTERNAL_ROOT}/avr/avr"
    CACHE FILEPATH "AVR model checker executable")
set(ABC_EXECUTABLE "${HWC_EXTERNAL_ROOT}/abc/abc"
    CACHE FILEPATH "ABC executable")
set(AVY_EXECUTABLE "${HWC_EXTERNAL_ROOT}/avy/build/avy/src/avy"
    CACHE FILEPATH "Avy executable")
set(SUPROVE_EXECUTABLE "${HWC_EXTERNAL_ROOT}/super-prove-install/bin/super_prove.sh"
    CACHE FILEPATH "SuProve executable")
set(BTOR2AIGER_EXECUTABLE "${HWC_EXTERNAL_ROOT}/boolector/deps/btor2tools/build-aiger/bin/btor2aiger"
    CACHE FILEPATH "BTOR2 to AIGER converter")
set(YOSYS_EXECUTABLE "/usr/bin/yosys" CACHE FILEPATH "Yosys executable")
set(SMTBMC_EXECUTABLE "/usr/bin/yosys-smtbmc" CACHE FILEPATH "Yosys SMTBMC executable")
file(GLOB HWC_SOURCES CONFIGURE_DEPENDS tools/src/*.cpp)
add_executable(hwc tools/main.cpp ${HWC_SOURCES} ${FORMAL_SOURCES})
set_target_properties(hwc PROPERTIES INSTALL_RPATH
    "$ORIGIN/../lib;${SPOT_ROOT}/lib")
target_include_directories(hwc PRIVATE
    tools/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../__BIN__
    ${SPOT_INCLUDE_DIR})
target_link_libraries(hwc PRIVATE
    btor2 eirc_core vir_ir vir_runtime emul_transport ${Z3_LIBRARY} ${SPOT_LIBRARY}
    ${BDDX_LIBRARY}
    ${CMAKE_DL_LIBS})
if(HWC_STATIC_PACKAGE)
    target_link_options(hwc PRIVATE -static)
endif()
target_compile_definitions(hwc PRIVATE
    HWC_DEFAULT_PONO="${PONO_EXECUTABLE}"
    HWC_DEFAULT_BTORMC="${BTORMC_EXECUTABLE}"
    HWC_DEFAULT_RIC3="${RIC3_EXECUTABLE}"
    HWC_DEFAULT_AVR="${AVR_EXECUTABLE}"
    HWC_DEFAULT_ABC="${ABC_EXECUTABLE}"
    HWC_DEFAULT_AVY="${AVY_EXECUTABLE}"
    HWC_DEFAULT_SUPROVE="${SUPROVE_EXECUTABLE}"
    HWC_DEFAULT_BTOR2AIGER="${BTOR2AIGER_EXECUTABLE}"
    HWC_DEFAULT_YOSYS="${YOSYS_EXECUTABLE}"
    HWC_DEFAULT_SMTBMC="${SMTBMC_EXECUTABLE}"
    HWC_RUNTIME_INCLUDE="${CMAKE_CURRENT_SOURCE_DIR}/vir/runtime/include"
    HWC_UVM_INCLUDE="${HWC_EXTERNAL_ROOT}/uvm/src"
    HWC_UVM_SOURCE="${HWC_EXTERNAL_ROOT}/uvm/src/uvm_pkg.sv")

install(TARGETS hwc vir_runtime
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib)

add_executable(test-target-description ../tests/complier/eir/target_description.cpp)
target_link_libraries(test-target-description PRIVATE eirc_core)
add_executable(test-mir-lowering ../tests/complier/eir/mir_lowering.cpp)
target_link_libraries(test-mir-lowering PRIVATE eirc_core)
add_executable(test-ir-core ../tests/complier/eir/ir_core.cpp)
target_link_libraries(test-ir-core PRIVATE eirc_core)
add_executable(test-packed-struct-type
    ../tests/complier/eir/packed_struct_type.cpp)
target_include_directories(test-packed-struct-type PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/eir/lib)
target_link_libraries(test-packed-struct-type PRIVATE eirc_core)
add_executable(test-btor2-lowering ../tests/complier/btor2/lowering.cpp)
target_link_libraries(test-btor2-lowering PRIVATE btor2 eirc_core)
add_executable(test-btor2-routing
    ../tests/complier/btor2/routing.cpp
    formal/router/decision.cpp)
target_include_directories(test-btor2-routing PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test-btor2-routing PRIVATE btor2)
add_executable(test-btor2-optimization
    ../tests/complier/btor2/optimization.cpp
    btor2/lib/Transform/HIR/ComponentDeduplication.cpp
    btor2/lib/Transform/HIR/PassPipeline.cpp
    btor2/lib/Transform/HIR/PropertySimplification.cpp)
target_include_directories(test-btor2-optimization PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/btor2/include)
target_link_libraries(test-btor2-optimization PRIVATE btor2)
add_executable(test-vir-new-intrinsics ../tests/complier/vir_new/intrinsics.cpp)
target_link_libraries(test-vir-new-intrinsics PRIVATE vir_runtime)
add_executable(test-vir-new-builtins ../tests/complier/vir_new/builtins.cpp)
target_link_libraries(test-vir-new-builtins PRIVATE vir_runtime)
add_executable(test-vir-new-ir ../tests/complier/vir_new/ir.cpp)
target_link_libraries(test-vir-new-ir PRIVATE vir_ir)
add_executable(test-vir-new-runtime-abi
    ../tests/complier/vir_new/runtime_abi.cpp)
target_link_libraries(test-vir-new-runtime-abi PRIVATE vir_runtime)
add_executable(test-formal-ltl-spot
    ../tests/complier/formal/ltl_spot.cpp
    btor2/lib/Lowering/Formal/LTL/spot_automaton.cpp
    btor2/lib/Lowering/Formal/LTL/sequence_formula.cpp)
target_include_directories(test-formal-ltl-spot PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/btor2/include
    ${SPOT_INCLUDE_DIR})
target_link_libraries(test-formal-ltl-spot PRIVATE
    ${SPOT_LIBRARY} ${BDDX_LIBRARY} eirc_core)
add_executable(test-formal-sampled-history
    ../tests/complier/formal/sampled_history.cpp
    btor2/lib/Lowering/Formal/history.cpp
    btor2/lib/IR/Builder.cpp
    btor2/lib/IR/Printer.cpp
    btor2/lib/IR/Operation.cpp
    btor2/lib/IR/Module.cpp
    btor2/lib/IR/Verifier.cpp
    btor2/lib/IR/Value.cpp)
target_include_directories(test-formal-sampled-history PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/btor2/include)
add_executable(test-formal-logical-implication
    ../tests/complier/formal/logical_implication.cpp
    btor2/lib/Lowering/Formal/lower_expression.cpp
    btor2/lib/Lowering/Formal/history.cpp
    btor2/lib/IR/Builder.cpp
    btor2/lib/IR/Printer.cpp
    btor2/lib/IR/Operation.cpp
    btor2/lib/IR/Module.cpp
    btor2/lib/IR/Verifier.cpp
    btor2/lib/IR/Value.cpp)
target_include_directories(test-formal-logical-implication PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/btor2/include)

enable_testing()
add_test(NAME target_description COMMAND test-target-description)
add_test(NAME btor2_lowering COMMAND test-btor2-lowering)
add_test(NAME btor2_routing COMMAND test-btor2-routing)
add_test(NAME btor2_optimization COMMAND test-btor2-optimization)
add_test(NAME vir_new_intrinsics COMMAND test-vir-new-intrinsics)
add_test(NAME vir_new_builtins COMMAND test-vir-new-builtins)
add_test(NAME vir_new_ir COMMAND test-vir-new-ir)
add_test(NAME vir_new_runtime_abi COMMAND test-vir-new-runtime-abi)
add_test(NAME formal_ltl_spot COMMAND test-formal-ltl-spot)
add_test(NAME formal_sampled_history COMMAND test-formal-sampled-history)
add_test(NAME packed_struct_type COMMAND test-packed-struct-type)
add_test(NAME formal_logical_implication COMMAND test-formal-logical-implication)
add_test(NAME formal_sva_sequence_shapes COMMAND hwc -f
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/complier/frontend/run_sva_shapes.tcl)
set(FORMAL_CACHE_REGRESSIONS
    ack_match exclusive_owner fair latest_write muexc reachability
    scoreboard stability transient_state writable_state)
foreach(regression IN LISTS FORMAL_CACHE_REGRESSIONS)
    add_test(NAME formal_regression_cache_${regression} COMMAND hwc -f
        ${CMAKE_CURRENT_SOURCE_DIR}/../tests/formal_regression_cache/scripts/run_${regression}.tcl)
    set_tests_properties(formal_regression_cache_${regression} PROPERTIES
        LABELS "formal;cache"
        TIMEOUT 120)
endforeach()
set(FORMAL_CACHE_KNOWN_FAILURES request_progress type_fair)
foreach(regression IN LISTS FORMAL_CACHE_KNOWN_FAILURES)
    add_test(NAME formal_regression_cache_${regression} COMMAND hwc -f
        ${CMAKE_CURRENT_SOURCE_DIR}/../tests/formal_regression_cache/scripts/run_${regression}.tcl)
    set_tests_properties(formal_regression_cache_${regression} PROPERTIES
        LABELS "formal;cache;expected-failure"
        WILL_FAIL TRUE
        TIMEOUT 120)
endforeach()
function(add_cache_fault_regression name source fault top property depth)
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/../tests/formal_regression_cache/scripts/files_${source}.f" file_list)
    string(REPLACE "../rtl/" "${CMAKE_CURRENT_SOURCE_DIR}/../tests/formal_regression_cache/rtl/" file_list "${file_list}")
    string(REPLACE "../env" "${CMAKE_CURRENT_SOURCE_DIR}/../tests/formal_regression_cache/env" file_list "${file_list}")
    string(REPLACE "../properties" "${CMAKE_CURRENT_SOURCE_DIR}/../tests/formal_regression_cache/properties" file_list "${file_list}")
    file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/formal_cache_fault_${name}.f"
        CONTENT "+define+${fault}\n${file_list}")
    file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/formal_cache_fault_${name}.tcl"
        CONTENT "read_file -f formal_cache_fault_${name}.f\nset_top ${top}\ncreate_clock i_clk\nset_reset i_rst_n -active_low\nset_engine bmc\nset_max_depth ${depth}\nset_expected_result counterexample\nprove ${property}\n")
    add_test(NAME formal_regression_cache_fault_${name} COMMAND hwc -f
        ${CMAKE_CURRENT_BINARY_DIR}/formal_cache_fault_${name}.tcl)
    set_tests_properties(formal_regression_cache_fault_${name} PROPERTIES
        LABELS "formal;cache;counterexample"
        TIMEOUT 120)
endfunction()
add_cache_fault_regression(muexc muexc KILL_MUEXC_CACHE_REQ_ACCEPT directory_muexc muexc_cache_req_accepted 8)
add_cache_fault_regression(scoreboard scoreboard KILL_SCOREBOARD_SAME_LINE directory_scoreboard unique_outstanding_line_reservation 8)
add_cache_fault_regression(exclusive_owner exclusive_owner KILL_EXCLUSIVE_OWNER directory_exclusive_owner directory_exclusive_owner_invariant 16)
add_cache_fault_regression(exclusive_owner_restore exclusive_owner KILL_EXCLUSIVE_OWNER_RESTORE_SHARERS directory_exclusive_owner directory_exclusive_owner_invariant 16)
add_cache_fault_regression(ack_match ack_match KILL_SNOOP_ACK_MATCH directory_ack_match only_matching_snoop_ack_completes 8)
add_cache_fault_regression(writable_state_allow_shared writable_state KILL_WRITABLE_STATE_ALLOW_SHARED cache_writable_state write_requires_writable_state 24)
add_cache_fault_regression(stability_drop_valid stability KILL_STALLED_OUTPUT_DROP_VALID directory_stability stalled_response_stable 8)
add_cache_fault_regression(writable_state writable_state KILL_WRITABLE_STATE_CHECK cache_writable_state write_requires_writable_state 24)
add_cache_fault_regression(transient_state transient_state KILL_TRANSIENT_STATE_STABILITY cache_transient_state transient_state_stable_until_completion 8)
add_cache_fault_regression(latest_write latest_write KILL_LATEST_COHERENT_WRITE cache_latest_write completed_read_observes_latest_write 8)
add_cache_fault_regression(latest_write_stale latest_write KILL_LATEST_COHERENT_WRITE_STALE cache_latest_write completed_read_observes_latest_write 8)
function(add_channel_fault_regression name model top property engine depth)
    file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/formal_channel_fault_${name}.f"
        CONTENT "+incdir+${CMAKE_CURRENT_SOURCE_DIR}/../tests/formal_regression_channels/env\n${CMAKE_CURRENT_SOURCE_DIR}/../tests/formal_regression_channels/generated/${model}\n${CMAKE_CURRENT_SOURCE_DIR}/../tests/formal_regression_channels/env/${top}_formal.sv\n")
    file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/formal_channel_fault_${name}.tcl"
        CONTENT "read_file -f formal_channel_fault_${name}.f\nset_top ${top}_formal\ncreate_clock i_clk\nset_reset i_rst_n -active_low\nset_engine ${engine}\nset_max_depth ${depth}\nset_expected_result counterexample\nprove ${property}\n")
    add_test(NAME formal_regression_channels_fault_${name} COMMAND hwc -f
        ${CMAKE_CURRENT_BINARY_DIR}/formal_channel_fault_${name}.tcl)
    set_tests_properties(formal_regression_channels_fault_${name} PROPERTIES
        LABELS "formal;channels;counterexample"
        TIMEOUT 120)
endfunction()
add_channel_fault_regression(case_i_duplicate case_i_duplicate.sv.anvil.sv channel_case_i no_duplicate_outstanding_input bmc 24)
add_channel_fault_regression(case_i_loss case_i_loss.sv.anvil.sv channel_case_i accepted_item_eventually_leaves pdr 80)
add_channel_fault_regression(case_ii_capacity case_ii_capacity.sv.anvil.sv channel_case_ii occupancy_within_boundary_capacity bmc 24)
add_channel_fault_regression(case_iv_order case_iv_order.sv.anvil.sv channel_case_iv fifo_order_preserved bmc 24)
add_test(NAME formal_regression_channels_case_i COMMAND hwc -f
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/formal_regression_channels/run_formal.tcl)
set_tests_properties(formal_regression_channels_case_i PROPERTIES
    LABELS "formal;channels;expected-failure"
    WILL_FAIL TRUE
    TIMEOUT 120)
add_test(NAME formal_request_grant COMMAND hwc -f
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/formal_request_grant/run.tcl)
set_tests_properties(formal_request_grant PROPERTIES
    LABELS "formal;request-grant"
    PASS_REGULAR_EXPRESSION "schedule: safety and liveness partitions in parallel"
    TIMEOUT 120)

add_test(NAME formal_request_grant_fault COMMAND hwc -f
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/formal_request_grant/run_fault.tcl)
set_tests_properties(formal_request_grant_fault PROPERTIES
    LABELS "formal;request-grant;counterexample"
    PASS_REGULAR_EXPRESSION "result: FAILED"
    TIMEOUT 120)
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/hierarchical_value_target.f" CONTENT
    "${CMAKE_CURRENT_SOURCE_DIR}/../tests/complier/eir/hierarchical_value_target.sv\n")
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/hierarchical_value_target.tcl" CONTENT
    "read_file -f ${CMAKE_CURRENT_BINARY_DIR}/hierarchical_value_target.f\nset_top hierarchical_value_target\ncreate_clock clk\nset_engine bmc\nset_max_depth 2\nprove hierarchical_assignment_preserved\n")
add_test(NAME hierarchical_value_target COMMAND hwc -f
    ${CMAKE_CURRENT_BINARY_DIR}/hierarchical_value_target.tcl)
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/hwc_vir.tcl" CONTENT
    "read_file -f hwc_sim.f\nset_top adder_tb\nwrite_artifacts artifacts\n")
add_test(NAME vir_new_adder_lowering COMMAND hwc -f
    ${CMAKE_CURRENT_BINARY_DIR}/hwc_vir.tcl)
find_program(LLVM_AS_EXECUTABLE llvm-as REQUIRED)
add_test(NAME vir_new_adder_llvm COMMAND ${LLVM_AS_EXECUTABLE}
    ${CMAKE_CURRENT_BINARY_DIR}/artifacts/program.vir -o ${CMAKE_CURRENT_BINARY_DIR}/adder.bc)
set_tests_properties(vir_new_adder_llvm PROPERTIES DEPENDS vir_new_adder_lowering)
find_program(LLVM_LLC_EXECUTABLE llc REQUIRED)
find_program(LLVM_OPT_EXECUTABLE opt REQUIRED)
add_test(NAME vir_new_adder_verify COMMAND ${LLVM_OPT_EXECUTABLE}
    -passes=verify ${CMAKE_CURRENT_BINARY_DIR}/adder.bc
    -o ${CMAKE_CURRENT_BINARY_DIR}/adder.verified.bc)
set_tests_properties(vir_new_adder_verify PROPERTIES DEPENDS vir_new_adder_llvm)
add_test(NAME vir_new_adder_object COMMAND ${LLVM_LLC_EXECUTABLE}
    ${CMAKE_CURRENT_BINARY_DIR}/adder.verified.bc -filetype=obj
    -o ${CMAKE_CURRENT_BINARY_DIR}/adder.o)
set_tests_properties(vir_new_adder_object PROPERTIES DEPENDS vir_new_adder_verify)
add_test(NAME mir_lowering COMMAND test-mir-lowering)
add_test(NAME ir_core COMMAND test-ir-core)
add_test(NAME hwc_help COMMAND hwc -help)

file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/hwc_sim.f" CONTENT
    "+incdir+../compiler/vir/runtime/include\n../tests/adder/rtl/adder.sv\n../tests/adder/tb/adder_tb.sv\n")
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/hwc_sim.tcl" CONTENT
    "read_file -f hwc_sim.f\nset_top adder_tb\nsimulate\n")
add_test(NAME hwc_simulate COMMAND hwc -f
    ${CMAKE_CURRENT_BINARY_DIR}/hwc_sim.tcl)
set_tests_properties(hwc_simulate PROPERTIES PASS_REGULAR_EXPRESSION "PASS")

file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/hwc_emul.f" CONTENT
    "../emulation/rtl/common/circular_buffer.sv\n../tests/circular_buffer/tb/circular_buffer_auto_tb.sv\n")
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/hwc_emul.tcl" CONTENT
    "read_file -f hwc_emul.f\nset_top circular_buffer_auto_tb\nset_target cpu\nemulate\n")
add_test(NAME hwc_emulate COMMAND hwc -f
    ${CMAKE_CURRENT_BINARY_DIR}/hwc_emul.tcl)
set_tests_properties(hwc_emulate PROPERTIES PASS_REGULAR_EXPRESSION "PASS")

file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/hwc_formal.f" CONTENT
    "../tests/formal_adder/rtl/adder.sv\n../tests/formal_adder/properties/adder_properties.sv\n")
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/hwc_formal.tcl" CONTENT
    "read_file -f hwc_formal.f\nset_top formal_adder_properties\nset_engine smt\nprove adder_correct\n")
add_test(NAME hwc_formal COMMAND hwc -f
    ${CMAKE_CURRENT_BINARY_DIR}/hwc_formal.tcl)
set_tests_properties(hwc_formal PROPERTIES PASS_REGULAR_EXPRESSION
    "PASS adder_correct")

file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/hwc_uvm.f" CONTENT
    "+incdir+../tests/uvm_adder/tb\n../tests/uvm_adder/rtl/adder.sv\n../tests/uvm_adder/tb/adder_uvm_pkg.sv\n../tests/uvm_adder/tb/adder_uvm_tb.sv\n")
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/hwc_uvm.tcl" CONTENT
    "read_file -f hwc_uvm.f\nset_top adder_uvm_tb\nset_uvm_test adder_test\nset_seed 100\nsimulate\n")
add_test(NAME hwc_uvm COMMAND hwc -f
    ${CMAKE_CURRENT_BINARY_DIR}/hwc_uvm.tcl)
set_tests_properties(hwc_uvm PROPERTIES PASS_REGULAR_EXPRESSION "PASS")

add_test(NAME hwc_uart_uvm COMMAND hwc -f
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/uart_uvm/run_uvm.tcl)
set_tests_properties(hwc_uart_uvm PROPERTIES
    PASS_REGULAR_EXPRESSION "PASS"
    TIMEOUT 60)
