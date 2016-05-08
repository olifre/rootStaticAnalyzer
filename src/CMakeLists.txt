add_executable(rootStaticAnalyzer rootStaticAnalyzer.cpp)

target_link_libraries(rootStaticAnalyzer ${ROOT_LIBS} ${OptionParser_LIBRARIES})
 
install(TARGETS rootStaticAnalyzer DESTINATION bin)
