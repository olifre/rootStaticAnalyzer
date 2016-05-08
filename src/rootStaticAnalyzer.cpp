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

#include <TInterpreter.h>
#include <TClassTable.h>
#include <TClass.h>

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
	auto unusedOptions = parser.fParse(argc, argv);

	
	
	TBufferFile buf(TBuffer::kWrite, 10000);

	gClassTable->Init();
	const char* clsName;
	Int_t clsIndex = 0;
	while ((clsName = gClassTable->At(clsIndex)) != nullptr) {
		clsIndex++;
		if (clsName == nullptr || clsName[0]!='T') {
			// Not a ROOT class, skip. 
			continue;
		}
		//std::cout << "LOADING class " << clsName << std::endl;
		auto cls = TClass::GetClass(clsName, kTRUE);
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

		UInt_t classSize = cls->Size();
		UInt_t uintCount = classSize / sizeof(UInt_t) + 1;
		auto storageArena = new UInt_t[uintCount];

		//std::cout << "BEGIN Constructor/destructor test for " << cls->GetName() << std::endl;
		// Test default construction / destruction. 
		{
			TObject* obj = static_cast<TObject*>(cls->New(storageArena));
			//UInt_t memHash_1 = TString::Hash(&storageArena, sizeof(UInt_t)*uintCount);
			cls->Destructor(obj, kTRUE);
		}
		//std::cout << "END Constructor/destructor test for " << cls->GetName() << std::endl;
		
		//std::cout << cls->GetName() << std::endl;

		//HACK
		if (strcmp(cls->GetName(), "TKDE") == 0
		    || strcmp(cls->GetName(), "TCanvas") == 0
		    || strcmp(cls->GetName(), "TInspectCanvas") == 0
		    || strcmp(cls->GetName(), "TStreamerInfo") == 0
		    || strcmp(cls->GetName(), "TTreeRow") == 0
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
						if (unstreamedData.Length() > 0) {
							unstreamedData += ",";
						}
						unstreamedData += rd->GetName();
					}
					std::cerr << "ERROR: " << "Class " << cls->GetName() << " inherits from " << baseClPtr->GetName() << " which has class version " << baseClPtr->GetClassVersion() << ", members: " << unstreamedData.Data() << " will not be streamed!" << std::endl;
				}
			}
		}
		
		UInt_t uninitializedUint_1 = 0xB33FD34D;
		UInt_t uninitializedUint_2 = 0xD34DB33F;

		// Stream it!
		TObject* obj;
		std::fill(&storageArena[0], &storageArena[uintCount], uninitializedUint_1);
		obj = static_cast<TObject*>(cls->New(storageArena));
		TString digest_1a = utilityFunctions::streamObjectToBufferAndChecksum(buf, obj);
		auto digests_1a = utilityFunctions::getRealDataDigests(obj);
		cls->Destructor(obj, kTRUE);

		std::fill(&storageArena[0], &storageArena[uintCount], uninitializedUint_1);
		obj = static_cast<TObject*>(cls->New(storageArena));
		TString digest_1b = utilityFunctions::streamObjectToBufferAndChecksum(buf, obj);
		cls->Destructor(obj, kTRUE);

		std::fill(&storageArena[0], &storageArena[uintCount], uninitializedUint_2);
		obj = static_cast<TObject*>(cls->New(storageArena));
		TString digest_2 = utilityFunctions::streamObjectToBufferAndChecksum(buf, obj);
		auto digests_2 = utilityFunctions::getRealDataDigests(obj);
		cls->Destructor(obj, kTRUE);

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
