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
#include <algorithm>
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

// Inspired by TSystem::IsFileInIncludePath(), extended with possibility to strip ROOT_INCLUDE_PATH from lookup for special checks.
TString utilityFunctions::searchInIncludePath(const char* aFileName, Bool_t aStripRootIncludePath) {
	if (!aFileName || !aFileName[0]) {
		return kFALSE;
	}

	TString aclicMode;
	TString arguments;
	TString io;

	// Removes path part and interpreter-flags.
	TString realname = gSystem->SplitAclicMode(aFileName, aclicMode, arguments, io);

	TString fileLocation = gSystem->DirName(realname);

	TString incPath = gSystem->GetIncludePath(); // of the form -Idir1  -Idir2 -Idir3 -I"dir4"
	incPath.Append(":").Prepend(" ");
	incPath.ReplaceAll(" -I", ":");       // of form :dir1 :dir2:"dir3"
	while ( incPath.Index(" :") != -1 ) {
		incPath.ReplaceAll(" :", ":");
	}
	// Remove extra quotes around path expressions.
	incPath.ReplaceAll("\":", ":");
	incPath.ReplaceAll(":\"", ":");

	incPath.Prepend(fileLocation + ":.:");

	// Cleanup, remove double slashes.
	incPath.ReplaceAll("//", "/");

	if (aStripRootIncludePath) {
		const char* root_inc_path = gSystem->Getenv("ROOT_INCLUDE_PATH");
		if (root_inc_path != nullptr) {
			TString rootIncPath(root_inc_path);
			// Remove extra quotes, clean double slashes.
			rootIncPath.ReplaceAll("\":", ":");
			rootIncPath.ReplaceAll(":\"", ":");
			rootIncPath.ReplaceAll("//", "/");
			auto allPaths = rootIncPath.Tokenize(":");
			TIter pathIter(allPaths);
			TObjString* pathSeg = nullptr;
			while ((pathSeg = static_cast<TObjString*>(pathIter())) != nullptr) {
				TString pString(pathSeg->GetString());
				incPath.ReplaceAll(pString + ":", "");
				incPath.ReplaceAll(pString + "/:", "");
			}
			delete allPaths;
		}
	}
	char *actual = gSystem->Which(incPath, realname);

	if (actual == nullptr) {
		return "";
	} else {
		TString returnPath(actual);
		delete [] actual;
		return returnPath;
	}
}

TString utilityFunctions::performPathLookup(const char* file, Bool_t aRemoveRootIncludePath) {
	TString fileName = file;
	if (gSystem->AccessPathName(file) != kFALSE) {
		// False means FILE EXISTS, i.e. we come here if it does not exist!
		// This strange convention is documented.
		// Include-path searching needed starting from ROOT 6:
		const TString& classHeaderFileAbs = searchInIncludePath(fileName.Data(), aRemoveRootIncludePath);
		if ((classHeaderFileAbs.Length() > 0) && (gSystem->AccessPathName(classHeaderFileAbs) == kFALSE)) {
			// Found!
			return classHeaderFileAbs;
		}
		// Otherwise, we still did not find it...
		// Let's just write the original content there => do nothing.
	}
	return fileName;
}

void utilityFunctions::parseRootmap(const char* aFilename, std::set<std::string>& classNames) {
	FILE *f = fopen(aFilename, "r");
	char line[4096];
	while (fgets(line, sizeof(line), f) != nullptr) {
		char className[1024];
		int conversions = sscanf(line, "Library.%s:%*[^\n]", className);
		//std::cout << conversions << std::endl;
		if (conversions == 1) {
			// ROOT5-rootmap format, all fine.
			char *lastColon = strrchr(className, ':');
			(*lastColon) = '\0';
			std::replace(className, lastColon, '@', ':');
			std::replace(className, lastColon, '-', ' ');
			classNames.insert(className);
		} else {
			// New rootmap-format?
			conversions = sscanf(line, "class %[^\n]", className);
			if (conversions == 1) {
				// ROOT6-rootmap format, all fine.
				classNames.insert(className);
			} else {
				if (conversions == EOF) {
					break;
				}
				if (feof(f)) {
					break;
				}
			}
		}
	}
	fclose(f);
}

