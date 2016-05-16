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

#ifndef __errorHandling_h__
#define __errorHandling_h__

#include <Rtypes.h>
class TPRegexp;

class errorHandling {
 public:
	enum errorType {
		kError,
		kWarning,
		kNotice
	};
  private:
	static void throwErrorInternal(const char* file, Int_t line, errorType errType, const char* message);
  public:
	static Bool_t throwError(const char* file, TPRegexp* lineMatcher, errorType errType, const char* message);
};

#endif /* __errorHandling_h__ */
