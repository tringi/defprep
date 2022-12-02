# DLL Module-Definition (.Def) Files Preprocessor

Generates [.def](https://learn.microsoft.com/en-us/cpp/build/reference/module-definition-dot-def-files?view=msvc-170) file for your EXE or DLL,
according to details specified in the .info file, and macros provided on the command line.

## Usage

To use as part of the build process in Visual Studio, in project properties, navigate to:  
*"Configuration Properties / Build Events / Pre-Build Event"* and add similar line as following to the *"Command Line"* field:

    "$(SolutionDir)defprep64.exe"
        in:"$(ProjectDir)$(AssemblyName)-template.def"
        out:"$(ProjectDir)$(AssemblyName).def"
        info:"$(ProjectDir)$(AssemblyName).info"
        platform:$(PlatformTarget)
        parameter:value

*Line breaks added for readability*

## Parameters

* **in** specifies DEF file template, see Syntax below
* **out** specified DEF file to generate, used to link the project
* **info** specifies .info file as specified in [RsrcGen](https://github.com/tringi/rsrcgen) project  
  This is regular INI file and `[description]` section is searched for `internalname` string and `major` and `minor` version numbers.

## Example

somedll-template.def:

    LIBRARY #
    VERSION #.#
    EXPORTS
        ; Exported variable
        data=?data@NAMESPACE@@3PBUStructure@1@B    #x86
        data=?data@NAMESPACE@@3PEBUStructure@1@EB  #amd64
        data=?data@NAMESPACE@@3PEBUStructure@1@EB  #ARM64

somedll.info (partial):

    [description]
        internalname = SOMEDLL
        major = 3
        minor = 14

Pre-Build Event command line:

    defprep64.exe  in:somedll-template.def  info:somedll.info  out:somedll.def  x86

*You'd use $(ProcessorArchitecture) instead "x86" to pass the current architecture*

Resulting **somedll.def**:

    LIBRARY "SOMEDLL"
    VERSION 3.14
    EXPORTS
    data=?data@NAMESPACE@@3PBUStructure@1@B

## Syntax

The template files are regular DEF files, where following lines (if present) are replaced:

* `LIBRARY #` is replaced with `LIBRARY "text"` where *text* is `internalname` from *info* file described above
* `VERSION #.#` is replaced with `VERSION 123.456` where 123 and 456 are `major` and `minor` numbers from *info* file described above

Text lines may end with `#`-starting directives, the rules are following:

* Line without any `#` character are directly copied to output
* Line may contain more than one directive
  * All directives must match, for the line to be copied to output
* `#ABC` directives match any `ABC` or `ABC:anything` command-line parameter
* `#!ABC` directives only match if there are no `ABC` or `ABC:anything` command-line parameters
* `#ABC=DEF` directives only match if there's at least one `ABC:DEF` command-line parameter, i.e. the value must match
* If all directives on a line match, the line is copied to output, with directives removed

## Notes

* The preprocessor removes all comments, empty lines and extra whitespace
* All comparisons are **case sensitive**
* You can use `ABC=DEF` on command line, instead of `ABC:DEF` too

## Microsoft References

* [Module-Definition (.Def) Files](https://learn.microsoft.com/en-us/cpp/build/reference/module-definition-dot-def-files?view=msvc-170)
* [Exporting from a DLL Using DEF Files](https://learn.microsoft.com/en-us/cpp/build/exporting-from-a-dll-using-def-files?view=msvc-170)
