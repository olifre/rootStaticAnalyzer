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

#ifndef __classObject_h__
#define __classObject_h__

#include <TObject.h>
#include <TClass.h>
#include <string>

class classObject {
 protected:
	TClass* lClass;         //< Underlying TClass.
	std::string lClassName; //< Name of underlying class.
	bool lInheritsTObject;  //< Inherits from TObject. 
	bool lIsDataObject;     //< DataObject (TObjects with class version not <= 0).
	bool lHasNew;           //< Whether New() is useable. 
	bool lHasDelete;        //< Whether Destructor() is useable.

	std::map<std::string, bool> lTestedFeatures;
	
public:
	classObject(TClass* aClass) :
	lClass{aClass},
		lClassName{aClass->GetName()},
			lInheritsTObject{lClass->InheritsFrom(TObject::Class())},
				lIsDataObject{lInheritsTObject && !(lClass->GetClassVersion() <= 0)},
					lHasNew{lClass->GetNew() != nullptr},
						lHasDelete{lClass->GetDestructor() != nullptr}
						{
		
	};
	bool operator<( const classObject& other ) const {
		return (lClassName) < (other.lClassName);
	};

	TClass* fGetTClass() const {
		return lClass;
	}

	std::string fGetClassName() const {
		return lClassName;
	}

	bool fHasNew() const {
		return lHasNew;
	}
	bool fHasDelete() const {
		return lHasDelete;
	}

	bool fInheritsTObject() const {
		return lInheritsTObject;
	}
	
	bool fWasTested(std::string aTestName) const {
		return (lTestedFeatures.find(aTestName) != lTestedFeatures.end());
	}
	void fMarkTested(std::string aTestName, bool aTestResult) {
		lTestedFeatures[aTestName] = aTestResult;
	}
	bool fWasTestedSuccessfully(std::string aTestName) const {
		auto testRes = lTestedFeatures.find(aTestName);
		if (testRes == lTestedFeatures.end()) {
			return false;
		} else {
			return testRes->second;
		}
	}
};

#endif /* __classObject_h__ */
