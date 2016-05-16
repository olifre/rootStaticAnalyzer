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
	TClass* fClass;         //< Underlying TClass.
	std::string fClassName; //< Name of underlying class.
	bool fInheritsTObject;  //< Inherits from TObject. 
	bool fIsDataObject;     //< DataObject (TObjects with class version not <= 0).
	bool fHasNew;           //< Whether New() is useable. 
	bool fHasDelete;        //< Whether Destructor() is useable.

	std::map<std::string, bool> fTestedFeatures;
	
public:
	classObject(TClass* aClass) :
		fClass{aClass},
		fInheritsTObject{fClass->InheritsFrom(TObject::Class())},
		fIsDataObject{fInheritsTObject && !(fClass->GetClassVersion() <= 0)},
		fHasNew{fClass->GetNew() != nullptr},
		fHasDelete{fClass->GetDestructor() != nullptr}
	{
		
	};
	bool operator<( const classObject& other ) const {
		return (fClassName) < (other.fClassName);
	};

};

#endif /* __classObject_h__ */
