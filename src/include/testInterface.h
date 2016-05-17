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

#ifndef __testInterface_h__
#define __testInterface_h__

#include <map>
#include <vector>
#include <string>
#include <iostream>

#include "classObject.h"

class testInterface {
 private:
	static std::map<std::string, testInterface*>& fGetTestMap() {
		static std::map<std::string, testInterface*> lTestSet;
		return lTestSet;
	};
	
	static void fRegisterTest(std::string& aTest, testInterface* aThis) {
		fGetTestMap()[aTest] = aThis;
	}
	
 protected:
	std::string lTestName;

	virtual bool fCheckPrerequisites(classObject& /*aClass*/) {
		return true;
	};

	virtual bool fRunTest(classObject& /*aClass*/) = 0;

 public:
 testInterface(std::string aTestName) : lTestName{aTestName} {
		fRegisterTest(lTestName, this);
	};
	virtual ~testInterface() = default;

	virtual std::size_t fRunTestOnClasses(std::vector<classObject>& allClasses, bool debug=false) {
		std::size_t testsRun = 0;
		for (auto& cls : allClasses) {
			if (cls.fWasTested(fGetTestName())) {
				// We already tested this. 
				continue;
			}
			if (fCheckPrerequisites(cls)) {
				if (debug) {
					std::cout << fGetTestName() << ": Testing " << cls.fGetClassName() << std::endl;
				}
				bool result = fRunTest(cls);
				cls.fMarkTested(fGetTestName(), result);
				testsRun++;
			}
		}
		return testsRun;
	}

	virtual std::string fGetTestName() const {
		return lTestName;
	}
	
	static const std::map<std::string, testInterface*>& fGetAllTests() {
		return fGetTestMap();
	};
};

#endif /* __testInterface_h__ */
