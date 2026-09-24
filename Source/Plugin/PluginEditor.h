#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "../Common/JGKLookAndFeel.h"

class JGKAudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit JGKAudioProcessorEditor(JGKAudioProcessor&);
    ~JGKAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Label label;
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    struct Combo
    {
        juce::Label label;
        juce::ComboBox box;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
    };

    void timerCallback() override;
    void drawPickLogo(juce::Graphics&, juce::Rectangle<float>);
    void drawDisplay(juce::Graphics&, juce::Rectangle<float>);

    JGKAudioProcessor& processor;
    jgk::LookAndFeel look;
    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<std::unique_ptr<Combo>> combos;
    float animation = 0.0f;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JGKAudioProcessorEditor)
};
