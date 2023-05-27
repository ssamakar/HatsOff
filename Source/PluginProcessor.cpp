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
    compressor.threshold = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("Threshold"));
    jassert(compressor.threshold != nullptr);

    compressor.attack = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("Attack"));
    jassert(compressor.attack != nullptr);

    compressor.release = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("Release"));
    jassert(compressor.release != nullptr);
    
    compressor.ratio = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("Ratio"));
    jassert(compressor.ratio != nullptr);

    compressor.bypassed = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter("Bypassed"));
    jassert(compressor.bypassed != nullptr);

    gain = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("Gain"));
    jassert(gain != nullptr);
    
    pause = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter("Pause"));
    jassert(pause != nullptr);
    
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
    spec.numChannels = getTotalNumOutputChannels();
    spec.sampleRate = sampleRate;
    
    compressor.prepare(spec);
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
    
    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    auto dryWetMix = juce::jmap(mix->get(), 0.0f, 100.0f, 0.0f, 1.0f);
    
    juce::dsp::AudioBlock<float> audioBlock {buffer};
    
    constexpr auto PI = 3.14159265359f;
    std::vector<float> dnBuffer;
    dnBuffer.resize(buffer.getNumChannels(), 0.f);
    const auto sign = -1.f;

    auto cutoff = freq->get();
    
    const auto tan = std::tan(PI * cutoff / 48000);
    const auto a1 = (tan - 1.f) / (tan + 1.f);
    
    
    compressor.updateCompressorSettings();
    compressor.process(buffer);
    
    for (auto channel = 0; channel < audioBlock.getNumChannels(); channel++)
    {
        auto* data = audioBlock.getChannelPointer(channel);
        
        for (auto sample = 0; sample < audioBlock.getNumSamples(); sample++)
        {
            auto input = data[sample];
            float output = input * -1.0; // flip polarity
            
            // compress the signal here

            
            const auto allPassFilteredSample = a1 * output + dnBuffer[channel];
            dnBuffer[channel] = output - a1 * allPassFilteredSample;
            
            const auto filterOutput = 0.5f * (output + sign * allPassFilteredSample);
            
            data[sample] = (1.0 - dryWetMix) * input + dryWetMix * filterOutput;
        }
    }
    
    auto dbGain = gain->get();
    auto rawGain = juce::Decibels::decibelsToGain(dbGain);
    
    auto paused = pause->get();
    
    
    
    
////     This is the place where you'd normally do the guts of your plugin's
////     audio processing...
////     Make sure to reset the state if your inner loop is processing
////     the samples and the outer loop is handling the channels.
////     Alternatively, you can process the samples with the channels
////     interleaved by keeping the same state.
//    for (int channel = 0; channel < totalNumInputChannels; ++channel)
//    {
//        auto* channelData = buffer.getWritePointer (channel);
//
//        for(int sample = 0; sample < block.getNumSamples(); ++sample)
//        {
////            channelData[sample] *= rawGain * -1.0;
////            channelData[sample] = channelData[sample] * rawGain * -1.0;
//
//            if (rawGain < 1)
//            {
//                channelData[sample] *= -1.0;
//            }
//
//            if (paused)
//            {
//                channelData[sample] = 1;
//            }
//        }
//    }
    
//    compressor.updateCompressorSettings();
//    compressor.process(buffer);
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
    
    layout.add(std::make_unique<AudioParameterFloat>(ParameterID {"Threshold", 1},
                                                     "Threshold",
                                                     NormalisableRange<float>(-60, 12, 1, 1),
                                                     0));
    
    auto attackReleaseRange = NormalisableRange<float>(0, 500, 1, 1);
    
    layout.add(std::make_unique<AudioParameterFloat>(ParameterID {"Attack", 1},
                                                     "Attack",
                                                     attackReleaseRange,
                                                     50));
    
    layout.add(std::make_unique<AudioParameterFloat>(ParameterID {"Release", 1},
                                                     "Release",
                                                     attackReleaseRange,
                                                     250));
    
    auto choices = std::vector<double>{ 1, 1.5, 2, 3, 4, 5, 6, 7, 8, 10, 15, 20, 50, 100 };
    
    juce::StringArray sa;
    for (auto choice : choices)
    {
        sa.add( juce::String(choice, 1) );
    }
    
    layout.add(std::make_unique<AudioParameterChoice>(ParameterID {"Ratio", 1},
                                                      "Ratio",
                                                      sa,
                                                      3));
    
    layout.add(std::make_unique<AudioParameterBool>(ParameterID {"Bypassed", 1}, "Bypassed", false));
    
    
    layout.add(std::make_unique<AudioParameterFloat>(ParameterID {"Gain", 1},
                                                     "Gain",
                                                     NormalisableRange<float>(-60, 12, 1, 1),
                                                     1));
    
    layout.add(std::make_unique<AudioParameterBool>(ParameterID {"Pause", 1}, "Pause", false));
    
    layout.add(std::make_unique<AudioParameterFloat>(ParameterID {"Mix", 1},
                                                     "Mix",
                                                     NormalisableRange<float>(0, 100, 1, 1),
                                                     50));
    
    layout.add(std::make_unique<AudioParameterFloat>(ParameterID {"Freq", 1},
                                                     "Freq",
                                                     NormalisableRange<float>(0, 20000, 1, 1),
                                                     50));
    
    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HatsOffAudioProcessor();
}
