# LK

LK is a scripting language originally developed for the National Renewable Energy Laboratory's [System Advisor Model (SAM)](https://sam.nrel.gov). LK is designed to be small, fast, and easily embedded in other applications, and provides a way for users to to extend an application's built-in functionality.

This repository contains a cross-platform standard library of function calls and core LK engine, which includes a lexical analyzer, parser, compiler, and virtual machine. It comprises roughly 7000 lines of ISO-standard C++ code, and is only dependent on the Standard C++ Library (STL), making LK extremely tight and portable to various platforms. LK also provides a C language API for writing extensions that can be dynamically loaded at runtime.

LK can utilize standard 8-bit ASCII strings via the built-in std::string class, or can be configured to utilize an std::string-compliant string class from an external library. In this way, Unicode text can be supported natively. For example, direct integration and linkage with the wxWidgets C++ GUI library string class is provided as an option.

## Running LK Scripts

LK is designed to be integrated into other applications to add functionality to those applications.

The [WEX](https://github.com/NREL/wex) project includes the lkscript program, which is a stand-alone application that can be used to write and run LK scripts.

The [System Advisor Model](https://sam.nrel.gov) includes LK script, and integrates the WEX lkscript program in its user interface.

## LK Language Documentation

The documentation of the LK language is written in LaTeX:

* [PDF](doc/lk_guide.pdf)

* [Source documents](doc/)

## Contributing

If you have found an issue with LK or would like to make a feature request, please let us know by adding a new issue on the [issues page](https://github.com/NREL/lk/issues).

If you would like to submit code to fix an issue or add a feature, you can use GitHub to do so. The overall steps are to create a fork on GitHub.com using the link above, and then install GitHub on your computer and use it to clone your fork, create a branch for your changes, and then once you have made your changes, commit and push the changes to your fork. You can then create a pull request that we will review and merge into the repository if approved.

## License

LK is licensed under an MIT [license](LICENSE.md).
