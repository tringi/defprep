# DLL Module-Definition (.Def) Files Preprocessor

Generates [.def](https://learn.microsoft.com/en-us/cpp/build/reference/module-definition-dot-def-files?view=msvc-170) file for your EXE or DLL,
according to details specified in the .info file, and macros provided on the command line.

## Usage

To use as part of the build process in Visual Studio, in project properties, navigate to:  
*"Configuration Properties / Build Events / Pre-Build Event"* and add similar line as following to the *"Command Line"* field:

    "$(SolutionDir)defprep.exe"
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

## Syntax

The template files are regular DEF files, where following lines are replaced:

* `LIBRARY #` is replaced with `LIBRARY "text"` where *text* is `internalname` from *info* file described above
* `VERSION #.#` is replaced with `VERSION 123.456` where 123 and 456 are `major` and `minor` numbers from *info* file described above

Text lines may end with `#`-starting directives, the rules are following:

* Line without any `#` character are directly copied to output
* Line may contain more than one directive
  * All directives must match for line to be copied to output

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
