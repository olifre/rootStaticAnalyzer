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

#include "testConstructionDestruction.h"

static testConstructionDestruction instance = testConstructionDestruction();

#include <TClass.h>
#include <TException.h>
#include <vector>

#include "errorHandling.h"

bool testConstructionDestruction::fRunTest(classObject& aClass) {
	auto cls = aClass.fGetTClass();

	UInt_t classSize = cls->Size();
	UInt_t uintCount = classSize / sizeof(UInt_t) + 1;
	std::vector<UInt_t> storageArenaVector(uintCount);
	auto storageArena = storageArenaVector.data();

	// Test default construction / destruction.
	volatile bool constructionDestructionWorked = true;

	TRY {
		TObject* obj = static_cast<TObject*>(cls->New(storageArena));
		cls->Destructor(obj, kTRUE);
	} CATCH ( excode ) {
		constructionDestructionWorked = false;
		Throw( excode );
	}
	ENDTRY;

	if (!constructionDestructionWorked) {
		errorHandling::throwError(cls->GetDeclFileName(), 0, errorHandling::kError,
		                          TString::Format("Construction/Destruction of class '%s' failed, manual check of backtrace above is needed!", cls->GetName()));
	}

	return constructionDestructionWorked;
};
