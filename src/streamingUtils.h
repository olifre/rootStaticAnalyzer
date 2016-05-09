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

#ifndef __streamingUtils_h__
#define __streamingUtils_h__

#include "errorHandling.h"

namespace streamingUtils {
	TString streamObjectToBufferAndChecksum(TBufferFile& buf, TObject* obj);
	std::map<TString, std::pair<TMD5, TRealData*>> getRealDataDigests(TObject* obj);
};

TString streamingUtils::streamObjectToBufferAndChecksum(TBufferFile& buf, TObject* obj) {
		// NECESSARY: Reset the map of the buffer, we are re-using it.
		// Buffers store internally a map of all known object pointers to only write them once.
		// For our check, we re-use the buffer and re-write to it from the start - thus, we need to reset the map.
		buf.ResetMap();

		// Reset buffer pointer to 0.
		buf.SetBufferOffset(0);

		// Add the clonesarray itself to the map to prevent self-reference issues.
		buf.MapObject(obj);

		// Stream it.
		obj->Streamer(buf);

		// Start the check.
		Int_t bufSize = buf.Length();
		buf.SetBufferOffset(0);
		char* bufPtr  = buf.Buffer();

		TMD5 checkSum;
		checkSum.Update(reinterpret_cast<UChar_t*>(bufPtr), bufSize);
		checkSum.Final();
		return checkSum.AsString();
}

std::map<TString, std::pair<TMD5, TRealData*>> streamingUtils::getRealDataDigests(TObject* obj) {
	std::map<TString, std::pair<TMD5, TRealData*>> digests;

	auto cls = obj->IsA();
	auto realData = cls->GetListOfRealData();
	if (realData != nullptr && realData->GetEntries() > 0) {
		TString unstreamedData;
		TIter nextRD(realData);
		TRealData* rd = nullptr;
		while ((rd = dynamic_cast<TRealData*>(nextRD())) != nullptr) {
			if (rd->IsObject()) {
				// Skip that.
				continue;
			}
			if (rd->TestBit(TRealData::kTransient)) {
				// Skip transient members. 
				continue;
			}
			auto dm = rd->GetDataMember();
			auto dt = dm->GetDataType();
			if (dt == nullptr) {
				continue;
			}
			/*
			std::cout << obj->IsA()->GetName() << " - "
			          << dm->GetName() << " - "
			          << dt->Size() << " @ " << rd->GetThisOffset() << std::endl;
			*/
			auto memberAddress = reinterpret_cast<UChar_t*>(obj) + rd->GetThisOffset();
			if (digests.find(rd->GetName()) != digests.end()) {
				errorHandling::throwError(cls->GetDeclFileName(), 0,
				                          TString::Format("warning: Class '%s' contains more than one realdata-member called '%s', that's a bad idea!", cls->GetName(), rd->GetName()));
				continue;
			}
			auto& digest = digests[rd->GetName()];
			//digest = new TMD5();
			digest.first.Update(memberAddress, dm->GetUnitSize()/sizeof(UChar_t));
			digest.first.Final();
			digest.second = rd;
			/*
			if (obj->IsA() == TClass::GetClass("TRandom3")) {
				rd->Dump();
				dm->Dump();
				dt->Dump();
			}
			*/
			//rd->Dump();
		}
	}
	return digests;
}

#endif /* __streamingUtils_h__ */
