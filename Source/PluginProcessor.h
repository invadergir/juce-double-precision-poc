/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "juce_igutil/MTLogger.h"
#include "juce_igutil/Profiler.h"

#include "audio_processing_float/SineWaveSynthesiser.h"
#include "audio_processing_double/SineWaveSynthesiser.h"

//==============================================================================
/**
*/
class DoublePrecisionPocAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    DoublePrecisionPocAudioProcessor();
    ~DoublePrecisionPocAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override;
    void processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midiMessages) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    const juce::String & getPrecisionText() const 
    { 
        return precisionText; 
    }

private:

    // profiler and logger objects
    std::shared_ptr<juce::FileLogger> pLogger;
    std::shared_ptr<juce_igutil::MTLogger> pMTL;
    std::unique_ptr<juce_igutil::Profiler> pProfiler;

    // The synths - one per processing type.  
    std::unique_ptr<audio_processing_float::SineWaveSynthesiser> pFloatSynth;
    std::unique_ptr<audio_processing_double::SineWaveSynthesiser> pDoubleSynth;

    // Buffer for testing performance with the "copy float to double buffer" 
    // scenario.
    std::unique_ptr<juce::AudioBuffer<double>> pDoubleBuffer;

    // set in the processBlock() functions, read by editor.
    juce::String & precisionText;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DoublePrecisionPocAudioProcessor)
};
