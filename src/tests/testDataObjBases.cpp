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

static testDataObjBases instance = testDataObjBases();

bool testDataObjBases::fRunTest(classObject& aClass) {
	auto cls = aClass.fGetTClass();
	cls->BuildRealData();
	TList* baseClasses = cls->GetListOfBases();
	TIter nextBase(baseClasses);
	TBaseClass* baseCl = nullptr;
	bool hasUnstreamedBaseMembers = false;
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
					hasUnstreamedBaseMembers = true;
					errorHandling::throwError(cls->GetDeclFileName(), 0, errorHandling::kError,
					                          TString::Format("Data object class '%s' inherits from '%s' which has class version '%d', members: %s will not be streamed!",
					                                  cls->GetName(), baseClPtr->GetName(), baseClPtr->GetClassVersion(), unstreamedData.Data()));
				}
			}
		}
	}
	return !hasUnstreamedBaseMembers;
}
