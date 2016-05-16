add_subdirectory(tests)

add_executable(rootStaticAnalyzer rootStaticAnalyzer.cpp utilityFunctions.cpp errorHandling.cpp)

include_directories(include)
target_link_libraries(rootStaticAnalyzer ${ROOT_LIBS} ${OptionParser_LIBRARIES} rootStaticAnalyzerTests)

install(TARGETS rootStaticAnalyzer DESTINATION bin)
