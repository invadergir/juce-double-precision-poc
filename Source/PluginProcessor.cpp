/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <JuceHeader.h>

using namespace juce;
using namespace juce_igutil;
using namespace std;

juce::String emptyText("");
juce::String singlePrecisionText("single");
juce::String doublePrecisionText("double");

// Uncomment one or both of these to get special behavior for profiling tests:

// Copy to double buffer, process in double, copy back to single buffer:
//#define PROFILING_SINGLE_TO_DOUBLE

// Define this to disable rendering during above test, to measure effect of the buffer copying.
//#define DISABLE_RENDER

//==============================================================================
DoublePrecisionPocAudioProcessor::DoublePrecisionPocAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),

    pLogger(std::shared_ptr<juce::FileLogger>(
        FileLogger::createDefaultAppLogger(
            "juce-double-precision-poc", 
            "juce-double-precision-poc.txt", 
            "Processor started."))),
    pMTL(std::make_shared<MTLogger>(pLogger)),
    precisionText(emptyText)
#endif
{
    // set up logger and profiler
    Logger::setCurrentLogger(pLogger.get());
    pLogger->logMessage("Audio Processor CONSTRUCTOR.");
    const int numWarmupCycles = 2000;
    pProfiler.reset(new Profiler(
        "DoublePrecisionPocAudioProcessor_Profiler", pMTL, numWarmupCycles, 500));

    // create synths
    pFloatSynth = make_unique<audio_processing_float::SineWaveSynthesiser>(pMTL);
    pDoubleSynth = make_unique<audio_processing_double::SineWaveSynthesiser>(pMTL);

    pLogger->logMessage("Constructor done.");
}

DoublePrecisionPocAudioProcessor::~DoublePrecisionPocAudioProcessor()
{
    Logger::setCurrentLogger(nullptr);
}

//==============================================================================
const juce::String DoublePrecisionPocAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool DoublePrecisionPocAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool DoublePrecisionPocAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool DoublePrecisionPocAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double DoublePrecisionPocAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int DoublePrecisionPocAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int DoublePrecisionPocAudioProcessor::getCurrentProgram()
{
    return 0;
}

void DoublePrecisionPocAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String DoublePrecisionPocAudioProcessor::getProgramName (int index)
{
    return {};
}

void DoublePrecisionPocAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void DoublePrecisionPocAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    pFloatSynth->prepare(sampleRate);
    pDoubleSynth->prepare(sampleRate);

    const int numChannels = getTotalNumOutputChannels();
    const int numSamples = samplesPerBlock * 2;

    if ( !pDoubleBuffer ) {
        pMTL->debug(String("PREPARE:  allocating double buffer, using twice the requested size to make sure we have enough:  numSamples = ") + String(numSamples));
        pDoubleBuffer.reset(new AudioBuffer<double>(numChannels, numSamples));
    }
    else {
        pMTL->debug(String("PREPARE:  RESIZING double buffer, using twice the requested size to make sure we have enough:  numSamples = ") + String(numSamples));
        pDoubleBuffer->setSize(numChannels, numSamples, true, false, true);
    }
}

void DoublePrecisionPocAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
    pFloatSynth->releaseResources();
    pDoubleSynth->releaseResources();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool DoublePrecisionPocAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

// Float-size render:
void DoublePrecisionPocAudioProcessor::processBlock(
    juce::AudioBuffer<float> & buffer, 
    juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    precisionText = singlePrecisionText;

    static bool gotHere = false;
    if ( !gotHere ) {
        pMTL->debug("Rendering in single-precision mode...");
        gotHere = true;
    }

    // check for bypass
    if ( !getBypassParameter() ) {
        //pProfiler->start();

        const auto numChannel = getTotalNumOutputChannels();

#ifdef PROFILING_SINGLE_TO_DOUBLE
        // copy to double buffer, process in double, copy back to single buffer
        // Copy.  Note makeCopyOf does a setSize() already.  
        pDoubleBuffer->makeCopyOf(buffer, true);

        // render - can be disabled to specifically test effect of copying:
        #ifndef DISABLE_RENDER
        pDoubleSynth->renderNextBlock(*pDoubleBuffer, 0, buffer.getNumSamples());
        #endif

        // copy back to single buffer
        buffer.makeCopyOf(*pDoubleBuffer, true);

#else // normal
        pFloatSynth->renderNextBlock(buffer, 0, buffer.getNumSamples());
#endif

        //pProfiler->stop();
    }
}

// Double-size render.  I don't have a VST host that can run & test this.
void DoublePrecisionPocAudioProcessor::processBlock(
    juce::AudioBuffer<double> & buffer, 
    juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    precisionText = doublePrecisionText;

    static bool gotHere = false;
    if ( !gotHere ) {
        pMTL->debug("Rendering in double-precision mode...");
        gotHere = true;
    }

    // check for bypass
    if ( !getBypassParameter() ) {
        pProfiler->start();

        const auto numChannels = getTotalNumOutputChannels();

        pDoubleSynth->renderNextBlock(buffer, 0, buffer.getNumSamples());

        pProfiler->stop();
    }
}

//==============================================================================
bool DoublePrecisionPocAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* DoublePrecisionPocAudioProcessor::createEditor()
{
    return new DoublePrecisionPocAudioProcessorEditor (*this);
}

//==============================================================================
void DoublePrecisionPocAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void DoublePrecisionPocAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DoublePrecisionPocAudioProcessor();
}
