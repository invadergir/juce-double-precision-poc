# Juce Easy Double-Precision POC (Proof of Concept / Experiment)

## Abstract

This example Juce application demonstrates two successful techniques for simplifying software engineering in the Juce environment by removing the need for templatized class types in the code. The first method generates double-precision audio processing code automatically from single-precision code.  For apps targeting very performance-constrained devices (such as mobile), it would be appropriate to apply this build process to make development easier at the cost of a small amount of process overhead.  For apps targeting higher performance devices (desktop/laptop), it is instead recommended to implement the second technique, using double precision processing but converting lower-precision buffers prior to and after processing, which gave acceptable performance.

In both cases, the described methodology avoids templating and thereby increases speed of development and maintainability of the code, reducing development cost and time to market and at the same time increasing run-time flexibility.

## Problem Description

The bulk of audio-processing APIs in Juce rely on code that deals with "`AudioBuffer<T>`" type, whose template type is typically "`float`" (single-precision) or "`double`" (double-precision).  Different audio host software or standalone implementations may use one or the other precision, so Juce-based code has to support both.  To avoid duplicating code, the software developer usually templatizes their classes and incurs the "template penalty" during development:  higher difficulty during development, testing, and maintenance of this code.  (The problem occurs because C++ does not allow virtual member functions to use templatized parameters; it is only allowed in templated classes.)  There is a need for a way to avoid the template interface patterns required inside this environment and to use object-oriented software designs that are easier to develop and maintain.

The other benefit of object-oriented software designs is that they provide runtime flexibility where templatized code is much less so.  For example, consider the case of having effect processors that could be ordered in any way the user wishes.  It is very difficult and maybe prohibitively memory-exensive to implement this with templates using "`juce::dsp`"-based effect processors and "`juce::dsp::ProcessorChain`"; all of the possible effect combinations would have to be declared and instantiated at compile time and activated based on the user's choices.  With standard object-oriented techniques this is comparatively trivial, where effect processing objects can be slotted into place very easily in any order the user wishes.

## Hypothesis

By applying a little automation to the build process, we should be able to easily generate double-precision code from single-precision code, writing code only once and avoiding templating.  The only requirements should be that the developer has to run a script to auto-copy code and transform some configuration files for use in double-precision mode.  The script should be cross-platorm like Juce.  The developer should only have to write code once for one precision type (single or double), and the other precision type should be able to be auto-generated via that script.  The build-process modification should be just as fast as templatized code, and the build-process change should compare favorably to the speed of copying low-precision audio buffers before and after double-precision audio processing.

## Procedure

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

### Performance Testing Procedure

Note that the performance difference between the script-generated code and a templatized version of the same code was not performed.  We can assume that the performance would be exactly the same, since for templates, the compiler creates multiple distinct class types based on each given template type; and this is exactly what I have done, in a different way outside of the compiler.

Performance of each of the two scenarios was measured using my Profiler and multi-threaded logging (MTLogger) classes.  3 runs of 10,000 samples each will be averaged for the final result.  

I ensured all processing uses SAMPLE_TYPE, set 100% minimum processor state in Windows' Power Options, and used this profiler config: 

* 2000 warmup cycles (where stat gathering is disabled)
* 500 logs skipped (10x more than default of 50)

## Results

Scenario 1, script-generated double-precision code performance results:

```
1. Perf Stats:  minNanos=12'200, maxNanos=142'400, nanosAvg=39'115, totalSamples=10'000
2. Perf Stats:  minNanos=12'200, maxNanos=183'700, nanosAvg=39'038, totalSamples=10'000
3. Perf Stats:  minNanos=13'800, maxNanos=176'300, nanosAvg=41'304, totalSamples=10'000
AVGS:                    12'733           167'467           39'819               10'000
```

Scenario 2, single precision processBlock() modified to copy the float AudioBuffer to a double buffer and process the audio with double-precision code:

```
1. Perf Stats:  minNanos=14'700, maxNanos=122'100, nanosAvg=45'889, totalSamples=10'000
2. Perf Stats:  minNanos=14'300, maxNanos=140'600, nanosAvg=42'122, totalSamples=10'000
3. Perf Stats:  minNanos=14'300, maxNanos=126'100, nanosAvg=44'323, totalSamples=10'000
AVGS:                    14'433           129'600           44'111               10'000
                                                           (+4'292 nanoseconds)
                                                           (+4.3 microseconds)
```

Additionally, I ran a performance test with the processing code commented out to clearly see the effect of the buffer copying.

```
1. Perf Stats:  minNanos=1'400, maxNanos=58'600, nanosAvg=4'342, totalSamples=10'000
2. Perf Stats:  minNanos=1'500, maxNanos=84'800, nanosAvg=4'682, totalSamples=10'000
3. Perf Stats:  minNanos=1'400, maxNanos=76'500, nanosAvg=4'587, totalSamples=10'000
AVGS:                    1'433           73'300           4'537               10'000
```

