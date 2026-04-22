#include "PluginProcessor.h"
#include "PluginEditor.h"

MushinAudioProcessorEditor::MushinAudioProcessorEditor (MushinAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), visualiser (1) // 1 channel
{
    setSize (600, 400);

    // Gain Slider setup
    gainSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    gainSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 20);
    addAndMakeVisible (gainSlider);
    
    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.treeState, "gain", gainSlider);

    gainLabel.setText ("Gain", juce::dontSendNotification);
    gainLabel.setJustificationType (juce::Justification::centred);
    gainLabel.attachToComponent (&gainSlider, false);
    addAndMakeVisible (gainLabel);

    // Visualiser setup
    visualiser.setRepaintRate (30);
    visualiser.setBufferSize (256);
    visualiser.setColours(juce::Colours::black, juce::Colours::cyan);
    addAndMakeVisible (visualiser);

    startTimerHz (30);
}

MushinAudioProcessorEditor::~MushinAudioProcessorEditor()
{
    stopTimer();
}

void MushinAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::darkgrey);
    
    g.setColour (juce::Colours::white);
    g.setFont (24.0f);
    g.drawFittedText ("MUSHIN", getLocalBounds().removeFromTop(40), juce::Justification::centred, 1);
}

void MushinAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop (40); // For title

    auto topArea = area.removeFromTop (100);
    gainSlider.setBounds (topArea.withSizeKeepingCentre (80, 80));

    // The rest is for the visualiser
    visualiser.setBounds (area.reduced (10));
}

void MushinAudioProcessorEditor::timerCallback()
{
    // Read from FIFO and push to visualiser
    int numReady = audioProcessor.abstractFifo.getNumReady();
    if (numReady > 0)
    {
        std::vector<float> buffer (numReady);
        int start1, block1, start2, block2;
        audioProcessor.abstractFifo.prepareToRead (numReady, start1, block1, start2, block2);

        if (block1 > 0)
        {
            for (int i = 0; i < block1; ++i)
                buffer[i] = audioProcessor.audioFifo[(size_t)(start1 + i)];
        }
        if (block2 > 0)
        {
            for (int i = 0; i < block2; ++i)
                buffer[block1 + i] = audioProcessor.audioFifo[(size_t)(start2 + i)];
        }

        audioProcessor.abstractFifo.finishedRead (block1 + block2);

        juce::AudioBuffer<float> pushBuffer (1, numReady);
        pushBuffer.copyFrom (0, 0, buffer.data(), numReady);
        visualiser.pushBuffer (pushBuffer);
    }
}
