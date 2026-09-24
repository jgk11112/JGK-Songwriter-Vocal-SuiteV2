#pragma once
#include <JuceHeader.h>
#include "../DSP/DSPModules.h"

class JGKAudioProcessor final : public juce::AudioProcessor
{
public:
    JGKAudioProcessor();
    ~JGKAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JGK_PRODUCT_NAME; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override;

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    jgk::PluginKind getKind() const { return kind; }
    juce::AudioProcessorValueTreeState& getState() { return state; }

private:
    const jgk::PluginKind kind = static_cast<jgk::PluginKind>(JGK_PLUGIN_KIND);
    juce::AudioProcessorValueTreeState state;
    std::unique_ptr<jgk::Module> module;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JGKAudioProcessor)
};
