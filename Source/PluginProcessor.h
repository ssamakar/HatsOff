/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
//
//struct CompressorBand
//{
//    juce::AudioParameterFloat* release { nullptr };
//    juce::AudioParameterBool* bypassed { nullptr };
//
//    void prepare(juce::dsp::ProcessSpec& spec)
//    {
//        compressor.prepare(spec);
//    }
//
//    void updateCompressorSettings()
//    {
//        compressor.setRelease(release->get());
//    }
//
//    void process(juce::AudioBuffer<float>& buffer)
//    {
//        auto block = juce::dsp::AudioBlock<float>(buffer);
//        auto context = juce::dsp::ProcessContextReplacing<float>(block); // should this be non-replacing?
//
//        context.isBypassed = bypassed->get();
//        compressor.process(context);
//    }
//private:
//    juce::dsp::Compressor<float> compressor;
//
//};

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
    float compressSample (float sample);
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
    juce::AudioParameterFloat* mix { nullptr };
    
    juce::AudioParameterFloat* freq { nullptr };
    juce::AudioParameterFloat* release { nullptr };

    juce::SmoothedValue<float> _mix;

    float thresh = -50.0f;
    float ratio = 30.0f;
    float gainSC = 0.0f;
    float gainSmooth = 0.0f;
    float gainSmoothPrevious = 0.0f;
    float currentSignal = 0.0f;
    float gainChange_dB = 0.0f;
    float sampleRate = getSampleRate();
    float pi = 3.14159265359f;
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HatsOffAudioProcessor)
};
