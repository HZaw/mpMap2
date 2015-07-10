## mpMap2

This is an updated version of the mpMap package for map construction and QTL mapping in RIL designs. This includes the F2, RIL, and 4- and 8- parent versions of those designs.

This package can be compiled in two ways; either using the standard package compilation commands (E.g. R CMD INSTALL) or by using the included CMake build files. The CMake build files allow compilation using Visual Studio on Windows.

##Compilation on Windows using CMake and Visual Studio

1. Download and compile the customized version of Rcpp from github repository rohan-shah/Rcpp. See the associated Readme file for details on compiling that code. 
2. Choose a binaries directory (E.g. <mpMap2Root>/build)
3. Run the cmake gui. 
4. Set Rcpp_DIR to the Rcpp binaries directory. 
5. Set R_COMMAND to <R_HOME>/bin/x64/R.exe. Ensure that you choose the 64-bit version. 
6. Enter the source directory and the binaries directory (E.g. <mpMap2Root>/build)
7. If the output is going to be NMake Makefiles, set CMAKE_BUILD_TYPE appropriately (E.g. as either Release or Debug)
8. Hit Configure and when prompted choose a Visual Studio 64-bit output, or NMake Makefiles.
9. When configuring succeeds, hit generate. 

The configuration scripts generate an import library for R.dll. This means that the scripts must be able to run cl.exe and lib.exe. If this step fails, check that cl.exe and lib.exe can run. If not, you may need to set up the correct environment for the compiler (by running a script such as vcvarsx86_amd64.bat) before running cmake. 

The package can now be compiled by either running nmake in the binaries directory (NMake Makefiles) or opening mpMap2.sln in the binaries directory. Once the package is compiled a properly formed R package (including NAMESPACE, DESCRIPTION, .R files and C code) will have been constructed in the binaries directory. If Visual Studio output was selected, the package directory will be <mpMap2Binaries>/<buildType> (E.g. mpMap2/build/Release for a release build). If NMake Makefiles output was selected, the package will be <mpMap2Binaries> (E.g mpMap2/build). 
