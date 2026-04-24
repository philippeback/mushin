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
    mixParam        = treeState.getRawParameterValue ("mix");

    filterACutoffParam    = treeState.getRawParameterValue ("filter_a_cutoff");
    filterAResonanceParam = treeState.getRawParameterValue ("filter_a_resonance");
    filterADriveParam     = treeState.getRawParameterValue ("filter_a_drive");
    filterAGritParam      = treeState.getRawParameterValue ("filter_a_grit");
    filterATypeParam      = treeState.getRawParameterValue ("filter_a_type");
    filterAModeParam      = treeState.getRawParameterValue ("filter_a_mode");

    filterBCutoffParam    = treeState.getRawParameterValue ("filter_b_cutoff");
    filterBResonanceParam = treeState.getRawParameterValue ("filter_b_resonance");
    filterBDriveParam     = treeState.getRawParameterValue ("filter_b_drive");
    filterBGritParam      = treeState.getRawParameterValue ("filter_b_grit");
    filterBTypeParam      = treeState.getRawParameterValue ("filter_b_type");
    filterBModeParam      = treeState.getRawParameterValue ("filter_b_mode");

    routingParam = treeState.getRawParameterValue ("routing");

    lfo1FreqParam = treeState.getRawParameterValue ("lfo1_freq");
    lfo1WaveParam = treeState.getRawParameterValue ("lfo1_wave");
    lfo2FreqParam = treeState.getRawParameterValue ("lfo2_freq");
    lfo2WaveParam = treeState.getRawParameterValue ("lfo2_wave");
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
    dualFilterSystem.prepare(spec);

    dryBuffer.setSize(spec.numChannels, samplesPerBlock);

    smoothedGain.reset(sampleRate, 0.05);
    smoothedMix.reset(sampleRate, 0.05);

    reset();
}

void MushinAudioProcessor::releaseResources() {}

