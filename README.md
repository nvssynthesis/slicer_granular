slicer-granular is a granular synthesizer. this is a work in progress. it is intended as the ground of a granular synth that will incorporate TSARA (timbre space analysis-resynthesis) capabilities. this version does not possess these abilities; they will be added later once the basic features are in top shape.
the design here is optimized for maintainability and extendability. for instance, it will be simple to add many new parameters without altering lots of areas of the code.
for now, i recommend trying the standalone application rather than any plugin version, as this is the most tested. not to mention, the instrument is not yet incorporating MIDI in a meaningful way; it's droning as soon as you load an audio file in. however, it can of course be recorded by using a virtual audio interface such as blackhole.

dependencies:

<a href="https://github.com/Reputeless/Xoshiro-cpp">Xoshiro-cpp</a>, a good random number generator for audio uses

<a href="https://github.com/serge-sans-paille/frozen">frozen map</a>, a map data structure which is  statically initialized. actually, you don't need this if you #define FROZEN_MAP 0; instead it will then use the StaticMap data structure defined in this repo's DataStructures.h. However, as the project grows I may use more data structures from frozen.

from my libraries:
<a href="https://github.com/nvssynthesis/nvs_libraries">nvs_libraries</a>, in particular nvs_gen, nvs_memoryless
