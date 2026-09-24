#include "PluginProcessor.h"
#include "PluginEditor.h"

JGKAudioProcessor::JGKAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      state(*this, nullptr, "JGK_STATE", jgk::createParameterLayout(kind)),
      module(jgk::createModule(kind, state))
{
}

void JGKAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock,
                                 (juce::uint32) juce::jmax(1, getTotalNumOutputChannels()) };
    module->prepare(spec);
}

void JGKAudioProcessor::releaseResources()
{
    module->reset();
}

bool JGKAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    auto out = layouts.getMainOutputChannelSet();
    auto in = layouts.getMainInputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    return in == out;
}

void JGKAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());
    module->process(buffer, midi);
}

juce::AudioProcessorEditor* JGKAudioProcessor::createEditor()
{
    return new JGKAudioProcessorEditor(*this);
}

double JGKAudioProcessor::getTailLengthSeconds() const
{
    if (kind == jgk::PluginKind::space) return 10.0;
    if (kind == jgk::PluginKind::echo) return 4.0;
    if (kind == jgk::PluginKind::vocalChain) return 4.0;
    return 0.0;
}

void JGKAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto xml = state.copyState().createXml();
    copyXmlToBinary(*xml, destData);
}

void JGKAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(state.state.getType()))
            state.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new JGKAudioProcessor();
}
