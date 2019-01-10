# About
A Simple port knocking client for 64-bit Windows.

# Support
Appears to work on 64-bit Windows 7, unsure about other versions.

# Word on status
 * This is incomplete, and a bit crufty in some areas.
 * There is a basic interface for providing encryption using the Win32 API, but is incomplete. (only uses base64 atm.)
 * Some settings are not actually coded to work (yet). (In Edit -> Settings)
 * There are a few odd GUI bugs that would be pretty easy to fix (one or two lines of code.) However, I've not actually fixed them as I can just easily work around them.
 * There may be some minor memleaks; Win32 API is weird in what it expects you to cleanup, and what it cleans up for you automatically.
 * This program is technically "incomplete", but works enough for me, so I am sharing.

# Credits
 * Written by Joseph Kinsella
 * Icons and Bitmaps are from the [Open Icon Library](https://sourceforge.net/projects/openiconlibrary/)

# License
You are free to use this program how ever you see fit. Distribution of this code and/or executable is freely granted.

# Warranty
None

# Executable in directory was compiled as follows
```bash
./make.bat
```

This is a "static" binary with debug symbols enabled.

# Tools for development
 * [Vim](https://www.vim.org/)
 * git-bash [Git](https://git-scm.com/downloads)
 * gcc ([MinGW](https://mingw-w64.org/)
 * [ResEdit](http://www.resedit.net/)
