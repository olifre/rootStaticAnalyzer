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

#include "testStreamingUninitialized.h"

#include "errorHandling.h"
#include "streamingUtils.h"

#include <TClass.h>
#include <TDataMember.h>
#include <TDataType.h>
#include <TRealData.h>

static testStreamingUninitialized instance = testStreamingUninitialized();

bool testStreamingUninitialized::fRunTest(classObject& aClass) {
	bool streamsUninitializedContent = false;
	
	auto cls = aClass.fGetTClass();

	UInt_t classSize = cls->Size();
	UInt_t uintCount = classSize / sizeof(UInt_t) + 1;
	std::vector<UInt_t> storageArenaVector(uintCount);
	auto storageArena = storageArenaVector.data();

	static const UInt_t uninitializedUint_1 = 0xB33FD34D;
	static const UInt_t uninitializedUint_2 = 0xD34DB33F;

	TObject* obj;
	
	std::fill(&storageArena[0], &storageArena[uintCount], uninitializedUint_1);
	obj = static_cast<TObject*>(cls->New(storageArena, TClass::kRealNew));
	auto digest_1a  = streamingUtils::streamObjectToBufferAndChecksum(obj);
	auto digests_1a = streamingUtils::getRealDataDigests(obj);
	cls->Destructor(obj, kTRUE);

	std::fill(&storageArena[0], &storageArena[uintCount], uninitializedUint_1);
	obj = static_cast<TObject*>(cls->New(storageArena, TClass::kRealNew));
	auto digest_1b  = streamingUtils::streamObjectToBufferAndChecksum(obj);
	auto digests_1b = streamingUtils::getRealDataDigests(obj);
	cls->Destructor(obj, kTRUE);

	std::fill(&storageArena[0], &storageArena[uintCount], uninitializedUint_2);
	obj = static_cast<TObject*>(cls->New(storageArena, TClass::kRealNew));
	auto digest_2  = streamingUtils::streamObjectToBufferAndChecksum(obj);
	auto digests_2 = streamingUtils::getRealDataDigests(obj);
	cls->Destructor(obj, kTRUE);


/* We only test whether uninitialized memory is picked up.
   In that case, the digest_1x will agree, but disagree with digest_2 which used the differently
   initialized arena. */
	if ((digest_1a == digest_1b) && (digest_1a != digest_2)) {
		streamsUninitializedContent = true;
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

	return !streamsUninitializedContent;
}
