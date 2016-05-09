#!/usr/bin/env bash

stylefile=astyle_file

files=$(find src/ -iname '*.cpp' -or -iname '*.h' -or -iname '*.hh' -type f)
if [ -z "$files" ]; then
	exit 0
fi

astyle --options=${stylefile} ${files}
