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

dependencies:

<a href="https://github.com/Reputeless/Xoshiro-cpp">Xoshiro-cpp</a>, a good random number generator for audio uses

<a href="https://github.com/fmtlib/fmt">fmt</a>, formatting library. 

from my libraries:
<a href="https://github.com/nvssynthesis/nvs_libraries">nvs_libraries</a>, in particular nvs_gen, nvs_memoryless
