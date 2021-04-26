# Juce Double-Precision POC (Proof of Concept)

This is an example Juce-based application that demonstrates a method for simplifying development by providing code-free double-precision audio fidelity out of single-precision code.

The bulk of audio-processing APIs in Juce rely on templated classes which require concrete types to be set at compile-time.  By applying a little DevOps to the build process, we can easily generate double-precision code from single-precision code, without having to rely on compile-time guarantees.  This makes it easier for developers because they can create code using run-time polymorphism and can sidestep much of the overhead and difficulty of templated code and template interface patterns.  

If the you create code that processes types like `AudioBuffer<SAMPLE_TYPE>` while following a few simple rules, to generate double-precision code all you have to do is run a simple script.  Then your code will work in single-precision and double-precision mode (ie. both `AudioBuffer<float>` and `AudioBuffer<double>`).  

## Steps to generate double-precision code:

1. Get your code working in single-precision mode, following these rules:
   a. Add all of your audio-processing code inside of the subfolder for single-precision: "`audio_processing_float/`"  Make sure there is a folder "GROUP" in the source listing in the jucer as well.
   b.  Copy in the provided example "`audio_processing_header.h`" file to the above subdirectory, and utilize the #define in your code for "`SAMPLE_TYPE`" where you would specify "`float`" or "`double`" audio sample types (for example use "`AudioBuffer<SAMPLE_TYPE>`" instead of "`AudioBuffer<float>`", etc).
   c.  Add every class in that folder into the namespace "`AUDIO_PROCESSING_NAMESPACE`" so that when the code generator runs, it's easy to distinguish between the two versions with the same name.
   d.  Be sure NOT to include the "`audio_processing_header.h`" file anywhere but inside its own subdirectory (or below that directory).
2. Exit the IDE and Projucer.
3. Run [this script](bin/generate-double-precision-support.py) to populate the double-precision directory.  It does the following:
   a. Copy all the files from "audio_processing_float" to "audio_processing_double"
   b. Edit the file audio_processing_double/audio_processing_header.h to:
       * Update #define for `SAMPLE_TYPE` to be `double`
       * Update #define for `AUDIO_PROCESSING_NAMESPACE` to be `double`
       * Change the header guard appropriately.
   c. Edit your "`*.jucer`" file to add the new files (it removes all existing from that dir first).
       * Find the GROUP named "audio_processing_double", delete it
       * Find the GROUP named "audio_processing_float", copy it to "audio_processing_double".
       * Change the GROUP id to be a different UUID
       * Change the FILE ids to be new unique IDs.  (random 6-char [a-zA-Z0-9]{6} sequence, check by grepping the file again, if found, re-randomize)
       * Fix the paths to the files to be `Source/audio_processing_double/......`
       * Delete the "Builds" directory (for Windows - not sure about MacOS/Linux/etc)
4. Open the projucer file and click "Save and Open in IDE"
5. Edit your AudioProcessor class to call the double-precision audio renderer in the apropriate processBlock() function (requires an override of the parent class).
6. Viola!  You now support 64-bit audio processing with no code changes!


