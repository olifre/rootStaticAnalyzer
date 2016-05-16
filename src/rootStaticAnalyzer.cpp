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
#include <TBufferFile.h>
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
			allClassObjects.emplace_back(cls);
		}
	}

	auto &allTests = testInterface::fGetAllTests();
	std::size_t executedTests = 0;
	do {
		executedTests = 0;
		for (auto& test : allTests) {
			auto testsRun = test.second->fRunTestOnClasses(allClassObjects);
			std::cout << test.first << ": " << testsRun << std::endl;
			executedTests += testsRun;
		}
	} while (executedTests > 0);
	
	// Set of TObject-inheriting classes. 
	std::set<TClass*> allTObjects;
	std::copy_if(allTClasses.begin(), allTClasses.end(), std::inserter(allTObjects, allTObjects.end()), [](TClass* cls){ return cls->InheritsFrom(TObject::Class()); });

	// Set of 
	std::set<TClass*> allDataObjects;
	std::copy_if(allTObjects.begin(), allTObjects.end(), std::inserter(allDataObjects, allDataObjects.end()), [](TClass* cls){ return !(cls->GetClassVersion() <= 0); });

	TBufferFile buf(TBuffer::kWrite, 10000);

	for (auto& cls : allDataObjects) {
		if (cls->GetNew() == nullptr) {
			// Abstract class, skip.
			continue;
		}
		if (cls->GetDestructor() == nullptr) {
			// Class with protected destructor, skip.
			continue;
		}

		//HACK
		if (TString(cls->GetName()).BeginsWith("TEve")
		        || TString(cls->GetName()).BeginsWith("TGL")
		        || strcmp(cls->GetName(), "TGraphStruct") == 0
		        || strcmp(cls->GetName(), "TMaterial") == 0
		        || strcmp(cls->GetName(), "TMixture") == 0
		        || strcmp(cls->GetName(), "TNode") == 0
		        || strcmp(cls->GetName(), "TNodeDiv") == 0
		        || strcmp(cls->GetName(), "TParallelCoord") == 0
		        || strcmp(cls->GetName(), "TParallelCoordVar") == 0
		        || strcmp(cls->GetName(), "TQueryDescription") == 0
		        || strcmp(cls->GetName(), "TRotMatrix") == 0
		        || strcmp(cls->GetName(), "TSessionDescription") == 0
		        || strcmp(cls->GetName(), "TMinuit2TraceObject") == 0
		        || cls->InheritsFrom("TShape")
		   ) {
			continue;
		}
		//HACK

		UInt_t classSize = cls->Size();
		UInt_t uintCount = classSize / sizeof(UInt_t) + 1;
		std::vector<UInt_t> storageArenaVector(uintCount);
		auto storageArena = storageArenaVector.data();

		// Test default construction / destruction.
		Bool_t constructionDestructionFailed = false;
		//std::cout << "BEGIN Constructor/destructor test for " << cls->GetName() << std::endl;
		TRY {
			TObject* obj = static_cast<TObject*>(cls->New(storageArena));
			cls->Destructor(obj, kTRUE);
		} CATCH ( excode ) {
			std::cerr << "Something bad happened... " << excode << std::endl;
			constructionDestructionFailed = true;
			Throw( excode );
		}
		ENDTRY;
		//std::cout << "END Constructor/destructor test for " << cls->GetName() << std::endl;

		if (constructionDestructionFailed) {
			continue;
		}

		// Check IsA().
		{
			TObject* obj = static_cast<TObject*>(cls->New(storageArena));
			bool IsAworked = true;
			if (obj->IsA() == nullptr) {
				errorHandling::throwError(cls->GetDeclFileName(), 0,
				                          TString::Format("IsA() of TObject-inheriting class '%s' return nullptr, this is bad!", cls->GetName()));
				IsAworked = false;
			}
			cls->Destructor(obj, kTRUE);
			if (!IsAworked) {
				continue;
			}
		}

		//HACK
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
		//HACK

		TList* baseClasses = cls->GetListOfBases();
		TIter nextBase(baseClasses);
		TBaseClass* baseCl = nullptr;
		while ((baseCl = dynamic_cast<TBaseClass*>(nextBase())) != nullptr) {
			TClass* baseClPtr = baseCl->GetClassPointer();
			if (baseClPtr->GetClassVersion() <= 0) {
				auto realData = baseClPtr->GetListOfRealData();
				if (realData != nullptr && realData->GetEntries() > 0) {
					TString unstreamedData;
					TIter nextRD(realData);
					TRealData* rd = nullptr;
					while ((rd = dynamic_cast<TRealData*>(nextRD())) != nullptr) {
						if (rd->TestBit(TRealData::kTransient)) {
							// Skip transient members.
							continue;
						}
						if (unstreamedData.Length() > 0) {
							unstreamedData += ",";
						}
						unstreamedData += rd->GetName();
					}
					if (unstreamedData.Length() > 0) {
						std::cerr << "ERROR: " << "Class " << cls->GetName() << " inherits from " << baseClPtr->GetName() << " which has class version " << baseClPtr->GetClassVersion() << ", members: " << unstreamedData.Data() << " will not be streamed!" << std::endl;
					}
				}
			}
		}

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
			errorHandling::throwError(cls->GetDeclFileName(), 0,
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

		/* We only test whether uninitialized memory is picked up.
		   In that case, the digest_1x will agree, but disagree with digest_2 which used the differently
		   initialized arena. */
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
					errorHandling::throwError(cls->GetDeclFileName(), &searchExpr,
					                          TString::Format("error: Streamed member '%s%s' of dataobject '%s' not initialized by constructor!",
					                                  (memberDataType != nullptr) ? TString::Format("%s ", memberDataType->GetName()).Data() : "",
					                                  memberName.Data(),
					                                  cls->GetName()));
					foundAmemberToBlame = kTRUE;
				}

			}
			if (!foundAmemberToBlame) {
				errorHandling::throwError(cls->GetDeclFileName(), 0,
				                          TString::Format("error: Dataobject '%s' streams uninitialized memory after default construction, unable to find the member which causes this!", cls->GetName()));
			}
		}
	}

	return 0;
}
