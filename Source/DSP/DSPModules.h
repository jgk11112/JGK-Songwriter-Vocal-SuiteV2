#pragma once
#include <JuceHeader.h>
#include <memory>
#include <vector>

namespace jgk
{
enum class PluginKind : int
{
    tune = 1, eq, comp, deEss, saturation, space, echo, limiter, vocalChain
};

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout(PluginKind kind);
std::vector<std::pair<juce::String, juce::String>> mainKnobsFor(PluginKind kind);
std::vector<std::pair<juce::String, juce::String>> comboParamsFor(PluginKind kind);
juce::String productTitle(PluginKind kind);
juce::String productSubtitle(PluginKind kind);

class Module
{
public:
    virtual ~Module() = default;
    virtual void prepare(const juce::dsp::ProcessSpec&) = 0;
    virtual void reset() = 0;
    virtual void process(juce::AudioBuffer<float>&, juce::MidiBuffer&) = 0;
};

std::unique_ptr<Module> createModule(PluginKind kind, juce::AudioProcessorValueTreeState& state);
}
