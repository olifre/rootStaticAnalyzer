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

#include "testDataObjBases.h"
#include "errorHandling.h"

#include <TRealData.h>
#include <TBaseClass.h>
#include <TList.h>
#include <TDataMember.h>

static testDataObjBases instance = testDataObjBases();

bool testDataObjBases::fRunTest(classObject& aClass) {
	auto cls = aClass.fGetTClass();
	cls->BuildRealData();
	bool hasUnstreamedMembers = false;
	auto realData = cls->GetListOfRealData();
	if (realData != nullptr && realData->GetEntries() > 0) {
		std::map<std::pair<std::string, Int_t>, std::vector<std::string>> unstreamedClassMembers;
		TIter nextRD(realData);
		TRealData* rd = nullptr;
		while ((rd = dynamic_cast<TRealData*>(nextRD())) != nullptr) {
			if (rd->TestBit(TRealData::kTransient)) {
				// Skip transient members.
				continue;
			}
			if (strchr(rd->GetName(), '.') != nullptr) {
				// That's a member of one of our members.
				// Don't descend, since only transientness of OUR member is interesting here.
				continue;
			}
			auto dm = rd->GetDataMember();
			auto dmClass = dm->GetClass();
			if (dmClass->GetClassVersion() <= 0) {
				// Ok, then this non-transient member will not be streamed - due to class-version zero!
				unstreamedClassMembers[std::make_pair(dmClass->GetName(), dmClass->GetClassVersion())].push_back(rd->GetName());
			}
		}
		if (!unstreamedClassMembers.empty()) {
			hasUnstreamedMembers = true;
			TString unstreamedMembers;
			for (auto& unstreamed : unstreamedClassMembers) {
				if (unstreamedMembers.Length() > 0) {
					unstreamedMembers += ", ";
				}
				TString memberList;
				for (auto& memberName : unstreamed.second) {
					if (memberList.Length() > 0) {
						memberList += ", ";
					}
					memberList += memberName;
				}
				unstreamedMembers += TString::Format("members '%s' from class '%s' (class-version %d)", memberList.Data(), unstreamed.first.first.c_str(), unstreamed.first.second);
			}
			errorHandling::throwError(cls->GetDeclFileName(), 0, errorHandling::kError,
			                          TString::Format("Data object class '%s' will not stream the following indirect members: %s!",
			                                  cls->GetName(), unstreamedMembers.Data()));
		}
	}
	return !hasUnstreamedMembers;
}