void MushinAudioProcessor::reset()
{
    waveshaper.reset();
    dualFilterSystem.reset();
}

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

    // 1. Update Parameters (Atomic to Local)
    waveshaper.setDrive (driveParam->load());
    waveshaper.setExhaustion (exhaustionParam->load() > 0.5f);
    waveshaper.setThreshold (thresholdParam->load());
    
    dualFilterSystem.baseParams.filterACutoff = filterACutoffParam->load();
    dualFilterSystem.baseParams.filterAResonance = filterAResonanceParam->load();
    dualFilterSystem.getFilterA().setDrive(filterADriveParam->load());
    dualFilterSystem.getFilterA().setGrit(filterAGritParam->load());
    dualFilterSystem.getFilterA().setType(static_cast<mushin::Filter::Type>((int)filterATypeParam->load()));
    dualFilterSystem.getFilterA().setMode(static_cast<mushin::Filter::Mode>((int)filterAModeParam->load()));

    dualFilterSystem.baseParams.filterBCutoff = filterBCutoffParam->load();
    dualFilterSystem.baseParams.filterBResonance = filterBResonanceParam->load();
    dualFilterSystem.getFilterB().setDrive(filterBDriveParam->load());
    dualFilterSystem.getFilterB().setGrit(filterBGritParam->load());
    dualFilterSystem.getFilterB().setType(static_cast<mushin::Filter::Type>((int)filterBTypeParam->load()));
    dualFilterSystem.getFilterB().setMode(static_cast<mushin::Filter::Mode>((int)filterBModeParam->load()));

    dualFilterSystem.setRouting(static_cast<mushin::DualFilterLFOMatrix::Routing>((int)routingParam->load()));

    dualFilterSystem.getLFO1().setFrequency(lfo1FreqParam->load());
    dualFilterSystem.getLFO1().setWaveform(static_cast<mushin::LFO::Waveform>((int)lfo1WaveParam->load()));
    dualFilterSystem.getLFO2().setFrequency(lfo2FreqParam->load());
    dualFilterSystem.getLFO2().setWaveform(static_cast<mushin::LFO::Waveform>((int)lfo2WaveParam->load()));

    for (int lfo = 0; lfo < 2; ++lfo) {
        for (int t = 0; t < 6; ++t) {
            juce::String paramId = "mod_lfo" + juce::String(lfo + 1) + "_target" + juce::String(t + 1);
            dualFilterSystem.setModAmount(lfo, t, treeState.getRawParameterValue(paramId)->load());
        }
    }

    smoothedGain.setTargetValue (gainParam->load());
    smoothedMix.setTargetValue (mixParam->load());

    int numSamples = buffer.getNumSamples();

    // 2. Store dry signal
    for (int ch = 0; ch < totalNumInputChannels; ++ch)
        dryBuffer.copyFrom (ch, 0, buffer, ch, 0, numSamples);

    // 3. Unified Processing Loop (Sample-by-sample)
    for (int s = 0; s < numSamples; ++s)
    {
        float mixVal = smoothedMix.getNextValue();
        float gainVal = smoothedGain.getNextValue();

        for (int ch = 0; ch < totalNumOutputChannels; ++ch)
        {
            float drySample = (ch < totalNumInputChannels) ? dryBuffer.getSample(ch, s) : 0.0f;
            float wetSample = drySample;

            // STAGE A: Distortion
            wetSample = waveshaper.processSample(ch, wetSample);

            // STAGE B: Dual Filtering & LFO Matrix
            wetSample = dualFilterSystem.processSample(ch, wetSample);

            // STAGE C: Mix
            float mixedSample = (drySample * (1.0f - mixVal)) + (wetSample * mixVal);
            
            // STAGE D: Final Gain
            float finalSample = mixedSample * gainVal;

            // NaN SAFETY GUARD
            if (std::isnan(finalSample) || std::isinf(finalSample)) {
                finalSample = 0.0f;
                reset(); // Reset engine if blowup occurs
            }

            buffer.setSample(ch, s, finalSample);

            // Push LEFT channel to UI FIFO once
            if (ch == 0) pushNextSampleIntoFifo (finalSample);
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
        juce::ParameterID { "mix", 1 }, "Dry/Wet Mix", 0.0f, 1.0f, 1.0f));

    // Filter A
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filter_a_cutoff", 1 }, "Filter A Cutoff", 
        juce::NormalisableRange<float>(20.0f, 20000.0f, 0.0f, 0.3f), 20000.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filter_a_resonance", 1 }, "Filter A Resonance", 0.0f, 1.0f, 0.1f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filter_a_drive", 1 }, "Filter A Drive", 1.0f, 10.0f, 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filter_a_grit", 1 }, "Filter A Grit", 0.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "filter_a_type", 1 }, "Filter A Type", juce::StringArray {"Clean", "Vintage", "Acid", "Digital"}, 0));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "filter_a_mode", 1 }, "Filter A Mode", juce::StringArray {"Lowpass", "Highpass", "Bandpass", "Notch"}, 0));

    // Filter B
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filter_b_cutoff", 1 }, "Filter B Cutoff", 
        juce::NormalisableRange<float>(20.0f, 20000.0f, 0.0f, 0.3f), 20000.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filter_b_resonance", 1 }, "Filter B Resonance", 0.0f, 1.0f, 0.1f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filter_b_drive", 1 }, "Filter B Drive", 1.0f, 10.0f, 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filter_b_grit", 1 }, "Filter B Grit", 0.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "filter_b_type", 1 }, "Filter B Type", juce::StringArray {"Clean", "Vintage", "Acid", "Digital"}, 0));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "filter_b_mode", 1 }, "Filter B Mode", juce::StringArray {"Lowpass", "Highpass", "Bandpass", "Notch"}, 0));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "routing", 1 }, "Routing", juce::StringArray {"Serial", "Parallel"}, 0));

    // LFOs
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "lfo1_freq", 1 }, "LFO 1 Freq", 0.1f, 50.0f, 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "lfo1_wave", 1 }, "LFO 1 Wave", juce::StringArray {"Sine", "Triangle", "Saw", "Square", "Random"}, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "lfo2_freq", 1 }, "LFO 2 Freq", 0.1f, 50.0f, 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "lfo2_wave", 1 }, "LFO 2 Wave", juce::StringArray {"Sine", "Triangle", "Saw", "Square", "Random"}, 0));

    // Matrix
    for (int lfo = 1; lfo <= 2; ++lfo) {
        for (int t = 1; t <= 6; ++t) {
            juce::String id = "mod_lfo" + juce::String(lfo) + "_target" + juce::String(t);
            juce::String name = "LFO " + juce::String(lfo) + " to Target " + juce::String(t);
            params.push_back (std::make_unique<juce::AudioParameterFloat> (
                juce::ParameterID { id, 1 }, name, -1.0f, 1.0f, 0.0f));
        }
    }
        
    return { params.begin(), params.end() };
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MushinAudioProcessor(); }
