/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

//==============================================================================
DoublePrecisionPocAudioProcessorEditor::DoublePrecisionPocAudioProcessorEditor (DoublePrecisionPocAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (400, 300);

    // Let it run a bit and then get the precision to display.
    Timer::callAfterDelay(500, [this]() {
        precisionText = 
            String("Current audio-processing precision is:  ") + 
            audioProcessor.getPrecisionText();
    });
}

DoublePrecisionPocAudioProcessorEditor::~DoublePrecisionPocAudioProcessorEditor()
{ 
    // empty
}

//==============================================================================
void DoublePrecisionPocAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText(precisionText, getLocalBounds(), juce::Justification::centred, 1);
}

void DoublePrecisionPocAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}
