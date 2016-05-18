/*
  rootStaticAnalyzer - A simple post-compile-time analyzer for ROOT and ROOT-based projects.
  Copyright (C) 2016  Oliver Freyermuth

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <list>
#include <string>
#include <string.h>

#include <TInterpreter.h>
#include <TClassTable.h>
#include <TClass.h>

#include <TROOT.h>
#include <TException.h>
#include <TApplication.h>
#include <TSystem.h>
#include <TList.h>
#include <TBaseClass.h>
#include <TRealData.h>
#include <TDataMember.h>
#include <TDataType.h>
#include <TString.h>
#include <TMD5.h>
#include <TPRegexp.h>

#include <algorithm>

#include "Options.h"

#include "classObject.h"
#include "testInterface.h"
#include "utilityFunctions.h"
#include "errorHandling.h"
#include "streamingUtils.h"

int main(int argc, char** argv) {
	OptionParser parser("Simple static analyzer for ROOT and ROOT-based projects");

	OptionContainer<std::string> rootMapPatterns('r', "rootMapPattern", "Regexp to match rootmaps to test with, can be given multiple times. '.*' matches all, no patterns given => test ROOT only.");
	OptionContainer<std::string> classNamePatterns('c', "classNamePattern", "Regexp to match class-names to test, can be given multiple times. '.*' (or no pattern given) tests all.");
	OptionContainer<std::string> classNameAntiPatterns('C', "classNameAntiPattern", "Regexp to match class-names NOT to test, can be given multiple times. Applied after a class has matched the classNamePattern.");
	Option<bool> dataObjectsOnly('D', "dataObjectsOnly", "Consider only TObject-inheriting classes with Class-version > 0 for all tests.", false);
	Option<bool> debug('d', "debug", "Make a lot of debug-noise to debug this program itself.", false);

	// We need a TApplication-instance to allow for rootmap-checks - at least for ROOT 5.
	gROOT->SetBatch(kTRUE);
	TApplication app("app", nullptr, nullptr);

	auto unusedOptions = parser.fParse(argc, argv);

	if (rootMapPatterns.empty()) {
		/* Test ROOT only. */
		TString rootLibDir(utilityFunctions::getRootLibDir());
		rootMapPatterns.emplace_back(TString(rootLibDir + "/.*\\.rootmap").Data());
	}

	// Get all rootmaps filtered by the patterns.
	std::set<std::string> allClasses{utilityFunctions::getRootmapsByRegexps(rootMapPatterns, debug)};

	// Filter by classname-patterns.
	utilityFunctions::filterSetByPatterns(allClasses, classNamePatterns, classNameAntiPatterns, debug);

	if (debug) {
		std::cout << "List of considered classes:" << std::endl;
		for (auto& clsName : allClasses) {
			std::cout << "- " << clsName << std::endl;
		}
	}

	// Prepare sets of TClasses which can then be used for the various tests.

	// Silent TClass lookup, triggers autoloading / autoparsing.
	std::set<TClass*> allTClasses;
	std::vector<classObject> allClassObjects;
	for (auto& clsName : allClasses) {
		auto cls = TClass::GetClass(clsName.c_str(), kTRUE);
		if (cls != nullptr) {
			allTClasses.insert(cls);
			bool toBeTested = true;
			if (dataObjectsOnly) {
				if (!cls->InheritsFrom(TObject::Class()) || !(cls->GetClassVersion() > 0)) {
					toBeTested = false;
				}
			}
			if (toBeTested) {
				allClassObjects.emplace_back(cls);
			}
		}
	}

	// BEGIN OF UGLY HACKS
	for (auto& cls : allClassObjects) {
		if (TString(cls.fGetClassName()).BeginsWith("TEve")
		        || TString(cls.fGetClassName()).BeginsWith("TG")
		        || TString(cls.fGetClassName()).BeginsWith("TMVA")
		        || TString(cls.fGetClassName()).BeginsWith("TQ")
		        || TString(cls.fGetClassName()).BeginsWith("TRoot")
		        || TString(cls.fGetClassName()).BeginsWith("TProof")
		        || TString(cls.fGetClassName()).BeginsWith("TPyth")
		        || strcmp(cls.fGetClassName().c_str(), "TAFS") == 0
		        || strcmp(cls.fGetClassName().c_str(), "TBrowser") == 0
		        || strcmp(cls.fGetClassName().c_str(), "RooCategory") == 0
		        || strcmp(cls.fGetClassName().c_str(), "TCivetweb") == 0
		        || strcmp(cls.fGetClassName().c_str(), "TKDE") == 0
		        || strcmp(cls.fGetClassName().c_str(), "TDrawFeedback") == 0
		        || strcmp(cls.fGetClassName().c_str(), "TGraphStruct") == 0
		        || strcmp(cls.fGetClassName().c_str(), "TMaterial") == 0
		        || strcmp(cls.fGetClassName().c_str(), "TMixture") == 0
		        || strcmp(cls.fGetClassName().c_str(), "TNode") == 0
		        || strcmp(cls.fGetClassName().c_str(), "TNodeDiv") == 0
		        || strcmp(cls.fGetClassName().c_str(), "TParallelCoord") == 0
		        || strcmp(cls.fGetClassName().c_str(), "TParallelCoordVar") == 0
		        || strcmp(cls.fGetClassName().c_str(), "TQueryDescription") == 0
		        || strcmp(cls.fGetClassName().c_str(), "TRotMatrix") == 0
		        || strcmp(cls.fGetClassName().c_str(), "TSessionDescription") == 0
		        || strcmp(cls.fGetClassName().c_str(), "TMinuit2TraceObject") == 0
		        || cls.fGetTClass()->InheritsFrom("TGedFrame")
		        || cls.fGetTClass()->InheritsFrom("TShape")
		   ) {
			cls.fMarkTested("ConstructionDestruction", false);
		}
	}
	// END OF UGLY HACKS

	auto &allTests = testInterface::fGetAllTests();
	if (debug) {
		std::cout << "Available tests:" << std::endl;
		for (auto& test : allTests) {
			std::cout << "- " << test.first << std::endl;
		}
	}
	std::size_t executedTests = 0;
	do {
		executedTests = 0;
		for (auto& test : allTests) {
			auto testsRun = test.second->fRunTestOnClasses(allClassObjects, debug);
			std::cout << test.first << ": " << testsRun << std::endl;
			executedTests += testsRun;
		}
	} while (executedTests > 0);

	return 0;

}