TString utilityFunctions::getRootLibDir() {
	auto sharedLibChar = TROOT::Class()->GetSharedLibs();
	TString sharedLib{sharedLibChar};
	sharedLib = sharedLib.Remove(TString::kBoth, ' ');
	auto sharedLibWithPath = gSystem->Which(gSystem->GetDynamicPath(), sharedLib);
	if (sharedLibWithPath == nullptr || strlen(sharedLibWithPath) == 0) {
		std::cerr << "Could not find ROOT library directory, it should contain '" << sharedLib << "'!" << std::endl;
		delete [] sharedLibWithPath;
		exit(1);
	}
	auto lastSlash = strrchr(sharedLibWithPath, '/');
	if (lastSlash == nullptr) {
		std::cerr << "Bad ROOT library directory: " << sharedLibWithPath << "!" << std::endl;
		delete [] sharedLibWithPath;
		exit(1);
	}
	*lastSlash = '\0';
	TString rootLibDir(sharedLibWithPath);
	delete [] sharedLibWithPath;
	return rootLibDir;
}

std::set<std::string> utilityFunctions::getRootmapsByRegexps(const std::vector<std::string>& rootMapPatterns, bool debug) {
	
	std::vector<TPRegexp> rootMapRegexps;
	for (auto& pattern : rootMapPatterns) {
		rootMapRegexps.emplace_back(pattern);
	}

	// Construct list of all classes to test.
	std::set<std::string> allClasses;
	{
		TObjArray* rootMaps = gInterpreter->GetRootMapFiles();
		TIter next(rootMaps);
		TNamed* rootMapName;
		while ((rootMapName = dynamic_cast<TNamed*>(next())) != nullptr) {
			TString path = rootMapName->GetTitle();
			if (debug) {
				std::cout << "Checking rootmap-path " << path.Data() << std::endl;
			}
			for (auto& pattern : rootMapRegexps) {
				if (pattern.MatchB(path)) {
					utilityFunctions::parseRootmap(path.Data(), allClasses);
					if (debug) {
						std::cout << "Path matched by regex '" << pattern.GetPattern() << "'." << std::endl;
					}
					break;
				}
			}
		}
	}
	return allClasses;
}

void utilityFunctions::filterSetByPatterns(std::set<std::string>& allNames,
                                           const std::vector<std::string>& namePatterns,
                                           const std::vector<std::string>& nameAntiPatterns,
                                           bool debug) {
	if (!namePatterns.empty()) {
		// Remove all which do not match any regexp in namePattern.
		std::vector<TPRegexp> nameRegexps;
		for (auto& pattern : namePatterns) {
			nameRegexps.emplace_back(pattern);
		}
		auto allNamesSav = allNames;
		for (auto& name : allNamesSav) {
			bool oneMatched = false;
			for (auto& regex : nameRegexps) {
				if (regex.MatchB(name)) {
					if (debug) {
						std::cout << "'" << name << "' kept since it matched pattern '" << regex.GetPattern().Data() << "'." << std::endl;
					}
					oneMatched = true;
					break;
				}
			}
			if (!oneMatched) {
				if (debug) {
					std::cout << "'" << name << "' removed since it did not match any pattern." << std::endl;
				}
				allNames.erase(name);
			}
		}
	}
	if (!nameAntiPatterns.empty()) {
		// Remove all which match any regexp in nameAntiPattern.
		std::vector<TPRegexp> nameRegexps;
		for (auto& pattern : nameAntiPatterns) {
			nameRegexps.emplace_back(pattern);
		}
		auto allNamesSav = allNames;
		for (auto& name : allNamesSav) {
			for (auto& regex : nameRegexps) {
				if (regex.MatchB(name)) {
					if (debug) {
						std::cout << "'" << name << "' kept since it matched anti-pattern '" << regex.GetPattern().Data() << "'." << std::endl;
					}
					allNames.erase(name);
					break;
				}
			}
		}
	}
}

#endif /* __utilityFunctions_h__ */
