# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

`DataPersistence` is a Visual Studio C++20 console application project (currently an empty scaffold — no source files have been added yet). The solution targets Win32 and x64 platforms.

- Solution file: `DataPersistence.slnx`
- Project file: `DataPersistence/DataPersistence.vcxproj`

## Build

Build via MSBuild (Visual Studio 2022 / v145 toolset):

```
msbuild DataPersistence.slnx /p:Configuration=Debug /p:Platform=x64
msbuild DataPersistence.slnx /p:Configuration=Release /p:Platform=x64
```

Or open `DataPersistence.slnx` in Visual Studio and build from there.

- Language standard: C++20 (`stdcpp20`)
- Output type: Console application
- Supported platform/configuration combinations: Debug|Win32, Release|Win32, Debug|x64, Release|x64

No test framework or lint tooling is configured yet.
