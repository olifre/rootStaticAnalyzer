
list(APPEND ALLTESTS
	testConstructionDestruction.cpp
	testIsA.cpp
	testDataObjBases.cpp
	)

include_directories(${PROJECT_SOURCE_DIR}/src/include)
include_directories(include)

add_library(rootStaticAnalyzerTests SHARED ${ALLTESTS})
install(TARGETS rootStaticAnalyzerTests DESTINATION lib)
