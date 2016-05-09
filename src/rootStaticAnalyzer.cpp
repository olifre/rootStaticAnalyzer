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

#include "Options.h"
#include "utilityFunctions.h"
#include "errorHandling.h"



int main(int argc, char** argv) {
	OptionParser parser("Simple static analyzer for ROOT and ROOT-based projects");
	OptionContainer<std::string> rootMapPatterns('r', "rootMapPattern", "Regexp to match rootmaps to test with, can be given multiple times. '.*' matches all, no patterns given => test ROOT only.");
	OptionContainer<std::string> classNamePatterns('c', "classNamePattern", "Regexp to match class-names to test, can be given multiple times. '.*' (or no pattern given) tests all.");
	OptionContainer<std::string> classNameAntiPatterns('C', "classNameAntiPattern", "Regexp to match class-names NOT to test, can be given multiple times. Applied after a class has matched the classNamePattern.");
	Option<bool> debug('d', "debug", "Make a lot of debug-noise to debug this program itself.", false);
	
	// We need a TApplication-instance to allow for rootmap-checks - at least for ROOT 5. 
	gROOT->SetBatch(kTRUE);
	TApplication app("app", nullptr, nullptr);

	//gSystem->ResetSignal(kSigSegmentationViolation, true);
	
	auto unusedOptions = parser.fParse(argc, argv);

	if (rootMapPatterns.empty()) {
		/* Test ROOT only.
		   First, determine path where ROOT is. 
		*/
		TString rootLibDir(utilityFunctions::getRootLibDir());
		rootMapPatterns.emplace_back(TString(rootLibDir+"/.*\\.rootmap").Data());
	}

	std::vector<TPRegexp> rootMapRegexps;
	for (auto& pattern : rootMapPatterns) {
		rootMapRegexps.emplace_back(pattern);
	}

	// Construct list of all classes to test.
	std::set<std::string> allClasses;
	{
		TObjArray* rootMaps = gInterpreter->GetRootMapFiles();
		TIter next(rootMaps);
		TNamed* rootMapName;
		while ((rootMapName = dynamic_cast<TNamed*>(next())) != nullptr) {
			TString path = rootMapName->GetTitle();
			if (debug) {
				std::cout << "Checking rootmap-path " << path.Data() << std::endl;
			}
			for (auto& pattern : rootMapRegexps) {
				if (pattern.MatchB(path)) {
					utilityFunctions::parseRootmap(path.Data(), allClasses);
					if (debug) {
						std::cout << "Path matched by regex '" << pattern.GetPattern() << "'." << std::endl;
					}
					break;
				}
			}
		}
	}

	// Filter by classname-patterns.
	{
		if (!classNamePatterns.empty()) {
			// Remove all which do not match any regexp in classNamePattern.
			std::vector<TPRegexp> classNameRegexps;
			for (auto& pattern : classNamePatterns) {
				classNameRegexps.emplace_back(pattern);
			}
			auto allClassesSav = allClasses;
			for (auto& clsName : allClassesSav) {
				bool oneMatched = false;
				for (auto& regex : classNameRegexps) {
					if (regex.MatchB(clsName)) {
						if (debug) {
							std::cout << "Class '" << clsName << "' kept since it matched pattern '" << regex.GetPattern().Data() << "'." << std::endl;
						}
						oneMatched = true;
						break;
					}
				}
				if (!oneMatched) {
					if (debug) {
						std::cout << "Class '" << clsName << "' removed since it did not match any classname-pattern." << std::endl;
					}
					allClasses.erase(clsName);
				}
			}
		}
		if (!classNameAntiPatterns.empty()) {
			// Remove all which match any regexp in classNameAntiPattern.
			std::vector<TPRegexp> classNameRegexps;
			for (auto& pattern : classNameAntiPatterns) {
				classNameRegexps.emplace_back(pattern);
			}
			auto allClassesSav = allClasses;
			for (auto& clsName : allClassesSav) {
				for (auto& regex : classNameRegexps) {
					if (regex.MatchB(clsName)) {
						if (debug) {
							std::cout << "Class '" << clsName << "' kept since it matched anti-pattern '" << regex.GetPattern().Data() << "'." << std::endl;
						}
						allClasses.erase(clsName);
						break;
					}
				}
			}
		}
	}

	if (debug) {
		std::cout << "List of considered classes:" << std::endl;
		for (auto& clsName : allClasses) {
			std::cout << "- " << clsName << std::endl;
		}
	}
	
	TBufferFile buf(TBuffer::kWrite, 10000);

	for (auto& clsName : allClasses) {
		/*
		if (clsName[0]!='T') {
			// Not a ROOT class, skip. 
			continue;
		}
		*/
		//std::cout << "LOADING class " << clsName << std::endl;
		auto cls = TClass::GetClass(clsName.c_str(), kTRUE);
		if (cls == nullptr) {
			// Skip that.
			continue;
		}
		// Only test TObject-inheriting classes.
		if (!cls->InheritsFrom(TObject::Class())) {
			continue;
		}
		if (cls->GetClassVersion() <= 0) {
			continue;
		}
		if (cls->GetNew() == nullptr) {
			// Abstract class, skip.
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
		}	ENDTRY;
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
		
		//std::cout << cls->GetName() << std::endl;

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
			digest_1a = utilityFunctions::streamObjectToBufferAndChecksum(buf, obj);
		} CATCH ( excode ) {
			errorHandling::throwError(cls->GetDeclFileName(), 0,
			                          TString::Format("Streaming of class '%s' failed fatally, needs manual investigation! Check the stacktrace!",
			                                          cls->GetName()));
			streamingWorked = false;
			Throw( excode );
		}	ENDTRY;
		decltype(utilityFunctions::getRealDataDigests(obj)) digests_1a;
		if (streamingWorked) {
			digests_1a = utilityFunctions::getRealDataDigests(obj);
		}
		TRY {
			cls->Destructor(obj, kTRUE);
		} CATCH ( excode ) {
			Throw( excode );
		}	ENDTRY;
		if (!streamingWorked) {
			continue;
		}

		std::fill(&storageArena[0], &storageArena[uintCount], uninitializedUint_1);
		obj = static_cast<TObject*>(cls->New(storageArena));
		TString digest_1b;
		TRY {
			digest_1b = utilityFunctions::streamObjectToBufferAndChecksum(buf, obj);
		} CATCH ( excode ) {
			Throw( excode );
		}	ENDTRY;
		TRY {
			cls->Destructor(obj, kTRUE);
		} CATCH ( excode ) {
			Throw( excode );
		}	ENDTRY;

		std::fill(&storageArena[0], &storageArena[uintCount], uninitializedUint_2);
		obj = static_cast<TObject*>(cls->New(storageArena));
		TString digest_2;
		TRY {
			digest_2 = utilityFunctions::streamObjectToBufferAndChecksum(buf, obj);
		} CATCH ( excode ) {
			Throw( excode );
		}	ENDTRY;
		auto digests_2 = utilityFunctions::getRealDataDigests(obj);
		TRY {
			cls->Destructor(obj, kTRUE);
		} CATCH ( excode ) {
			Throw( excode );
		}	ENDTRY;

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
