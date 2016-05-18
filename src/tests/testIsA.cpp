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

#include "testIsA.h"
#include "errorHandling.h"

#include <TClass.h>

static testIsA instance = testIsA();

bool testIsA::fRunTest(classObject& aClass) {
	auto cls = aClass.fGetTClass();

	UInt_t classSize = cls->Size();
	UInt_t uintCount = classSize / sizeof(UInt_t) + 1;
	std::vector<UInt_t> storageArenaVector(uintCount);
	auto storageArena = storageArenaVector.data();

	TObject* obj = static_cast<TObject*>(cls->New(storageArena));
	bool IsAworked = true;
	if (obj->IsA() == nullptr) {
		errorHandling::throwError(cls->GetDeclFileName(), 0, errorHandling::kError,
		                          TString::Format("IsA() of TObject-inheriting class '%s' return nullptr, this should not be!", cls->GetName()));
		IsAworked = false;
	}
	cls->Destructor(obj, kTRUE);
	return IsAworked;
}
