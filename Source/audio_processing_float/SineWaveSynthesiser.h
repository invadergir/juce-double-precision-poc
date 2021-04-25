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

#include "juce_igutil/MTLogger.h"

namespace AUDIO_PROCESSING_NAMESPACE {

/**
 * Fake synthesiser class that renders multiple sine waves regardless of midi 
 * input.  Note the lack of templatization. 
 */
class SineWaveSynthesiser
{
public:
    
    // Construct
    SineWaveSynthesiser(
        std::shared_ptr<juce_igutil::MTLogger> _pMTL
    ): 
        pMTL(_pMTL)
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
        level = 0.25;
        
        double cyclesPerSample = frequency / sampleRate;
        radiansDelta = cyclesPerSample * juce::MathConstants<double>::twoPi;
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
        if (radiansDelta > 0.0)
        {
            while (--numSamples >= 0)
            {
                SAMPLE_TYPE currentSample = 
                    static_cast<SAMPLE_TYPE>( std::sin(currentRadians) ) * level;

                for (auto i = outputBuffer.getNumChannels(); --i >= 0;)
                    outputBuffer.addSample (i, startSample, currentSample);

                currentRadians += radiansDelta;
                ++startSample;
            }
        }
    }

    // Reset and clean up any resources.
    void releaseResources() 
    {
        pMTL.reset();
        radiansDelta = 0.0;
    }

private:

    float frequency = 0.0;
    double currentRadians = 0.0;
    double radiansDelta = 0.0;
    float level = 0.0;

    // logger
    std::shared_ptr<juce_igutil::MTLogger> pMTL;
};

} // AUDIO_PROCESSING_NAMESPACE

