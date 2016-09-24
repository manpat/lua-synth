LuaSynth
========

LuaSynth is a small experimental project with the goal of enabling quick and iterative composition of procedural and potentially interactive audio.

'Synths' are described with a relatively simple lua interface (imo) and audio is synthesized on the fly.
Some objects allow various parameters to be modified at runtime, which means that synths can be integrated with gameplay code to create more dynamic audio.
Or at least, that's the idea.
It's still very much a work in progress.

At the moment it depends on lua, SDL2 (mainly for simple audio output), and libsndfile.
Although the dependency on libsndfile can be removed fairly easily, as it's mainly for testing purposes.