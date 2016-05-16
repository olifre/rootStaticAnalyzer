#include "errorHandling.h"

#include "utilityFunctions.h"

#include <TSystem.h>
#include <TPRegexp.h>
#include <iostream>

void errorHandling::throwErrorInternal(const char* file, Int_t line, const char* message) {
	std::cerr << file << ":" << line << ": " << message << std::endl;
}

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
