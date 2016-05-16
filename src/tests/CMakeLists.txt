
list(APPEND ALLTESTS testIsA.cpp)

include_directories(${PROJECT_SOURCE_DIR}/src/include)
include_directories(include)

add_library(rootStaticAnalyzerTests SHARED ${ALLTESTS})
