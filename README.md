# pico
![Release version](https://img.shields.io/badge/alpha-v0.1.0-red.svg)

Yet another text editor ought to be as simple as possible (following the KISS principle). It is purely written in C and
is designed to be used solely on Microsoft Windows (relies on Win32 API). MinGW-32 GCC has been used as the compiler.


# Obtaining binaries

The x86 (32-bit) binary can be obtained [here](https://github.com/makuke1234/pico/raw/main/pico.exe).


# Features

Currently a fixed number of features is supported:
- [x] file must be given as a command-line argument, 'raw editing'/'saving later to a file' is impossible for a reason
- [x] pico editor utilizes the whole command prompt window, window is as big as your console currently is
- [x] the last line of the window is dedicated to status, for example showing success or failure when an attempt to save the file has been made
- [x] the following keyboard shortcuts:
	| Key    | Action |
	| ------ | ------ |
	| ESC    | Closes the editor |
	| Ctrl+S | Tries to save the current open file |
	| Ctrl+R | Tries to reload contents of current file |
- [x] 2 ways to start the program:
	| Syntax        | Action |
	| ------------- | ------ |
	| pico          | Shows help |
	| pico \[file\] | Starts editor with the specified file,<br>file does not have to exist prior<br>(where \[file\] is the file's name) |


# License

As stated, the project uses MIT license.
