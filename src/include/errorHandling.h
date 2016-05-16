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

#include "utilityFunctions.h"

class errorHandling {
  private:
	static void throwErrorInternal(const char* file, Int_t line, const char* message) {
		std::cerr << file << ":" << line << ": " << message << std::endl;
	}
  public:
	static Bool_t throwError(const char* file, TPRegexp* lineMatcher, const char* message);
};

Bool_t errorHandling::throwError(const char* file, TPRegexp* lineMatcher, const char* message) {
	const TString& fileName = utilityFunctions::performPathLookup(file, kTRUE);
	Int_t lineNo = 0;
	if (gSystem->AccessPathName(fileName.Data()) == kFALSE) {
		// False means FILE EXISTS!
		FILE *f = fopen(fileName.Data(), "r");
		char line[4096];
		Bool_t found = kFALSE;
		while (fgets(line, sizeof(line), f) != nullptr) {
			lineNo++;
			if (lineMatcher->MatchB(line)) {
				// Found match!
				// Check whether there is a suppression active for this line.
				TPRegexp suppressExpr(".*rootStaticAnalyzer: ignore.*");
				if (suppressExpr.MatchB(line)) {
					// Yes, ignore that.
					return kFALSE;
				}
				found = kTRUE;
				break;
			}
			if (feof(f)) {
				// Reached EOF, not found:
				lineNo = 0;
				break;
			}
		}
		if (!found) {
			lineNo = 0;
		}
		fclose(f);
	}
	throwErrorInternal(fileName.Data(), lineNo, message);
	return kTRUE;
}

#endif /* __errorHandling_h__ */
