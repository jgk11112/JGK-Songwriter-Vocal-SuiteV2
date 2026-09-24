#include "JGKLookAndFeel.h"

namespace jgk
{
LookAndFeel::LookAndFeel()
{
    setColour(juce::Slider::textBoxTextColourId, ink());
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::backgroundColourId, cream().brighter(0.03f));
    setColour(juce::ComboBox::textColourId, ink());
    setColour(juce::ComboBox::outlineColourId, ink().withAlpha(0.18f));
    setColour(juce::PopupMenu::backgroundColourId, cream());
    setColour(juce::PopupMenu::textColourId, ink());
}

void LookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                   float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                   juce::Slider&)
{
    auto r = juce::Rectangle<float>((float) x, (float) y, (float) width, (float) height).reduced(6.0f);
    auto radius = juce::jmin(r.getWidth(), r.getHeight()) * 0.5f;
    auto centre = r.getCentre();
    auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    g.setColour(ink().withAlpha(0.14f));
    g.drawEllipse(r.reduced(2.0f), 2.0f);

    juce::Path arc;
    arc.addCentredArc(centre.x, centre.y, radius - 3.0f, radius - 3.0f, 0.0f,
                      rotaryStartAngle, angle, true);
    g.setColour(blue());
    g.strokePath(arc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    auto knob = r.reduced(radius * 0.22f);
    juce::ColourGradient grad(cream().brighter(0.12f), knob.getTopLeft(), cream().darker(0.12f), knob.getBottomRight(), false);
    g.setGradientFill(grad);
    g.fillEllipse(knob);
    g.setColour(ink().withAlpha(0.85f));
    g.drawEllipse(knob, 2.0f);

    juce::Path pointer;
    auto pointerLength = radius * 0.48f;
    auto pointerThickness = 2.2f;
    pointer.addRoundedRectangle(-pointerThickness * 0.5f, -radius * 0.58f,
                                pointerThickness, pointerLength, 1.0f);
    g.setColour(ink());
    g.fillPath(pointer, juce::AffineTransform::rotation(angle).translated(centre.x, centre.y));
}

void LookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool,
                               int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float>(0.0f, 0.0f, (float) width, (float) height).reduced(0.5f);
    g.setColour(findColour(juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle(r, 8.0f);
    g.setColour(findColour(juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle(r, 8.0f, 1.0f);

    juce::Path p;
    auto cx = (float) width - 18.0f;
    auto cy = (float) height * 0.5f;
    p.startNewSubPath(cx - 4.0f, cy - 2.0f);
    p.lineTo(cx, cy + 2.0f);
    p.lineTo(cx + 4.0f, cy - 2.0f);
    g.setColour(ink());
    g.strokePath(p, juce::PathStrokeType(1.7f));
}

juce::Font LookAndFeel::getComboBoxFont(juce::ComboBox& box)
{
    return juce::Font(juce::FontOptions((float) juce::jmin(16, box.getHeight() - 8)));
}
}