// BEGIN OF DEAD CODE. THIS NEEDS TO BE CONVERTED TO MODULAR TESTS!



//TBufferFile buf(TBuffer::kWrite, 10000);

/*
if (strcmp(cls->GetName(), "TKDE") == 0
        || strcmp(cls->GetName(), "TCanvas") == 0
        || strcmp(cls->GetName(), "TInspectCanvas") == 0
        || strcmp(cls->GetName(), "TGeoBranchArray") == 0
        || strcmp(cls->GetName(), "TStreamerInfo") == 0
        || strcmp(cls->GetName(), "TTreeRow") == 0
        || strcmp(cls->GetName(), "THelix") == 0
        || strcmp(cls->GetName(), "TClonesArray") == 0
        || strcmp(cls->GetName(), "TBranchObject") == 0) {
	continue;
}
*/

// STREAMING TEST
/*
	UInt_t uninitializedUint_1 = 0xB33FD34D;
	UInt_t uninitializedUint_2 = 0xD34DB33F;

	// Stream it!
	bool streamingWorked = true;
	TObject* obj;
	std::fill(&storageArena[0], &storageArena[uintCount], uninitializedUint_1);
	obj = static_cast<TObject*>(cls->New(storageArena));
	TString digest_1a;
	TRY {
		digest_1a = streamingUtils::streamObjectToBufferAndChecksum(buf, obj);
	} CATCH ( excode ) {
		errorHandling::throwError(cls->GetDeclFileName(), 0, errorHandling::kError,
		                          TString::Format("Streaming of class '%s' failed fatally, needs manual investigation! Check the stacktrace!",
		                                  cls->GetName()));
		streamingWorked = false;
		Throw( excode );
	}
	ENDTRY;
	decltype(streamingUtils::getRealDataDigests(obj)) digests_1a;
	if (streamingWorked) {
		digests_1a = streamingUtils::getRealDataDigests(obj);
	}
	TRY {
		cls->Destructor(obj, kTRUE);
	} CATCH ( excode ) {
		Throw( excode );
	}
	ENDTRY;
	if (!streamingWorked) {
		continue;
	}
	std::fill(&storageArena[0], &storageArena[uintCount], uninitializedUint_1);
	obj = static_cast<TObject*>(cls->New(storageArena));
	TString digest_1b;
	TRY {
		digest_1b = streamingUtils::streamObjectToBufferAndChecksum(buf, obj);
	} CATCH ( excode ) {
		Throw( excode );
	}
	ENDTRY;
	TRY {
		cls->Destructor(obj, kTRUE);
	} CATCH ( excode ) {
		Throw( excode );
	}
	ENDTRY;

	std::fill(&storageArena[0], &storageArena[uintCount], uninitializedUint_2);
	obj = static_cast<TObject*>(cls->New(storageArena));
	TString digest_2;
	TRY {
		digest_2 = streamingUtils::streamObjectToBufferAndChecksum(buf, obj);
	} CATCH ( excode ) {
		Throw( excode );
	}
	ENDTRY;
	auto digests_2 = streamingUtils::getRealDataDigests(obj);
	TRY {
		cls->Destructor(obj, kTRUE);
	} CATCH ( excode ) {
		Throw( excode );
	}
	ENDTRY;
*/
/* We only test whether uninitialized memory is picked up.
   In that case, the digest_1x will agree, but disagree with digest_2 which used the differently
   initialized arena. */
/*
	if ((digest_1a == digest_1b) && (digest_1a != digest_2)) {
		// Blame members!
		Bool_t foundAmemberToBlame = kFALSE;
		for (auto memberCheck : digests_1a) {
			auto& memberName = memberCheck.first;
			auto memberRealData   = memberCheck.second.second;
			auto memberDataMember = memberRealData->GetDataMember();
			auto memberDataType   = memberDataMember->GetDataType();
			if (memberCheck.second.first != digests_2[memberName].first) {
				TPRegexp searchExpr(TString::Format(".*[^_a-zA-Z]%s[^_a-zA-Z0-9].*", memberName.Data()));
				errorHandling::throwError(cls->GetDeclFileName(), searchExpr, errorHandling::kError,
				                          TString::Format("Streamed member '%s%s' of dataobject '%s' not initialized by constructor!",
				                                  (memberDataType != nullptr) ? TString::Format("%s ", memberDataType->GetName()).Data() : "",
				                                  memberName.Data(),
				                                  cls->GetName()));
				foundAmemberToBlame = kTRUE;
			}

		}
		if (!foundAmemberToBlame) {
			errorHandling::throwError(cls->GetDeclFileName(), 0, errorHandling::kError,
			                          TString::Format("Dataobject '%s' streams uninitialized memory after default construction, unable to find the member which causes this!", cls->GetName()));
		}
	}
}
*/
