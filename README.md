# Juce Easy Double-Precision POC (Proof of Concept / Experiment)

## Abstract

This is an example Juce-based application that demonstrates a method for implementing a double-precision audio processing algorithm automatically from single-precision code.  By employing some "DevOps" and simple preprocessing directives, we can avoid templating and thereby increase speed of development and maintainability of the code, reducing development cost and time to market and at the same time increasing run-time flexibility.

## Problem

The bulk of audio-processing APIs in Juce rely on code that deals with "`AudioBuffer<T>`" type, whose template type is typically "`float`" (single-precision) or "`double`" (double-precision).  Different audio host software or standalone implementations may use one or the other precision, so Juce-based code has to support both.  To avoid duplicating code, the software developer usually templatizes his classes and incurs the "template penalty" during development:  higher difficulty during development, testing, and maintenance of this code.  (The problem occurs because C++ does not allow virtual member functions to use templatized parameters; it is only allowed in templated classes.)  There is a need for a way to avoid the template interface patterns required inside this environment and to use object-oriented software designs that are easier to develop and maintain.  

Object-oriented software designs also provide runtime flexibility where templatized code is much more inflexible.  For example, consider the case of having effect processors that could be ordered in any way the user wishes.  It is virtually impossible with template interface patterns using only "`juce::dsp`" modules and "`ProcessorChain`"; however with standard O.O. techniques it is trivial.

## Hypothesis

By applying a little automation to the build process, we can easily generate double-precision code from single-precision code, writing code only once and avoiding templating.  The only requirements should be that the developer has to run a script to auto-copy code and transform one header file for use in double-precision mode.  The script should be OS-agnostic (Win/Mac/Linux) like Juce.  The developer should only have to write code once for one precision type (single or double), and the other precision type should be able to be auto-generated via that script.

## Procedure and Results

To test the hypothesis, I developed a script that can be run to auto-generate code for double-precision, as long as some requirements are followed.

### Requirements for Enabling Auto-Generation of Double Precision Code

Organize your single-precision code in the following way:

1. Add all of your audio-processing code inside of the subfolder for single-precision: "`Source/audio_processing_float/`"  Make sure there is a matching folder "GROUP" in the source listing in the projucer file configuration as well.  The easiest way is to add the entire folder directly into the projucer.
2.  Add a "`audio_processing_header.h`" file to the above subdirectory.
3.  Utilize the #define for "`SAMPLE_TYPE`" in your code instead of "`float`" or "`double`" audio sample types (for example use "`AudioBuffer<SAMPLE_TYPE>`" instead of "`AudioBuffer<float>`", etc).
4.  Add every class in that folder into the namespace "`AUDIO_PROCESSING_NAMESPACE`" so that it's easy to distinguish between the two versions with the same name (the script will create a new namespace name to distinguish them).
5.  Be sure NOT to include the "`audio_processing_header.h`" file anywhere but inside its own subdirectory (or below that directory).

### To Auto-Generate Double Precision Code

Provided the above requirements are met, it's easy to generate the double precision code:

