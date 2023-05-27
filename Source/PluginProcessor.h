/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

/*
 Roadmap
 1) figure out how to split the audio into 3 bands
 2) create parameters to control where this split happens
 3) prove that splitting into 3 bands produces no audible artifacts
 4) create audio parameters for the 3 compressor bands
 5) add two remaining compressors
 6) add ability to mute/solo/bypass individual compressors
 7) add input and output gain
 8) clean up
 */

/*
 For real TODO
 - how do we chain processes?
    - use ProcessorChain - AudioProcessGraph has a high overhead due to dynamic order
    https://forum.juce.com/t/processors-chain-or-audioprocessorgraph/37022
    https://forum.juce.com/t/advantages-of-dsp-processorchain-vs-audioprocessorgraph/51445
 - how do I get the high pass filter to work?
 - how do I set compressor makeup gain to 0?
 */

struct CompressorBand
{
    juce::AudioParameterFloat* threshold { nullptr };
    juce::AudioParameterFloat* attack { nullptr };
    juce::AudioParameterFloat* release { nullptr };
    juce::AudioParameterChoice* ratio { nullptr };
    juce::AudioParameterBool* bypassed { nullptr };
    
    void prepare(juce::dsp::ProcessSpec& spec)
    {
        compressor.prepare(spec);
    }
    
    void updateCompressorSettings()
    {
        compressor.setThreshold(threshold->get());
        compressor.setAttack(attack->get());
        compressor.setRelease(release->get());
        compressor.setRatio(ratio->getCurrentChoiceName().getFloatValue() );
//        compressor.setAttack(0.0);
//        compressor.setRelease(100.0);
//        compressor.setThreshold(-60.0);
//        compressor.setRatio(100.0);
    }
    
    void process(juce::AudioBuffer<float>& buffer)
    {
        auto block = juce::dsp::AudioBlock<float>(buffer);
        auto context = juce::dsp::ProcessContextReplacing<float>(block); // should this be non-replacing?
        
        context.isBypassed = bypassed->get();
        compressor.process(context);
    }
private:
    juce::dsp::Compressor<float> compressor;
    
};

//==============================================================================
/**
*/
class HatsOffAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    HatsOffAudioProcessor();
    ~HatsOffAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    using APVTS = juce::AudioProcessorValueTreeState;
    static APVTS::ParameterLayout createParameterLayout();
    
    APVTS apvts { *this, nullptr, "Parameters", createParameterLayout() };

private:
//    juce::dsp::Compressor<float> compressor;
    juce::AudioParameterFloat* gain { nullptr };
//    juce::AudioParameterFloat* threshold { nullptr };
//    juce::AudioParameterFloat* attack { nullptr };
//    juce::AudioParameterFloat* release { nullptr };
//    juce::AudioParameterChoice* ratio { nullptr };
//    juce::AudioParameterBool* bypassed { nullptr };
    juce::AudioParameterBool* pause { nullptr };
    
    juce::AudioParameterFloat* mix { nullptr };
    
    juce::AudioParameterFloat* freq { nullptr };
    
    CompressorBand compressor;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HatsOffAudioProcessor)
};
