#include "PluginProcessor.h"
#include "PluginEditor.h"

MushinAudioProcessor::MushinAudioProcessor()
    : AudioProcessor (BusesProperties()
                     .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                     .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      treeState (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

MushinAudioProcessor::~MushinAudioProcessor() {}

const juce::String MushinAudioProcessor::getName() const { return "Mushin"; }
bool MushinAudioProcessor::acceptsMidi() const { return false; }
bool MushinAudioProcessor::producesMidi() const { return false; }
bool MushinAudioProcessor::isMidiEffect() const { return false; }
double MushinAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int MushinAudioProcessor::getNumPrograms() { return 1; }
int MushinAudioProcessor::getCurrentProgram() { return 0; }
void MushinAudioProcessor::setCurrentProgram (int) {}
const juce::String MushinAudioProcessor::getProgramName (int) { return {}; }
void MushinAudioProcessor::changeProgramName (int, const juce::String&) {}

void MushinAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    abstractFifo.reset();

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels = (juce::uint32) getTotalNumOutputChannels();

    waveshaper.prepare(spec);
}

void MushinAudioProcessor::releaseResources() {}

bool MushinAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void MushinAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Update parameters
    waveshaper.setDrive(treeState.getRawParameterValue("drive")->load());
    waveshaper.setExhaustion(treeState.getRawParameterValue("exhaustion")->load() > 0.5f);
    waveshaper.setThreshold(treeState.getRawParameterValue("threshold")->load());

    // Process through waveshaper
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    waveshaper.process(context);

    // Apply gain
    float gain = treeState.getRawParameterValue("gain")->load();

    auto* channelData = buffer.getWritePointer (0); // Using left channel for visualization
    
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        // Apply gain across all channels
        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            buffer.getWritePointer(channel)[sample] *= gain;
        }

        // Push to FIFO (mono mix of first channel)
        pushNextSampleIntoFifo (channelData[sample]);
    }
}

void MushinAudioProcessor::pushNextSampleIntoFifo (float sample) noexcept
{
    if (abstractFifo.getFreeSpace() > 0)
    {
        int start1, block1, start2, block2;
        abstractFifo.prepareToWrite (1, start1, block1, start2, block2);
        
        if (block1 > 0) audioFifo[(size_t) start1] = sample;
        if (block2 > 0) audioFifo[(size_t) start2] = sample;
        
        abstractFifo.finishedWrite (block1 + block2);
    }
}

bool MushinAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* MushinAudioProcessor::createEditor()
{
    return new MushinAudioProcessorEditor (*this);
}

void MushinAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = treeState.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void MushinAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (treeState.state.getType()))
            treeState.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessorValueTreeState::ParameterLayout MushinAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "gain", 1 }, "Gain", 0.0f, 2.0f, 1.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "drive", 1 }, "Drive", 1.0f, 10.0f, 1.0f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "exhaustion", 1 }, "Exhaustion", false));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "threshold", 1 }, "Threshold", 0.0f, 1.0f, 1.0f));
        
    return { params.begin(), params.end() };
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MushinAudioProcessor();
}
