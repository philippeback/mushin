#include "PluginProcessor.h"
#include "PluginEditor.h"

MushinAudioProcessor::MushinAudioProcessor()
    : AudioProcessor (BusesProperties()
                     .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                     .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      treeState (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    gainParam       = treeState.getRawParameterValue ("gain");
    driveParam      = treeState.getRawParameterValue ("drive");
    exhaustionParam = treeState.getRawParameterValue ("exhaustion");
    thresholdParam  = treeState.getRawParameterValue ("threshold");
    cutoffParam     = treeState.getRawParameterValue ("cutoff");
    resonanceParam  = treeState.getRawParameterValue ("resonance");
    mixParam        = treeState.getRawParameterValue ("mix");
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
    
    filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    filter.prepare(spec);

    dryBuffer.setSize(spec.numChannels, samplesPerBlock);

    smoothedGain.reset(sampleRate, 0.05);
    smoothedMix.reset(sampleRate, 0.05);
    smoothedCutoff.reset(sampleRate, 0.05);
    smoothedResonance.reset(sampleRate, 0.05);
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

    // --- ABSOLUTE BRIDGE KILL FLAG ---
    if (bridgeWorked.load()) {
        buffer.clear();
        return;
    }

    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Update DSP parameters from atomic pointers
    waveshaper.setDrive (driveParam->load());
    waveshaper.setExhaustion (exhaustionParam->load() > 0.5f);
    waveshaper.setThreshold (thresholdParam->load());
    
    smoothedCutoff.setTargetValue (cutoffParam->load());
    smoothedResonance.setTargetValue (resonanceParam->load());
    smoothedGain.setTargetValue (gainParam->load());
    smoothedMix.setTargetValue (mixParam->load());

    int numSamples = buffer.getNumSamples();

    // Store clean signal for mix - no allocation here as dryBuffer is pre-allocated
    for (int ch = 0; ch < totalNumInputChannels; ++ch)
        dryBuffer.copyFrom (ch, 0, buffer, ch, 0, numSamples);

    // 1. Distortion & Filter processing
    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);
    
    // Per-sample processing for filter to avoid steps if desired, 
    // but TPT filter doesn't have a per-sample process that takes context.
    // We update once per block for simplicity here, or we could loop.
    filter.setCutoffFrequency (smoothedCutoff.getNextValue());
    filter.setResonance (smoothedResonance.getNextValue());

    waveshaper.process (context);
    filter.process (context);

    // 2. Mix & Gain
    for (int s = 0; s < numSamples; ++s)
    {
        float mixVal = smoothedMix.getNextValue();
        float gainVal = smoothedGain.getNextValue();

        for (int ch = 0; ch < totalNumInputChannels; ++ch)
        {
            auto* wetData = buffer.getWritePointer (ch);
            auto* dryData = dryBuffer.getReadPointer (ch);
            
            // Linear Dry/Wet interpolation
            float processed = (dryData[s] * (1.0f - mixVal)) + (wetData[s] * mixVal);
            
            // Final Gain
            wetData[s] = processed * gainVal;
            
            // Push left channel to oscilloscope
            if (ch == 0) pushNextSampleIntoFifo (wetData[s]);
        }
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
juce::AudioProcessorEditor* MushinAudioProcessor::createEditor() { return new MushinAudioProcessorEditor (*this); }

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
        juce::ParameterID { "drive", 1 }, "Drive", 1.0f, 20.0f, 1.0f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "exhaustion", 1 }, "Exhaustion", false));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "threshold", 1 }, "Threshold", 0.0f, 1.0f, 1.0f));
        
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "cutoff", 1 }, "Filter Cutoff", 
        juce::NormalisableRange<float>(20.0f, 20000.0f, 0.0f, 0.3f), 20000.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "resonance", 1 }, "Filter Resonance", 0.0f, 1.0f, 0.1f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "Dry/Wet Mix", 0.0f, 1.0f, 1.0f));
        
    return { params.begin(), params.end() };
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MushinAudioProcessor(); }
