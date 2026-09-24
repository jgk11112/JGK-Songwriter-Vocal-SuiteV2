#include "PluginEditor.h"

JGKAudioProcessorEditor::JGKAudioProcessorEditor(JGKAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setLookAndFeel(&look);
    setResizable(true, true);
    setResizeLimits(760, 430, 1500, 900);
    setSize(1100, 650);

    for (auto& item : jgk::mainKnobsFor(processor.getKind()))
    {
        auto k = std::make_unique<Knob>();
        k->label.setText(item.first, juce::dontSendNotification);
        k->label.setJustificationType(juce::Justification::centred);
        k->label.setColour(juce::Label::textColourId, jgk::LookAndFeel::ink());
        k->slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        k->slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 84, 22);
        k->slider.setDoubleClickReturnValue(true, processor.getState().getParameter(item.second)->getDefaultValue());
        addAndMakeVisible(k->label);
        addAndMakeVisible(k->slider);
        k->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.getState(), item.second, k->slider);
        knobs.push_back(std::move(k));
    }

    for (auto& item : jgk::comboParamsFor(processor.getKind()))
    {
        auto c = std::make_unique<Combo>();
        c->label.setText(item.first.toUpperCase(), juce::dontSendNotification);
        c->label.setColour(juce::Label::textColourId, jgk::LookAndFeel::ink().withAlpha(0.75f));
        c->label.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
        if (auto* choice = dynamic_cast<juce::AudioParameterChoice*>(processor.getState().getParameter(item.second)))
            c->box.addItemList(choice->choices, 1);
        addAndMakeVisible(c->label);
        addAndMakeVisible(c->box);
        c->attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.getState(), item.second, c->box);
        combos.push_back(std::move(c));
    }
    startTimerHz(30);
}

JGKAudioProcessorEditor::~JGKAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void JGKAudioProcessorEditor::drawPickLogo(juce::Graphics& g, juce::Rectangle<float> r)
{
    juce::Path pick;
    auto cx = r.getCentreX();
    pick.startNewSubPath(cx, r.getBottom());
    pick.cubicTo(r.getX()-3.0f, r.getY()+r.getHeight()*0.55f, r.getX(), r.getY(), cx, r.getY());
    pick.cubicTo(r.getRight(), r.getY(), r.getRight()+3.0f, r.getY()+r.getHeight()*0.55f, cx, r.getBottom());
    g.setColour(jgk::LookAndFeel::ink());
    g.fillPath(pick);
    g.setColour(jgk::LookAndFeel::blue());
    g.setFont(juce::Font(juce::FontOptions(r.getHeight()*0.29f, juce::Font::italic)));
    g.drawText("JGK", r.reduced(4.0f), juce::Justification::centred, false);
}

void JGKAudioProcessorEditor::drawDisplay(juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour(jgk::LookAndFeel::panelDark());
    g.fillRoundedRectangle(r, 10.0f);
    g.setColour(jgk::LookAndFeel::blue().withAlpha(0.14f));
    for (int i=1;i<8;++i) g.drawVerticalLine((int)(r.getX()+r.getWidth()*i/8.0f), r.getY()+12.0f, r.getBottom()-12.0f);
    for (int i=1;i<4;++i) g.drawHorizontalLine((int)(r.getY()+r.getHeight()*i/4.0f), r.getX()+12.0f, r.getRight()-12.0f);

    juce::Path wave;
    auto mid = r.getCentreY();
    for (int x=0; x<(int)r.getWidth(); ++x)
    {
        float nx=(float)x/r.getWidth();
        float y=mid + std::sin(nx*juce::MathConstants<float>::twoPi*3.0f+animation)*r.getHeight()*0.10f
                      + std::sin(nx*juce::MathConstants<float>::twoPi*8.0f-animation*0.7f)*r.getHeight()*0.035f;
        if(x==0) wave.startNewSubPath(r.getX(),y); else wave.lineTo(r.getX()+(float)x,y);
    }
    g.setColour(jgk::LookAndFeel::blue());
    g.strokePath(wave, juce::PathStrokeType(2.0f));

    g.setColour(juce::Colours::white.withAlpha(0.82f));
    g.setFont(juce::Font(juce::FontOptions(16.0f, juce::Font::bold)));
    g.drawText(jgk::productSubtitle(processor.getKind()), r.reduced(18.0f).removeFromTop(26.0f), juce::Justification::centredLeft, false);
}

void JGKAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(jgk::LookAndFeel::cream());
    auto b = getLocalBounds().toFloat();
    auto advanced = b.removeFromBottom(getHeight()*0.28f);
    g.setColour(jgk::LookAndFeel::graphite());
    g.fillRect(advanced);
    g.setColour(jgk::LookAndFeel::ink().withAlpha(0.14f));
    g.drawHorizontalLine((int)advanced.getY(), 0.0f, (float)getWidth());

    drawPickLogo(g, {28.0f, 22.0f, 52.0f, 66.0f});
    g.setColour(jgk::LookAndFeel::ink());
    g.setFont(juce::Font(juce::FontOptions(32.0f, juce::Font::bold)));
    g.drawText(jgk::productTitle(processor.getKind()), 100, 24, 360, 42, juce::Justification::centredLeft, false);
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.drawText("SONGWRITER SERIES", 102, 62, 320, 22, juce::Justification::centredLeft, false);

    g.setColour(juce::Colour(0xffa65d3a));
    g.setFont(juce::Font(juce::FontOptions(18.0f, juce::Font::italic)));
    g.drawText("By JGK  ♡", getWidth()-190, 28, 150, 30, juce::Justification::centredRight, false);

    auto display = juce::Rectangle<float>(getWidth()*0.25f, 105.0f, getWidth()*0.58f, getHeight()*0.28f);
    drawDisplay(g, display);

    g.setColour(juce::Colours::white.withAlpha(0.88f));
    g.setFont(juce::Font(juce::FontOptions(18.0f, juce::Font::bold)));
    g.drawText("ADVANCED", advanced.reduced(24.0f).removeFromTop(30.0f), juce::Justification::centredLeft, false);
    g.setColour(juce::Colours::white.withAlpha(0.48f));
    g.setFont(juce::Font(juce::FontOptions(13.0f)));
    g.drawText("Development panel — deeper per-module controls will be promoted here as DSP is validated.",
               advanced.reduced(24.0f).withTrimmedTop(46.0f), juce::Justification::topLeft, true);
}

void JGKAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    auto top = area.removeFromTop(100);
    juce::ignoreUnused(top);
    auto lower = area.removeFromBottom((int)(getHeight()*0.28f));
    juce::ignoreUnused(lower);

    auto left = juce::Rectangle<int>(30, 112, (int)(getWidth()*0.18f), (int)(getHeight()*0.28f));
    int comboH = 54;
    for (auto& c : combos)
    {
        auto row = left.removeFromTop(comboH);
        c->label.setBounds(row.removeFromTop(18));
        c->box.setBounds(row.reduced(0,2));
        left.removeFromTop(8);
    }

    auto knobArea = juce::Rectangle<int>(55, (int)(getHeight()*0.48f), getWidth()-110, (int)(getHeight()*0.23f));
    if (!knobs.empty())
    {
        int w = knobArea.getWidth() / (int)knobs.size();
        for (auto& k : knobs)
        {
            auto cell = knobArea.removeFromLeft(w);
            k->label.setBounds(cell.removeFromBottom(26));
            k->slider.setBounds(cell.reduced(8, 0));
        }
    }
}

void JGKAudioProcessorEditor::timerCallback()
{
    animation += 0.08f;
    repaint(juce::Rectangle<int>((int)(getWidth()*0.25f), 105, (int)(getWidth()*0.58f), (int)(getHeight()*0.28f)));
}
