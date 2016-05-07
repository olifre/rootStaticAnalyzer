CXXFLAGS=`root-config --libs --cflags` -ggdb -O2

copytest:	rootStaticAnalyzer.cpp utilityFunctions.h
		g++ $(CXXFLAGS) rootStaticAnalyzer.cpp -o rootStaticAnalyzer
