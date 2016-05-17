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

#ifndef __testDataObjBases_h__
#define __testDataObjBases_h__

#include "testInterface.h"

class testDataObjBases : public testInterface {
  protected:
	virtual bool fCheckPrerequisites(classObject& aClass) {
		return aClass.fIsDataObject();
	};

	virtual bool fRunTest(classObject& aClass);

  public:
	testDataObjBases() : testInterface("DataObjBases") { };
};

#endif /* __testDataObjBases_h__ */
