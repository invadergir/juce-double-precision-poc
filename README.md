# Juce Easy Double-Precision POC (Proof of Concept)

This is an example Juce-based application that demonstrates a method for simplifying development by providing code-free double-precision audio fidelity out of single-precision code.

The bulk of audio-processing APIs in Juce rely on templated classes which require concrete types to be set at compile-time.  By applying a little DevOps to the build process, we can easily generate double-precision code from single-precision code, without having to rely on compile-time guarantees.  This makes it easier for developers because they can create code using run-time polymorphism and can sidestep much of the overhead and difficulty of templated code and template interface patterns.  

If the you create code that processes types like `AudioBuffer<SAMPLE_TYPE>` while following a few simple rules, to generate double-precision code all you have to do is run a simple script.  Then your code will work in single-precision and double-precision mode (ie. both `AudioBuffer<float>` and `AudioBuffer<double>`).  

## Steps to generate double-precision code:

1. Get your code working in single-precision mode, following these rules:
    1. Add all of your audio-processing code inside of the subfolder for single-precision: "`audio_processing_float/`"  Make sure there is a folder "GROUP" in the source listing in the jucer as well.
    2.  Copy in the provided example "`audio_processing_header.h`" file to the above subdirectory, and utilize the #define in your code for "`SAMPLE_TYPE`" where you would specify "`float`" or "`double`" audio sample types (for example use "`AudioBuffer<SAMPLE_TYPE>`" instead of "`AudioBuffer<float>`", etc).
    3.  Add every class in that folder into the namespace "`AUDIO_PROCESSING_NAMESPACE`" so that when the code generator runs, it's easy to distinguish between the two versions with the same name.
    4.  Be sure NOT to include the "`audio_processing_header.h`" file anywhere but inside its own subdirectory (or below that directory).
2. Exit the IDE and Projucer.
3. Run [this script](bin/generate-double-precision-support.py) to populate the double-precision directory.  It assumes the user wishes to copy single-precision to the double-precision dir, but this could be reversed with an argument in the future.(Run example: "`bin/generate-double-precision-support.py -j path/to/projucer/file.jucer`"  It does the following:
    1. Copy all the files from "audio_processing_float" to "audio_processing_double"
    2. Edit the file audio_processing_double/audio_processing_header.h to:
       * Update #define for `SAMPLE_TYPE` to be `double`
       * Update #define for `AUDIO_PROCESSING_NAMESPACE` to be `double`
       * Change the header guard appropriately.
    3. Edit your "`*.jucer`" file to add the new files (it removes all existing from that dir first).
        * Find the GROUP named "audio_processing_double", delete it
        * Find the GROUP named "audio_processing_float", copy it to "audio_processing_double".
        * Change the GROUP id to be a different UUID
        * Change the FILE ids to be new unique IDs.  (random 6-char [a-zA-Z0-9]{6} sequence, check by grepping the file again, if found, re-randomize)
        * Fix the paths to the files to be `Source/audio_processing_double/......`
        * Delete the "Builds" directory (for Windows - not sure about MacOS/Linux/etc)
4. Open the projucer file and click "Save and Open in IDE"
5. Edit your AudioProcessor class to call the double-precision audio renderer in the apropriate processBlock() function (requires an override of the parent class).
6. Viola!  You now support 64-bit audio processing with no code changes!

## Benefits

You can use concrete AudioBuffer types in all your code, which allows you to create interfaces and APIs with much less templating, which means:

* development will be faster
* debugging will be easier

## Drawbacks

* Before the first run of the script, no calls to double-precision code can be made.  For example, you can't populate the `AudioBuffer<double>` version of processBlock() until you generate the double code. (Something to keep in mind.)
* After running the script, if you continue to develop, you will gather ever-more stale code in the double-precision dir, which may cause compile problem and development issues.  Workarounds: 
    * One workaround would be to delete that dir again, and add #ifdefs around double code inside the AudioProcessor class.  Once you have your code working again, run the script and disable the ifdef.  Maybe the ifdef could be handled by the script as well.
    * Another workaround would be to add the script run as a pre-compilation step in the projucer file.  This would only work in cases where you don't add any new files to the project, but this is a known limitation of Juce development.  If you add files, you have to close your IDE and muck around in the Projucer.  Using this script as a base, it would be easy to create a script to auto-add files to the projucer though.
* Shared audio-processing code in other modules or libraries will have to be handled carefully.  It's likely they would need to be converted to use the same namespaces for audio processing, or at least wrapped with code that uses the SAMPLE_TYPE as the concrete floating point type.

### Other Minor Considerations

* The parameters in the `juce::AudioProcessorValueTreeState` class are all `float` (single-precision) values.  This means any controls you have on your effect/instrument won't have the same resolution as the processing code.  This is probably not an issue though.  Even assuming some human can hear the difference between 32-bit and 64-bit processed audio, there's no way their fingers can distinguish between it while turning a knob or moving a slider.  ;)  It may affect automation but I can't believe this is much of an issue.
* Since the editor class is typically separate from the processing code, any samples displayed in any view components will be only a singular precision. This is also probably not a huge issue.

Overall, I feel the benefits to developer productivity and code maintainability outweigh the stated drawbacks, unless you have a large code base already.

## Alternatives

* Write your code to also handle the highest precision you want (double or long double if you have super hearing), and in the single-precision `processBlock()` function, copy the AudioBuffer<float> to a higher-precision AudioBuffer, then process in the higher precision, copying it back to the float buffer when done.  This incurs some extra processing and memory, which may be acceptable especially as PC specs keep increasing.
* Use templating in code that deals with "`AudioBuffer<T>`".

