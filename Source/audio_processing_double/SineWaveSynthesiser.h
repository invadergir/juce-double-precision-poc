/**
 * SineWaveSynthesiser 
 *  
 * A synth audio source that calculates the sine wave in real 
 * time.  
 */

#pragma once

#include <JuceHeader.h>
#include <math.h>

// This provides the AUDIO_PROCESSING_NAMESPACE name and the SAMPLE_TYPE #defines.
#include "audio_processing_header.h"

#define TWOPI (juce::MathConstants<SAMPLE_TYPE>::twoPi)

namespace AUDIO_PROCESSING_NAMESPACE {

/**
 * Fake synthesiser class that renders multiple sine waves regardless of midi 
 * input.  Note the lack of templatization. 
 */
class SineWaveSynthesiser
{
public:
    
    // Construct
    SineWaveSynthesiser( std::shared_ptr<juce_igutil::MTLogger> _pMTL )
    {
        // empty
    }

    // Destruct
    virtual ~SineWaveSynthesiser() = default;

    // Prepare to start playing
    void prepare(const double sampleRate) 
    {
        // just play one note, forever
        frequency = 440.0;
        currentRadians = 0.0;
        level = 0.1;
        
        SAMPLE_TYPE cyclesPerSample = frequency / sampleRate;
        radiansDelta = cyclesPerSample * TWOPI;
    }

    /**
     * Render the next block.  Expects an AudioBuffer of a specific, concrete 
     * SAMPLE_TYPE, as defined in the audio_processing_header. 
     */
    void renderNextBlock (
        juce::AudioBuffer<SAMPLE_TYPE> & outputBuffer, 
        int startSample, 
        int numSamples) 
    {
        const static SAMPLE_TYPE TWO = static_cast<SAMPLE_TYPE>(2.0);
        if (radiansDelta > 0.0)
        {
            while (--numSamples >= 0)
            {
                SAMPLE_TYPE currentSample = ( 
                    static_cast<SAMPLE_TYPE>( std::sin(currentRadians) ) + 
                    static_cast<SAMPLE_TYPE>( std::sin(currentRadians * TWO) ) 
                ) * level;

                for (int chan = 0; chan < outputBuffer.getNumChannels(); ++chan)
                    outputBuffer.addSample (chan, startSample, currentSample);

                currentRadians += radiansDelta;
                if (currentRadians > TWOPI) currentRadians -= TWOPI;
                ++startSample;
            }
        }
    }

    // Reset and clean up any resources.
    void releaseResources() 
    {
        radiansDelta = 0.0;
    }

private:

    SAMPLE_TYPE frequency = 0.0;
    SAMPLE_TYPE currentRadians = 0.0;
    SAMPLE_TYPE radiansDelta = 0.0;
    SAMPLE_TYPE level = 0.0;
};

} // AUDIO_PROCESSING_NAMESPACE
