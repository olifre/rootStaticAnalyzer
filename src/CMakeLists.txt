add_executable(rootStaticAnalyzer rootStaticAnalyzer.cpp)

target_link_libraries(rootStaticAnalyzer ${ROOT_LIBS} ${OPTIONPARSER_LIBRARIES})
 
install(TARGETS rootStaticAnalyzer DESTINATION bin)
