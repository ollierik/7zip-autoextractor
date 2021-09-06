# 7zip-autoextractor

A simple C++ binary that uses 7z.dll to extract archives in contextually non-stupid way.

## Why?

One of the best features in OSX is being able to double click on a .zip file and having it extracted into the same directory.
On Windows, unzipping is (for reasons unknown) a very slow operation. Using 7zip makes extracting arhives a lot faster, and it provides a handy context menu for arhive files, with many options to choose from.
In 99% of the cases though, these options are not needed, as there would seem to be the only one "correct" option — the one that OSX seems to do without even asking.

This is a quick project that uses the 7z.dll to provide an executable that can be used as "Open with" unarchiver for Windows. It extracts the archive depeding on the file structure inside an archive.

Only single item in the archive? -> Extract just the file
Does the archive have more than one files at the root? -> Create a parent folder derived from the name of the archive
Do all the files in the archive reside inside a common directory? -> Use that as the parent directory

In each case, the name of the file or root directory may be incremented with " (n)" to avoid overwriting existing files.
No more huge archives having their guts accidentally splattered over Downloads.

## Dependencies
- The project has been (minimally and empirically) tested with recent version of Windows 10, running in x64.
- The included .sln currently compiles as C++17 (for std::filesystem), and as such probably requires VS2019
- The project depends on bit7z library for interfacing with 7z.dll


## Building
Clone 7zip-autoextractor and https://github.com/rikyoz/bit7z to same directory.
i.e.
-- my_code_dir/
---- 7zip-autoextractor/
---- bit7z/

Build bit7z using the instrucitions in their repository. 7zip-autoextractor requires the bit7z's DBIT7Z_AUTO_FORMAT=1 to work.
e.g.
```
...
cmake -G "Visual Studio 16 2019" -A x64 ../ -DCMAKE_BUILD_TYPE=Release -DBIT7Z_AUTO_FORMAT=1
cmake --build . --config Debug
```

Open .sln, compile. Move the build product to your 7zip folder. It needs to be in the same folder as 7z.dll.

Change your desired archive types' "Open with" to point to the `7z-autoextractor.exe`




