#include "PluginProcessor.h"
#include "PluginEditor.h"

MushinAudioProcessor::MushinAudioProcessor()
    : AudioProcessor (BusesProperties()
                     .withInput  ("Input",     juce::AudioChannelSet::stereo(), true)
                     .withOutput ("Output",    juce::AudioChannelSet::stereo(), true)
                     .withInput  ("Sidechain", juce::AudioChannelSet::stereo(), false)),
      treeState (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    auto logFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("PhilippeBack")
        .getChildFile("Mushin")
        .getChildFile("Mushin_Log.txt");
    
    if (!logFile.getParentDirectory().exists())
        logFile.getParentDirectory().createDirectory();

    juce::Logger::setCurrentLogger (new juce::FileLogger (logFile, "Mushin Log Started", 0));
    juce::Logger::writeToLog("--- Mushin Processor Initialized ---");

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

    scActiveParam    = treeState.getRawParameterValue ("sc_active");
    scSourceParam    = treeState.getRawParameterValue ("sc_source");
    scThresholdParam = treeState.getRawParameterValue ("sc_threshold");
    scAttackParam    = treeState.getRawParameterValue ("sc_attack");
    scReleaseParam   = treeState.getRawParameterValue ("sc_release");
    scModeParam      = treeState.getRawParameterValue ("sc_mode");
    scAmountParam    = treeState.getRawParameterValue ("sc_amount");
    scTargetParam    = treeState.getRawParameterValue ("sc_target");
    scHpFreqParam    = treeState.getRawParameterValue ("sc_hp_freq");
    scLpFreqParam    = treeState.getRawParameterValue ("sc_lp_freq");

    noiseActiveParam  = treeState.getRawParameterValue ("noise_active");
    noiseTypeParam    = treeState.getRawParameterValue ("noise_type");
    noiseFreqParam    = treeState.getRawParameterValue ("noise_freq");
    noiseLevelParam   = treeState.getRawParameterValue ("noise_level");
    noiseRoutingParam = treeState.getRawParameterValue ("noise_routing");
    noiseFmModParam   = treeState.getRawParameterValue ("noise_fm_mod");
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
    sidechainProcessor.prepare(spec);
    noiseOscillator.prepare(spec);

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
    sidechainProcessor.reset();
    noiseOscillator.reset();
}

