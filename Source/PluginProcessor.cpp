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
        scAmountParam->load(),
        scHpFreqParam->load(),
        scLpFreqParam->load()
    );

    smoothedGain.setTargetValue (gainParam->load());
    smoothedMix.setTargetValue (mixParam->load());

    int numSamples = buffer.getNumSamples();

    // 2. Store dry signal
    dryBuffer.clear();
    auto mainInputBuffer = getBusBuffer(buffer, true, 0);
    int channelsToCopy = std::min(mainInputBuffer.getNumChannels(), dryBuffer.getNumChannels());
    for (int ch = 0; ch < channelsToCopy; ++ch) {
        dryBuffer.copyFrom (ch, 0, mainInputBuffer, ch, 0, numSamples);
    }

    // Get sidechain buffer if external and enabled
    juce::AudioBuffer<float> sidechainBuffer;
    if (auto* scBus = getBus (true, 1)) {
        if (scBus->isEnabled()) {
            sidechainBuffer = getBusBuffer (buffer, true, 1);
        }
    }

    // 3. Unified Processing Loop (Sample-by-sample)
    for (int s = 0; s < numSamples; ++s)
    {
        float mixVal = smoothedMix.getNextValue();
        float gainVal = smoothedGain.getNextValue();

        // Calculate Sidechain modulation value for this sample
        float scMod = 0.0f;
        if (sidechainProcessor.isActive()) {
            float scInputSample = 0.0f;
            if (sidechainProcessor.getSource() == mushin::SidechainProcessor::Source::External) {
                // Mix external stereo SC to mono
                if (sidechainBuffer.getNumChannels() > 0) {
                    scInputSample = sidechainBuffer.getSample(0, s);
                    if (sidechainBuffer.getNumChannels() > 1) {
                        scInputSample = (scInputSample + sidechainBuffer.getSample(1, s)) * 0.5f;
                    }
                }
            } else {
                // Internal SC uses pre-distortion main input (mixed to mono)
                scInputSample = dryBuffer.getSample(0, s);
                if (dryBuffer.getNumChannels() > 1) {
                    scInputSample = (scInputSample + dryBuffer.getSample(1, s)) * 0.5f;
                }
            }
            scMod = sidechainProcessor.processSample(scInputSample);
        }

        for (int ch = 0; ch < totalNumOutputChannels; ++ch)
        {
            float drySample = dryBuffer.getSample(ch, s);
            float wetSample = drySample;

            // STAGE A: Distortion
            float currentDrive = driveParam->load();
            if (sidechainProcessor.isActive() && sidechainProcessor.getTarget() == mushin::SidechainProcessor::Target::Drive) {
                currentDrive *= (1.0f + scMod);
            }
            waveshaper.setDrive(std::clamp(currentDrive, 1.0f, 50.0f));
            wetSample = waveshaper.processSample(ch, wetSample);

            // STAGE B: Dual Filtering & LFO Matrix
            // If SC targets Cutoff, we apply it as an offset to the base params
            if (sidechainProcessor.isActive() && sidechainProcessor.getTarget() == mushin::SidechainProcessor::Target::Cutoff) {
                // We'll apply it to both filters for now as a 1-octave shift per 1.0 modulation
                float cutoffOffset = std::pow(2.0f, scMod);
                dualFilterSystem.baseParams.filterACutoff = filterACutoffParam->load() * cutoffOffset;
                dualFilterSystem.baseParams.filterBCutoff = filterBCutoffParam->load() * cutoffOffset;
            } else {
                dualFilterSystem.baseParams.filterACutoff = filterACutoffParam->load();
                dualFilterSystem.baseParams.filterBCutoff = filterBCutoffParam->load();
            }
            
            wetSample = dualFilterSystem.processSample(ch, wetSample);

            // STAGE C: Mix
            float mixedSample = (drySample * (1.0f - mixVal)) + (wetSample * mixVal);
            
            // STAGE D: Final Gain
            float currentGain = gainVal;
            if (sidechainProcessor.isActive() && sidechainProcessor.getTarget() == mushin::SidechainProcessor::Target::Gain) {
                currentGain *= (1.0f + scMod);
            }
            float finalSample = mixedSample * std::max(0.0f, currentGain);

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

    // Sidechain
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "sc_active", 1 }, "SC Active", false));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "sc_source", 1 }, "SC Source", juce::StringArray {"Internal", "External"}, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "sc_threshold", 1 }, "SC Threshold", -60.0f, 0.0f, -24.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "sc_attack", 1 }, "SC Attack", 0.1f, 500.0f, 10.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "sc_release", 1 }, "SC Release", 1.0f, 2000.0f, 100.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "sc_mode", 1 }, "SC Mode", juce::StringArray {"Peak", "RMS"}, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "sc_amount", 1 }, "SC Amount", -1.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "sc_target", 1 }, "SC Target", juce::StringArray {"Drive", "Cutoff", "Gain"}, 2));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "sc_hp_freq", 1 }, "SC HPF", 
        juce::NormalisableRange<float>(20.0f, 2000.0f, 0.0f, 0.3f), 20.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "sc_lp_freq", 1 }, "SC LPF", 
        juce::NormalisableRange<float>(500.0f, 20000.0f, 0.0f, 0.3f), 20000.0f));
        
    return { params.begin(), params.end() };
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MushinAudioProcessor(); }
