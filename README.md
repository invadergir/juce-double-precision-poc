# Steps to generate double-precision code:

1. Get your code working with all audio-processing code inside of the subfolder 
for single-precision ("audio_processing_float")
2. Exit the IDE and Projucer.
3. Run this script to populate the double-precision directory.  It does the following:
    1. Copy all the files from "audio_processing_float" to "audio_processing_double"
    2. Edit the file audio_processing_double/audio_processing_header.h to:
       * Update #define for `SAMPLE_TYPE` to be `double`
       * Update #define for `AUDIO_PROCESSING_NAMESPACE` to be `double`
       * Change the header guard appropriately.
    3. Edit your `*.jucer` file to add the new files (it removes all existing from that dir first).
       * Find the GROUP named "audio_processing_double", delete it
       * Find the GROUP named "audio_processing_float", copy it to "audio_processing_float".
       * Change the GROUP id to be a different UUID
       * Change the FILE ids to be new unique IDs.  (random 6-char [a-zA-Z0-9]{6} sequence, check by grepping the file again, if found, re-randomize)
       * Fix the paths to the files to be `Source/audio_processing_double/......`
       * Delete the "Builds" directory (for Windows - not sure about MacOS/Linux/etc)
4. Open the projucer file and click "Save and Open in IDE"
5. Edit your AudioProcessor class to call the double-precision audio renderer in the apropriate processBlock() function (requires an override of the parent class).
6. Viola!  You now support 64-bit audio processing with no code changes!


