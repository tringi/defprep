# DLL Module-Definition (.Def) Files Preprocessor

Generates [.def](https://learn.microsoft.com/en-us/cpp/build/reference/module-definition-dot-def-files?view=msvc-170) file for your EXE or DLL,
according *annotations* provided on the command line, and optionally from details specified in the .info (ini) file.

## Example

somedll-template.def (containing placeholders and annotations):

    LIBRARY #
    VERSION #.#
    EXPORTS
        data=?data@NAMESPACE@@3PBUStructure@1@B    #x86
        data=?data@NAMESPACE@@3PEBUStructure@1@EB  #amd64
        data=?data@NAMESPACE@@3PEBUStructure@1@EB  #ARM64

somedll.info (partial, optional):

    [description]
        internalname = SOMEDLL
        major = 3
        minor = 14

Pre-Build Event command line (passing annotations as arguments):

    defprep64.exe  in:somedll-template.def  info:somedll.info  out:somedll.def  x86

* You'd use `$(ProcessorArchitecture)` instead "x86" to pass the current architecture
* You can use syntax `platform:$(ProcessorArchitecture)` then the annotation would need to say `#platform=x86`

Resulting **somedll.def**:

    LIBRARY "SOMEDLL"
    VERSION 3.14
    EXPORTS
    data=?data@NAMESPACE@@3PBUStructure@1@B

## Usage

To use as part of the build process in Visual Studio, in project properties, navigate to:
*"Configuration Properties / Build Events / Pre-Build Event"* and add similar line as following to the *"Command Line"* field:

    "$(SolutionDir)defprep64.exe"
        in:"$(ProjectDir)$(AssemblyName)-template.def"
        out:"$(ProjectDir)$(AssemblyName).def"
        info:"$(ProjectDir)$(AssemblyName).info"
        parameter:value [...]

*Line breaks added for readability*

## Parameters

* **in** - specifies DEF file template, see Syntax below
* **out** - specifies DEF file to generate, used to link the project
* **info** - specifies .info file as specified in [RsrcGen](https://github.com/tringi/rsrcgen) project
  * This is regular INI file and `[description]` section is searched for `internalname` string and `major` and `minor` version numbers.

## Syntax

The template files are regular DEF files, where following placeholder lines (if present) are replaced:

* `LIBRARY #` is replaced with `LIBRARY "text"` where *text* is `internalname` from the *info* file described above
* `VERSION #.#` is replaced with `VERSION 123.456` where 123 and 456 are `major` and `minor` numbers also from the *info* file described above

Text lines may end with `#`-starting annotations. The rules are following:

* Line without any `#` character is directly copied to output
* Line may contain more than one annotation
  * All annotations must match, for the line to be copied to output
* `#ABC` annotation match if any `ABC` or `ABC:anything` command-line parameter is present
* `#!ABC` annotation only match if there are no `ABC` or `ABC:anything` command-line parameters present
* `#ABC=DEF` annotations match only if there's at least one `ABC:DEF` command-line parameter
  * the value part of the parameter must match
* If all annotation on a line match, the line is copied to output, with annotations removed

## Notes

* The preprocessor removes all comments, empty lines and extra whitespace
* All comparisons are **case sensitive**
* You can use `ABC=DEF` on command line, instead of `ABC:DEF` too

## Microsoft References

* [Module-Definition (.Def) Files](https://learn.microsoft.com/en-us/cpp/build/reference/module-definition-dot-def-files?view=msvc-170)
* [Exporting from a DLL Using DEF Files](https://learn.microsoft.com/en-us/cpp/build/exporting-from-a-dll-using-def-files?view=msvc-170)
