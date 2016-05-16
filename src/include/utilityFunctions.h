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

#ifndef __utilityFunctions_h__
#define __utilityFunctions_h__

#include <TString.h>
#include <string>
#include <set>

namespace utilityFunctions {
	TString searchInIncludePath(const char* aFileName, Bool_t aStripRootIncludePath);
	TString performPathLookup(const char* file, Bool_t aRemoveRootIncludePath = kFALSE);

	void parseRootmap(const char* aFilename, std::set<std::string>& classNames);

	TString getRootLibDir();

	std::set<std::string> getRootmapsByRegexps(const std::vector<std::string>& rootMapPatterns, bool debug);
	void filterSetByPatterns(std::set<std::string>& allClasses,
	                         const std::vector<std::string>& classNamePatterns,
	                         const std::vector<std::string>& classNameAntiPatterns,
	                         bool debug);
};

#endif /* __utilityFunctions_h__ */