The performance results show that there is about a 4.5 microsecond penalty for copying single-precision audio into a double-precision buffer and back.

It is interesting to note that in the example synthesiser code, the sine wave generation uses either the `"float"` or `"double"` version of the std::sin() function, and the performance results between the `"float"` or `"double"` versions of the audio processing code, when subtracting the buffer copy penalty, are almost exactly the same (+200 nanoseconds).  So at least on my 5-year old Intel laptop processor, those different-precision functions perform roughly the same.

## Discussion

### Benefits

The main benefit of both approaches is that they enable object-oriented development techniques, simplifying designs.  Although the example audio processing code provided ("`SineWaveSynthesiser.h`") is not particularly object-oriented, its interface includes parameters of type "`AudioBuffer<SAMPLE_TYPE>`", which is now a concrete type that you can use in virtual methods, create an interface around, and implement object-oriented patterns on.  For a good object-oriented example of a synthesiser built using a single-type AudioBuffer, please see [https://github.com/invadergir/midi-synthesiser](https://github.com/invadergir/midi-synthesiser).  It also includes a solution to the arbitrary effect-ordering problem mentioned in the Problem Description.

By using O.O. software designs:

* Development will be faster compared to template interface patterns.
* Debugging will be easier.
* Run-time polymorphism is allowed, giving more flexibility.

### Drawbacks

The drawback to the buffer-copying solution are pretty clear:  a 4.5 microsecond hit to the audio thread and a bit more memory usage depending on your buffer size.

There are a few more caveats when using the auto-generated code approach:

* Before the first run of the script, no calls to double-precision code can be made.  For example, you can't populate the "`AudioBuffer<double>`" version of processBlock() until you generate the double code. (Something to keep in mind.)
* After running the script, if you continue to develop, you will gather ever-more stale code in the double-precision dir, which will cause compile problems or logic issues.  It was pretty clear that the code-generation method has a bit of a process penalty; while updating some of my code for performance testing, I noticed some discrepancies between the 'float' and 'double' versions of my code (when I forgot to update the other directory), and found myself developing a file-copy workaround instead of running the script every time.  (The script is a major slowdown in development because you have to close and reopen the IDE and Juce.)  Some possible workarounds:
    * One workaround would be to delete that dir again, and add #ifdefs around double code inside the AudioProcessor class.  Once you have your code working again, run the script and disable the ifdef.  Maybe the ifdef could be handled by the script as well.
    * Another workaround would be to add the script run as a pre-compilation step in the projucer file.  (An option could be added to the script to only run the code-generation step to enable this.)  This would only work in cases where you don't add any new files to the project, but this is a known limitation of Juce development.  If you add files, you have to close your IDE and muck around in the Projucer.  
        * (Side note: Using this script as a base, it would be easy to create a script to auto-add files to the projucer though.)
* Shared audio-processing code in other modules or libraries will have to be handled carefully.  It is likely they would need to be converted to use the same namespaces for audio processing, or at least wrapped with code that uses the SAMPLE_TYPE as the concrete floating point type.  More discovery is needed here.

### Other Considerations

* The parameters in the "`juce::AudioProcessorValueTreeState`" class are all "`float`" (single-precision) values.  This means any controls you have on your effect/instrument won't have the same resolution as the processing code.  This is probably not an issue, though; even assuming some human can hear the difference between 32-bit and 64-bit processed audio, there's no way their fingers can                           distinguish between it while turning a knob or moving a slider.  ;)  It may affect some very precise automation moves, but I can't believe this is much of an issue.
* Since the editor class is typically separate from the processing code, any samples displayed in any view components will be only a singular precision. This is also probably not a huge issue.

## Conclusion and Recommendations

Overall, I feel the benefits to developer productivity and code maintainability outweigh the stated drawbacks for either method.  However, since there is a not-insignificant development process hit to using the code-generation method, I recommend starting off a project by writing the double precision audio code with buffer copies to support lower precisions.  Development can be done on a higher-performance computer (desktop or laptop) computer with little concern for the performance impact of the buffer copies.

Then when main development wraps up and your app has been tested successfully on higher-performance hardware, if you plan to target mobile devices and top performance is a legitimate concern, you can remove your buffer-copying code and using the code-generation script to generate the "pure" single-precision code.  (A slight modification to the script would be necessary to support this.)

By following this procedure, you can maximize the benefit and minimize the drawbacks of both techniques.  It will speed up development by writing object-oriented code, sidestep the process issues on constrained devices while in the thick of development, but later in the project, provide better support for constrained devices using the same code - all the while still avoiding templating.

For existing projects, if you have a large code base already with many Juce modules and libraries, you will need to have your build procedures and target envirments carefully considered before implementing either method.  You would also need to balance the maintenance needs of existing templated software with the ease of use of the methods described here; it may or may not be a worthwhile investment.