1. Exit your IDE (or close the project), and exit the Projucer if it's running.
2. Run the script [bin/generate-double-precision-support.py](bin/generate-double-precision-support.py) to populate the double-precision directory.  It assumes the user wishes to copy single-precision to the double-precision dir, but this could be reversed with an argument in the future. (Run example: "`bin/generate-double-precision-support.py -j path/to/projucer/file.jucer`"  It does the following:
    1. Copies all the files from "audio_processing_float" to "audio_processing_double"
    2. Edits the file audio_processing_double/audio_processing_header.h and.  Changes that it makes are:
        * Updates the #define for `SAMPLE_TYPE` to be `double`
        * Updates the #define for `AUDIO_PROCESSING_NAMESPACE` to be `double`
        * Changes the header guard appropriately.
    3. Edits your "`*.jucer`" file to add the new files (it removes all existing files from that group first).
        * Finds the GROUP named "audio_processing_double" and deletes it
        * Finds the GROUP named "audio_processing_float" and copies it to "audio_processing_double".
        * Changes the GROUP id to be a different UUID
        * Changes the FILE ids to be new unique IDs.  (random 6-char [a-zA-Z0-9]{6} sequence (checked for uniqueness)
        * Fixes the paths to the files to be `Source/audio_processing_double/......`
        * Deletes the "Builds" directory (this works on Windows - not sure about MacOS/Linux yet - feedback welcome!)
4. Open the projucer file and click "Save and Open in IDE"
5. Edit your AudioProcessor class to call the double-precision audio renderer in the apropriate processBlock() function (requires a method override of the parent class).
6. Viola!  You now support 64-bit audio processing with no code changes!

## Conclusions and Discussion

The simple example audio processing code provided ("`SineWaveSynthesiser.h`") generates a sine wave at 440 Hz and writes to an "`AudioBuffer<SAMPLE_TYPE>`", but you will notice that no templating is required.  The `renderNextBlock` method takes an argument of "`juce::AudioBuffer<SAMPLE_TYPE>`".  This flexibility would make it easy to refactor the synth class into an interface and develop different implementations against it.

### Benefits

The developer can use concrete AudioBuffer types in all of her code, which allows you to create interfaces and APIs with little or no templating, which means:

* Development will be faster.
* Debugging will be easier.
* Run-time polymorphism is allowed.

### Drawbacks

There are some caveats when using this approach.

* Before the first run of the script, no calls to double-precision code can be made.  For example, you can't populate the "`AudioBuffer<double>`" version of processBlock() until you generate the double code. (Something to keep in mind.)
* After running the script, if you continue to develop, you will gather ever-more stale code in the double-precision dir, which may cause compile problems and development issues.  Workarounds:
    * One workaround would be to delete that dir again, and add #ifdefs around double code inside the AudioProcessor class.  Once you have your code working again, run the script and disable the ifdef.  Maybe the ifdef could be handled by the script as well.
    * Another workaround would be to add the script run as a pre-compilation step in the projucer file.  (An option could be added to the script to only run the code-generation step to enable this.)  This would only work in cases where you don't add any new files to the project, but this is a known limitation of Juce development.  If you add files, you have to close your IDE and muck around in the Projucer.  (Using this script as a base, it would be easy to create a script to auto-add files to the projucer though.)
* Shared audio-processing code in other modules or libraries will have to be handled carefully.  It is likely they would need to be converted to use the same namespaces for audio processing, or at least wrapped with code that uses the SAMPLE_TYPE as the concrete floating point type.  More discovery is needed here.

### Other Minor Considerations

* The parameters in the "`juce::AudioProcessorValueTreeState`" class are all "`float`" (single-precision) values.  This means any controls you have on your effect/instrument won't have the same resolution as the processing code.  This is probably not an issue, though; even assuming some human can hear the difference between 32-bit and 64-bit processed audio, there's no way their fingers can distinguish between it while turning a knob or moving a slider.  ;)  It may affect some very precise automation moves, but I can't believe this is much of an issue.
* Since the editor class is typically separate from the processing code, any samples displayed in any view components will be only a singular precision. This is also probably not a huge issue.


## Alternatives

Alternatives to this procedure that would give similar results:

1. Write your code to handle concrete types at the highest precision you want ("`double`" or "`long double`" if you have super-hearing or it is the future and 128-bit processing is common), and in the single-precision `processBlock()` function, copy the "`AudioBuffer<float>`" to a higher-precision AudioBuffer, then process in the higher precision, copying it back (and converting your samples) to the "`float`" buffer when done.  The tradeoff is that this incurs some extra processing and memory, which may be acceptable depending on yoru target platform(s).
2. One could always use templating in your code that deals with "`AudioBuffer<T>`".  The issues with that approach have been discussed already above.  :)

## Conclusion

Overall, I feel the benefits to developer productivity and code maintainability outweigh the stated drawbacks, unless you have a large code base already.  A large codebase with many modules and libraries will need to have the build procedures carefully thought out before implementing a process like this.

However, since this does incur a slight development cost to run the script and follow the rules, it is worth profiling the alternative option mentioned above before implementing this procedure.  The extra memory and processing may be acceptable, and that is the lowest-impact solution.
