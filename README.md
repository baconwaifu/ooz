# ooz
Open source Kraken / Mermaid / Selkie / Leviathan / LZNA / Bitknit decompressor

Can also compress Kraken / Mermaid / Selkie / Leviathan

Also supports using oo2core_7_win32.dll / oo2core_7_win64.dll which can be acquired from the free game Warframe on steam.

```
ooz v7.0

Usage: ooz [options] input [output]
 -c --stdout              write to stdout
 -d --decompress          decompress (default)
 -z --compress            compress
 -b                       just benchmark, don't overwrite anything
 -f                       force overwrite existing file
 --dll                    decompress with the dll
 --verify                 decompress and verify that it matches output
 --verify=<folder>        verify with files in this folder
 -<1-9> --level=<-4..10>  compression level
 -m<k>                    [k|m|s|l|h] compressor selection
 --kraken --mermaid --selkie --leviathan --hydra    compressor selection

(Warning! not fuzz safe, so please trust the input)
```

## Building
There are two options for building ooz: Visual Studio, and CMake. With Visual Studio, just open the project and hit
compile. For CMake, it's a little more in-depth:

```bash
mkdir build/
cd build
cmake ..
make -j 4
sudo make install
```

Visual Studio will produce a runnable binary directly, while CMake instead produces a shared library.

### CMake Config flags

* OODLE_ENABLE_DLL: Enables runtime linking against the proprietary DLL in Windows Builds (Default: Off)
* OODLE_ENABLE_AVX2: Enables AVX/AVX2 support in GCC. Resulting executable requires AVX2 to run. (Default: On)
* OODLE_FAIL_ON_WARNING: Mostly for developers; used to halt the build on any compiler warning. (Default: Off)

DLL loading is disabled by default as it conflicts with the spirit, possibly also the letter, of the GPL, and doesn't work on Linux anyways without building a 'wine-hybridized' executable
(yes, you can actually do that. you shouldn't, but you can)
Relevant GPL [FAQ](http://www.gnu.org/licenses/gpl-faq.html#GPLIncompatibleLibs); however the specific linking behavior may fall under the 'Separate Program' exception, since the only interface
between the two is the C-equivalent to 'calling another executable and getting a result', rather than exchanging complex data structures and long-term state, and the
program is capable of operating on it's own without the DLL, and the DLL without the program.

## Roadmap
- [x] Compile as shared library
- [ ] Compile main executable on Linux
- [ ] Working `main()` on Linux
- [ ] Add fallback implementation for AVX intrinsics for non-x86 targets
- [ ] Hardening and Fuzzing (needs clang support?)
- [ ] Add Validation test cases (Compress and decompress samples)
- [ ] Make library present same interface as oo2core
- [ ] Add dll spec, and add dll target to VS build
