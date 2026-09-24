#pragma once
#include <JuceHeader.h>

namespace jgk
{
class LookAndFeel final : public juce::LookAndFeel_V4
{
public:
    LookAndFeel();
    void drawRotarySlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    void drawComboBox(juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;

    static juce::Colour cream()      { return juce::Colour(0xfff2eadf); }
    static juce::Colour graphite()   { return juce::Colour(0xff11161b); }
    static juce::Colour panelDark()  { return juce::Colour(0xff0b1116); }
    static juce::Colour blue()       { return juce::Colour(0xff67cfff); }
    static juce::Colour ink()        { return juce::Colour(0xff1d252b); }
    static juce::Colour muted()      { return juce::Colour(0xff8a837c); }
};
}
