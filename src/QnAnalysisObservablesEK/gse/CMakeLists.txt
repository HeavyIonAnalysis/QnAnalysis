

find_package(ROOT REQUIRED COMPONENTS RIO)
ROOT_GENERATE_DICTIONARY(
        G__GraphSysErr
        ${CMAKE_CURRENT_SOURCE_DIR}/GraphSysErr.h
        LINKDEF GraphSysErrLinkDef.h
        OPTIONS -inlineInputHeader
)
add_library(gse SHARED GraphSysErr.cpp G__GraphSysErr.cxx)
target_compile_options(gse PRIVATE -W)
target_link_libraries(gse PUBLIC ROOT::RIO)
target_include_directories(gse PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
