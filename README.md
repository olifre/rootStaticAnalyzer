# rootStaticAnalyzer
A not-so-static post-compile-time analyzer for [ROOT](https://root.cern.ch) and [ROOT](https://root.cern.ch)-based projects. 

[![Build Status](https://travis-ci.org/olifre/rootStaticAnalyzer.svg?branch=master)](https://travis-ci.org/olifre/rootStaticAnalyzer)

#### Disclaimer
I'm not affiliated with [ROOT](https://root.cern.ch) (but I'm a fan!). As such, I'm working on this for my own well-being. 

So: If there are errors in the design of the tests, it's fully my fault! You have been warned! 

# WARNING
This project is still in VERY early stages. Use at your own risk!

# Implemented tests
#### Construction/Destruction
Tries to construct and destruct an object (if there is a default public constructor / destructor). 

Catches any segmentation-faults and issues an error-message + stacktrace on failure. 

#### Working IsA
Classes which survived the Construction/Destruction test and inherit from TObject are constructed and their "IsA()" is tested. 

#### Unstreamed datamembers from base-classes
Classes with class-version > 0 are checked for indirect datamembers (from bases) which are part of a class with class-version 0. 

This can happen if a dataobjects (meant for streaming) inherits from a class which is not meant to be streamed (but has non-transient members). 

In this often not-so-obvious case, the content of the members from the base-class are lost on streaming the derived class. 

#### Simple streaming after default construction
This streams all objects which are dataobjects (and for which Construction/Destruction did not fail) to a memory buffer. 

Potential segmentation faults are captured and a stacktrace plus an error message are shown. 

#### Test for streaming of uninitialized data after default construction
Objects which survived the simple streaming test are constructed / destructed in a prefilled arena. 

They are streamed in a buffer, which is checksummed afterwards. Checksums for simple members (using known memberoffsets) are also made. 

The objects are then constructed to a differently prefilled arena. 

If the streamed information changes, the objects have streamed uninitialized content, and we might even be able to blame the member. 

# Examples
(not yet there)