bool MushinAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    // Sidechain input can be mono or stereo (or disabled)
    auto scInput = layouts.getChannelSet (true, 1);
    if (scInput != juce::AudioChannelSet::disabled() && 
        scInput != juce::AudioChannelSet::mono() && 
        scInput != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void MushinAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear unused output channels
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

    // Update Sidechain Parameters
    sidechainProcessor.setActive(scActiveParam->load() > 0.5f);
    sidechainProcessor.setSource(static_cast<mushin::SidechainProcessor::Source>((int)scSourceParam->load()));
    sidechainProcessor.setTarget(static_cast<mushin::SidechainProcessor::Target>((int)scTargetParam->load()));
    sidechainProcessor.setParameters(
        scAttackParam->load(),
        scReleaseParam->load(),
        (int)scModeParam->load() == 1, // RMS
        scThresholdParam->load(),
        scAmountParam->load() * 0.01f, // UI is 0-100%, processor expects 0-1.0
        scHpFreqParam->load(),
        scLpFreqParam->load()
    );

    smoothedGain.setTargetValue (gainParam->load());
    smoothedMix.setTargetValue (mixParam->load());

    int numSamples = buffer.getNumSamples();

    // 2. Store dry signal - CLONE strictly from Main Input Bus
    dryBuffer.clear();
    auto mainBusBuffer = getBusBuffer(buffer, true, 0);
    for (int ch = 0; ch < std::min(mainBusBuffer.getNumChannels(), dryBuffer.getNumChannels()); ++ch) {
        dryBuffer.copyFrom(ch, 0, mainBusBuffer.getReadPointer(ch), numSamples);
    }

    // Get sidechain buffer if external and enabled
    juce::AudioBuffer<float> sidechainBuffer;
    bool scBusEnabled = false;
    if (auto* scBus = getBus (true, 1)) {
        if (scBus->isEnabled()) {
            sidechainBuffer = getBusBuffer (buffer, true, 1);
            scBusEnabled = true;
        }
    }

    float blockMaxScInput = 0.0f;

    // 3. Unified Processing Loop (Sample-by-sample)
    for (int s = 0; s < numSamples; ++s)
    {
        float mixVal = smoothedMix.getNextValue();
        float gainVal = smoothedGain.getNextValue();

        // Calculate Noise Generator sample and FM modulation offset for this sample
        float genSample = 0.0f;
        bool noiseActive = (noiseActiveParam != nullptr && noiseActiveParam->load() > 0.5f);
        bool testSignalForSidechain = (sidechainProcessor.isActive() && 
                                       sidechainProcessor.getSource() == mushin::SidechainProcessor::Source::TestSignal);
        
        if (noiseActive || testSignalForSidechain) {
            noiseOscillator.setType((int)noiseTypeParam->load());
            noiseOscillator.setFrequency(noiseFreqParam->load());
            genSample = noiseOscillator.nextSample() * noiseLevelParam->load();
        }

        float fmAmount = (noiseFmModParam != nullptr) ? noiseFmModParam->load() : 0.0f;
        float currentFmMod = genSample * fmAmount;

        // Calculate Sidechain modulation value for this sample
        float scMod = 0.0f;
        if (sidechainProcessor.isActive()) {
            float scInputSample = 0.0f;
            if (sidechainProcessor.getSource() == mushin::SidechainProcessor::Source::External && scBusEnabled) {
                // Mix external stereo SC to mono
                if (sidechainBuffer.getNumChannels() > 0) {
                    scInputSample = sidechainBuffer.getSample(0, s);
                    if (sidechainBuffer.getNumChannels() > 1) {
                        scInputSample = (scInputSample + sidechainBuffer.getSample(1, s)) * 0.5f;
                    }
                }
            } else if (sidechainProcessor.getSource() == mushin::SidechainProcessor::Source::TestSignal) {
                // Test signal source
                scInputSample = genSample;
            } else {
                // Internal SC uses dry main input
                scInputSample = dryBuffer.getSample(0, s);
                if (dryBuffer.getNumChannels() > 1) {
                    scInputSample = (scInputSample + dryBuffer.getSample(1, s)) * 0.5f;
                }
            }
            
            blockMaxScInput = std::max(blockMaxScInput, std::abs(scInputSample));
            scMod = sidechainProcessor.processSample(scInputSample);
        }

        for (int ch = 0; ch < totalNumOutputChannels; ++ch)
        {
            float drySample = dryBuffer.getSample(ch, s);
            float wetSample = drySample;

            // STAGE A: Distortion (Pre-Dist Injection)
            if (noiseActive && (int)noiseRoutingParam->load() == 0) {
                wetSample += genSample;
            }

            float currentDrive = driveParam->load();
            if (sidechainProcessor.isActive() && sidechainProcessor.getTarget() == mushin::SidechainProcessor::Target::Drive) {
                currentDrive += (scMod * 20.0f); 
            }
            waveshaper.setDrive(std::clamp(currentDrive, 1.0f, 50.0f));
            wetSample = waveshaper.processSample(ch, wetSample);

            // STAGE B: Dual Filtering & LFO Matrix (Pre-Filter Injection)
            if (noiseActive && (int)noiseRoutingParam->load() == 1) {
                wetSample += genSample;
            }

            if (sidechainProcessor.isActive() && sidechainProcessor.getTarget() == mushin::SidechainProcessor::Target::Cutoff) {
                float cutoffOffset = std::pow(2.0f, scMod * 2.0f);
                dualFilterSystem.baseParams.filterACutoff = filterACutoffParam->load() * cutoffOffset;
                dualFilterSystem.baseParams.filterBCutoff = filterBCutoffParam->load() * cutoffOffset;
            } else {
                dualFilterSystem.baseParams.filterACutoff = filterACutoffParam->load();
                dualFilterSystem.baseParams.filterBCutoff = filterBCutoffParam->load();
            }
            
            wetSample = dualFilterSystem.processSample(ch, wetSample, currentFmMod);

            // STAGE C: Post-Filter Injection
            if (noiseActive && (int)noiseRoutingParam->load() == 2) {
                wetSample += genSample;
            }

            // STAGE D: Mix
            float mixedSample = (drySample * (1.0f - mixVal)) + (wetSample * mixVal);
            
            // STAGE E: Final Gain
            float currentGain = gainVal;
            if (sidechainProcessor.isActive() && sidechainProcessor.getTarget() == mushin::SidechainProcessor::Target::Gain) {
                currentGain += scMod;
            }
            float finalSample = mixedSample * std::max(0.0f, currentGain);

            // NaN SAFETY GUARD
            if (std::isnan(finalSample) || std::isinf(finalSample)) {
                finalSample = 0.0f;
                reset(); 
            }

            buffer.setSample(ch, s, finalSample);
            if (ch == 0) pushNextSampleIntoFifo (finalSample);
        }

        if (s == numSamples - 1) {
            scMeterLevel.store(scMod);
            scInputPeak.store(blockMaxScInput);
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
juce::AudioProcessorEditor* MushinAudioProcessor::createEditor() { 
    juce::Logger::writeToLog("Host called createEditor(). Message Thread: " + juce::String((int)juce::MessageManager::getInstance()->isThisTheMessageThread()));
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
        juce::ParameterID { "lfo1_freq", 1 }, "LFO 1 Freq", 
        juce::NormalisableRange<float>(0.1f, 20.0f, 0.0f, 0.4f), 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "lfo1_wave", 1 }, "LFO 1 Wave", juce::StringArray {"Sine", "Triangle", "Saw", "Square", "Random"}, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "lfo2_freq", 1 }, "LFO 2 Freq", 
        juce::NormalisableRange<float>(0.1f, 20.0f, 0.0f, 0.4f), 1.0f));
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

    // Sidechain
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "sc_active", 1 }, "SC Active", false));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "sc_source", 1 }, "SC Source", juce::StringArray {"Internal", "External", "Test Signal"}, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "sc_threshold", 1 }, "SC Threshold", -60.0f, 0.0f, -24.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "sc_attack", 1 }, "SC Attack", 0.1f, 500.0f, 10.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "sc_release", 1 }, "SC Release", 1.0f, 2000.0f, 100.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "sc_mode", 1 }, "SC Mode", juce::StringArray {"Peak", "RMS"}, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "sc_amount", 1 }, "SC Amount", -100.0f, 100.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "sc_target", 1 }, "SC Target", juce::StringArray {"Drive", "Cutoff", "Gain"}, 2));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "sc_hp_freq", 1 }, "SC HPF", 
        juce::NormalisableRange<float>(20.0f, 2000.0f, 0.0f, 0.3f), 20.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "sc_lp_freq", 1 }, "SC LPF", 
        juce::NormalisableRange<float>(500.0f, 20000.0f, 0.0f, 0.3f), 20000.0f));

    // Noise Oscillator
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "noise_active", 1 }, "Noise Active", false));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "noise_type", 1 }, "Noise Type", 
        juce::StringArray {"White Noise", "Pink Noise", "Sine Tone", "Triangle Wave", "Sawtooth Wave", "Square Wave"}, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "noise_freq", 1 }, "Noise Freq", 
        juce::NormalisableRange<float>(20.0f, 20000.0f, 0.0f, 0.3f), 440.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "noise_level", 1 }, "Noise Level", 0.0f, 1.0f, 0.1f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "noise_routing", 1 }, "Noise Routing", 
        juce::StringArray {"Pre-Dist", "Pre-Filter", "Post-Filter"}, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "noise_fm_mod", 1 }, "Filter FM Mod", 0.0f, 1.0f, 0.0f));
        
    return { params.begin(), params.end() };
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MushinAudioProcessor(); }
