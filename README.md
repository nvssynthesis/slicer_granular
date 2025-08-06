slicer-granular is a granular synthesizer, developed primarily as the basis of a more advanced [granular synth that incorporates timbre space navigation (TSN) capabilities](https://github.com/nvssynthesis/tsn-granular/). 

[![YouTube](http://i.ytimg.com/vi/gGwrM2HmIXU/hqdefault.jpg)](https://www.youtube.com/watch?v=gGwrM2HmIXU)

the design of slicer-granular is optimized for maintainability and extendability. for instance, it will be simple to add many new parameters without altering lots of areas of the code.

features:
<ul>
<li>granulation of lossless audio files</li>
<li>simple audio file importing, with stored history of recent files</li>
<li>shows indication of each grain's position with in the waveform, as well as its panning and level</li>
</ul>
parameters:
<ul>
<li>transpose + randomness amount</li>
<li>position + randomness amount</li>
<li>speed + randomness amount</li>
<li>duration + randomness amount</li>
<li>skew + randomness amount</li>
<li>pan + randomness amount</li>
</ul>


## Prerequisites

- **JUCE 7.x**: Download and install from [JUCE website](https://juce.com/get-juce)
- **CMake 3.15+**: [Download CMake](https://cmake.org/download/)
- **Git**: For cloning submodules
- **C++20 compatible compiler**

### Platform-specific:
- **macOS**: Xcode command line tools
- **Windows**: Visual Studio 2019+ or MinGW
- **Linux**: GCC 9+ or Clang 10+

## Build Instructions

### 1. Clone the repository
```bash
git clone https://github.com/nvssynthesis/slicer_granular.git
cd slicer_granular
git submodule update --init --recursive

# Point to your JUCE installation
ln -s /Applications/JUCE ./JUCE


# Or if JUCE is installed elsewhere:
ln -s /path/to/your/JUCE ./JUCE

mkdir build
cd build
cmake ..
cmake --build . --config Release

# For CLion: Open the project root directory
# CMake will auto-configure

# For Xcode:
cmake .. -G Xcode
open slicer_granular.xcodeproj

# For Visual Studio:
cmake .. -G "Visual Studio 16 2019"
```

## Dependencies
This project automatically downloads:

- **fmt**: String formatting library
- **Xoshiro-cpp**: Random number generation

### Local dependencies (included as submodules):

- **nvs_libraries**: Custom utility library (includes Sprout)

## Plugin Formats
Builds the following formats:

- **VST3**: Cross-platform plugin
- **AU**: macOS Audio Unit
- **Standalone**: Desktop application

## Troubleshooting
### "JuceHeader.h not found"

Ensure the JUCE symlink is created correctly: ls -la JUCE

Delete build directory and rebuild: 
```bash
rm -rf build && mkdir build && cd build && cmake ..
```

### "nvs_libraries not found"

Initialize submodules: 

```bash
git submodule update --init --recursive
```

### CMake configuration fails

Check CMake version: 

```bash 
cmake --version (requires 3.15+)
```

And verify the JUCE installation path.