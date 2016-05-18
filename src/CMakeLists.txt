add_subdirectory(tests)

add_executable(rootStaticAnalyzer classObject.cpp rootStaticAnalyzer.cpp utilityFunctions.cpp streamingUtils.cpp errorHandling.cpp)

include_directories(include)
include_directories(tests/include)
target_link_libraries(rootStaticAnalyzer ${ROOT_LIBS} ${OptionParser_LIBRARIES} rootStaticAnalyzerTests)

install(TARGETS rootStaticAnalyzer DESTINATION bin)
