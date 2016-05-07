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

#ifndef __utilityFunctions_h__
#define __utilityFunctions_h__

namespace utilityFunctions {
	TString searchInIncludePath(const char* aFileName, Bool_t aStripRootIncludePath);
	TString performPathLookup(const char* file, Bool_t aRemoveRootIncludePath = kFALSE);
	
	std::map<TString, std::pair<TMD5, TRealData*>> getRealDataDigests(TObject* obj);
	TString streamObjectToBufferAndChecksum(TBufferFile& buf, TObject* obj);

};

// Inspired by TSystem::IsFileInIncludePath(), extended with possibility to strip ROOT_INCLUDE_PATH from lookup for special checks.
TString utilityFunctions::searchInIncludePath(const char* aFileName, Bool_t aStripRootIncludePath) {
	if (!aFileName || !aFileName[0]) {
		return kFALSE;
	}

	TString aclicMode;
	TString arguments;
	TString io;

	// Removes path part and interpreter-flags.
	TString realname = gSystem->SplitAclicMode(aFileName, aclicMode, arguments, io);

	TString fileLocation = gSystem->DirName(realname);

	TString incPath = gSystem->GetIncludePath(); // of the form -Idir1  -Idir2 -Idir3 -I"dir4"
	incPath.Append(":").Prepend(" ");
	incPath.ReplaceAll(" -I", ":");       // of form :dir1 :dir2:"dir3"
	while ( incPath.Index(" :") != -1 ) {
		incPath.ReplaceAll(" :", ":");
	}
	// Remove extra quotes around path expressions.
	incPath.ReplaceAll("\":", ":");
	incPath.ReplaceAll(":\"", ":");

	incPath.Prepend(fileLocation + ":.:");

	// Cleanup, remove double slashes.
	incPath.ReplaceAll("//", "/");

	if (aStripRootIncludePath) {
		const char* root_inc_path = gSystem->Getenv("ROOT_INCLUDE_PATH");
		if (root_inc_path != nullptr) {
			TString rootIncPath(root_inc_path);
			// Remove extra quotes, clean double slashes.
			rootIncPath.ReplaceAll("\":", ":");
			rootIncPath.ReplaceAll(":\"", ":");
			rootIncPath.ReplaceAll("//", "/");
			auto allPaths = rootIncPath.Tokenize(":");
			TIter pathIter(allPaths);
			TObjString* pathSeg = nullptr;
			while ((pathSeg = static_cast<TObjString*>(pathIter())) != nullptr) {
				TString pString(pathSeg->GetString());
				incPath.ReplaceAll(pString + ":", "");
				incPath.ReplaceAll(pString + "/:", "");
			}
			delete allPaths;
		}
	}
	char *actual = gSystem->Which(incPath, realname);

	if (actual == nullptr) {
		return "";
	} else {
		TString returnPath(actual);
		delete [] actual;
		return returnPath;
	}
}

TString utilityFunctions::performPathLookup(const char* file, Bool_t aRemoveRootIncludePath) {
	TString fileName = file;
	if (gSystem->AccessPathName(file) != kFALSE) {
		// False means FILE EXISTS, i.e. we come here if it does not exist!
		// This strange convention is documented.
		// Include-path searching needed starting from ROOT 6:
		const TString& classHeaderFileAbs = searchInIncludePath(fileName.Data(), aRemoveRootIncludePath);
		if ((classHeaderFileAbs.Length() > 0) && (gSystem->AccessPathName(classHeaderFileAbs) == kFALSE)) {
			// Found!
			return classHeaderFileAbs;
		}
		// Otherwise, we still did not find it...
		// Let's just write the original content there => do nothing.
	}
	return fileName;
}

TString utilityFunctions::streamObjectToBufferAndChecksum(TBufferFile& buf, TObject* obj) {
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

std::map<TString, std::pair<TMD5, TRealData*>> utilityFunctions::getRealDataDigests(TObject* obj) {
	std::map<TString, std::pair<TMD5, TRealData*>> digests;
	
	auto realData = obj->IsA()->GetListOfRealData();
	if (realData != nullptr && realData->GetEntries() > 0) {
		TString unstreamedData;
		TIter nextRD(realData);
		TRealData* rd = nullptr;
		while ((rd = dynamic_cast<TRealData*>(nextRD())) != nullptr) {
			if (rd->IsObject()) {
				// Skip that.
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

#endif /* __utilityFunctions_h__ */
