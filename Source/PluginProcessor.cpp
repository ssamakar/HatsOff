/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
HatsOffAudioProcessor::HatsOffAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    release = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("Release"));
    jassert(release != nullptr);
 
    mix = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("Mix"));
    jassert(mix != nullptr);
    
    freq = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("Freq"));
    jassert(freq != nullptr);
}

HatsOffAudioProcessor::~HatsOffAudioProcessor()
{
}

//==============================================================================
const juce::String HatsOffAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool HatsOffAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool HatsOffAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool HatsOffAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double HatsOffAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int HatsOffAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int HatsOffAudioProcessor::getCurrentProgram()
{
    return 0;
}

void HatsOffAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String HatsOffAudioProcessor::getProgramName (int index)
{
    return {};
}

void HatsOffAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void HatsOffAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.sampleRate = getSampleRate();
    spec.numChannels = getTotalNumOutputChannels();
    _highpassModule.prepare(spec);
    _highpassModule.setType(juce::dsp::LinkwitzRileyFilterType::highpass);
    _highpassModule.setCutoffFrequency(freq->get());
    _gainModule.prepare(spec);
}

void HatsOffAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool HatsOffAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void HatsOffAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    auto dryWetMix = juce::jmap(mix->get(), 0.0f, 100.0f, 0.0f, 1.0f);

    
    juce::dsp::AudioBlock<float> audioBlock {buffer};
    
    std::vector<float> dnBuffer;
    dnBuffer.resize(buffer.getNumChannels(), 0.f);
    
    _highpassModule.setCutoffFrequency(freq->get());
    
    for (auto channel = 0; channel < audioBlock.getNumChannels(); channel++)
    {
        auto* data = audioBlock.getChannelPointer(channel);
        
        for (auto sample = 0; sample < audioBlock.getNumSamples(); sample++)
        {
            auto input = data[sample];
            
            // flip polarity
            float output = input * -1.0;
            
            // compress hard
            output = compressSample(output);
            
            // high pass
            output = _highpassModule.processSample(channel, output);

            // blend
            output = (1.0 - dryWetMix) * input + dryWetMix * output;
            
            // unity gain
            // auto inputGain = juce::Decibels::
            _gainModule.setGainDecibels(input);
            output = _gainModule.processSample(output);
            
            data[sample] = output;
        }
    }
}

float HatsOffAudioProcessor::compressSample (float input) {
    auto releaseTime = release->get();
    auto sampleRate = getSampleRate();
    auto alphaRelease = std::exp((std::log(9) * -1.0) / (sampleRate * (releaseTime / 1000)));

    const auto x = input;

    auto x_Uni = abs(x);
    auto x_dB = juce::Decibels::gainToDecibels(x_Uni);

    if (x_dB < -96.0)
    {
        x_dB = -96.0;
    }

    if (x_dB > thresh )
    {
        gainSC = thresh + (x_dB - thresh ) / ratio ;
    }

    else
    {
        gainSC = x_dB;
    }

    gainChange_dB = gainSC - x_dB;

    if (gainChange_dB < gainSmoothPrevious)
    {
        gainSmooth = gainChange_dB;
        currentSignal = gainSmooth;
    }

    else
    {
        gainSmooth = ((1 - alphaRelease) * gainChange_dB) + (alphaRelease * gainSmoothPrevious);
        currentSignal = gainSmooth;
    }

    gainSmoothPrevious = gainSmooth;

    auto compressedSample = x * juce::Decibels::decibelsToGain(gainSmooth);
    return compressedSample;
}

//==============================================================================
bool HatsOffAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* HatsOffAudioProcessor::createEditor()
{
//    return new HatsOffAudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void HatsOffAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void HatsOffAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if ( tree.isValid() )
    {
        apvts.replaceState(tree);
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout HatsOffAudioProcessor::createParameterLayout()
{
    APVTS::ParameterLayout layout;
    
    using namespace juce;
    
    layout.add(std::make_unique<AudioParameterBool>(ParameterID {"Bypassed", 1}, "Bypassed", false));
    
    auto releaseRange = NormalisableRange<float>(5, 500, 1, 1);
    
    layout.add(std::make_unique<AudioParameterFloat>(ParameterID {"Release", 1},
                                                     "Release",
                                                     releaseRange,
                                                     250));

    layout.add(std::make_unique<AudioParameterFloat>(ParameterID {"Mix", 1},
                                                     "Mix",
                                                     NormalisableRange<float>(0, 100, 1, 1),
                                                     50));
    
    layout.add(std::make_unique<AudioParameterFloat>(ParameterID {"Freq", 1},
                                                     "LPF (hz)",
                                                     NormalisableRange<float>(20, 20000, 1, .2),
                                                     50));
    
    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HatsOffAudioProcessor();
}
