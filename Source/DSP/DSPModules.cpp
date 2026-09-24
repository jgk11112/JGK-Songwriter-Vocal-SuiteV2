#include "DSPModules.h"
#include <cmath>
#include <array>

namespace jgk
{
namespace
{
using APVTS = juce::AudioProcessorValueTreeState;

float value(APVTS& s, const char* id)
{
    if (auto* v = s.getRawParameterValue(id)) return v->load();
    return 0.0f;
}

std::unique_ptr<juce::RangedAudioParameter> fparam(const char* id, const char* name,
                                                    float min, float max, float def, float step = 0.001f)
{
    return std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id, 1}, name,
        juce::NormalisableRange<float>(min, max, step), def);
}

std::unique_ptr<juce::RangedAudioParameter> logParam(const char* id, const char* name,
                                                      float min, float max, float def)
{
    juce::NormalisableRange<float> r(min, max);
    r.setSkewForCentre(def);
    return std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id, 1}, name, r, def);
}

std::unique_ptr<juce::RangedAudioParameter> choice(const char* id, const char* name,
                                                   std::initializer_list<const char*> values, int def = 0)
{
    juce::StringArray a;
    for (auto* v : values) a.add(v);
    return std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{id, 1}, name, a, def);
}

struct PitchDetector
{
    void prepare(double sr)
    {
        sampleRate = sr;
        history.assign(4096, 0.0f);
        write = 0;
        filled = 0;
    }

    void push(const float* data, int n)
    {
        for (int i = 0; i < n; ++i)
        {
            history[(size_t) write] = data[i];
            write = (write + 1) % (int) history.size();
            filled = juce::jmin((int) history.size(), filled + 1);
        }
    }

    float detect() const
    {
        if (filled < 1024) return 0.0f;
        constexpr int N = 2048;
        std::array<float, N> x{};
        int start = write - N;
        while (start < 0) start += (int) history.size();
        float mean = 0.0f;
        for (int i = 0; i < N; ++i)
        {
            x[(size_t)i] = history[(size_t)((start + i) % (int)history.size())];
            mean += x[(size_t)i];
        }
        mean /= (float) N;
        float rms = 0.0f;
        for (auto& v : x) { v -= mean; rms += v * v; }
        rms = std::sqrt(rms / (float) N);
        if (rms < 0.003f) return 0.0f;

        int minLag = juce::jmax(20, (int) (sampleRate / 1000.0));
        int maxLag = juce::jmin(N / 2 - 1, (int) (sampleRate / 65.0));
        float best = 0.0f;
        int bestLag = 0;
        for (int lag = minLag; lag <= maxLag; ++lag)
        {
            double corr = 0.0, e1 = 0.0, e2 = 0.0;
            int count = N - lag;
            for (int i = 0; i < count; ++i)
            {
                auto a = x[(size_t)i];
                auto b = x[(size_t)(i + lag)];
                corr += a * b; e1 += a * a; e2 += b * b;
            }
            auto denom = std::sqrt(e1 * e2) + 1.0e-12;
            auto c = (float)(corr / denom);
            if (c > best) { best = c; bestLag = lag; }
        }
        if (best < 0.55f || bestLag == 0) return 0.0f;
        return (float)(sampleRate / (double) bestLag);
    }

    double sampleRate = 44100.0;
    std::vector<float> history;
    int write = 0, filled = 0;
};

class DualDelayPitchShifter
{
public:
    void prepare(double sr, int channels)
    {
        sampleRate = sr;
        size = 16384;
        buffers.assign((size_t) channels, std::vector<float>((size_t) size, 0.0f));
        write = 0;
        phase = 0.25f;
        ratioSmoothed = 1.0f;
    }

    void reset()
    {
        for (auto& b : buffers) std::fill(b.begin(), b.end(), 0.0f);
        write = 0; phase = 0.25f; ratioSmoothed = 1.0f;
    }

    void setRatio(float r, float smooth)
    {
        targetRatio = juce::jlimit(0.75f, 1.333333f, r);
        smoothing = juce::jlimit(0.0002f, 0.08f, smooth);
    }

    void process(juce::AudioBuffer<float>& audio)
    {
        auto channels = audio.getNumChannels();
        auto samples = audio.getNumSamples();
        const float window = (float) juce::jlimit(256, 2048, (int)(sampleRate * 0.018));
        const float baseDelay = window + 16.0f;

        for (int i = 0; i < samples; ++i)
        {
            ratioSmoothed += smoothing * (targetRatio - ratioSmoothed);
            auto phaseStep = (1.0f - ratioSmoothed) / window;
            auto pa = phase;
            auto pb = std::fmod(phase + 0.5f, 1.0f);
            auto wa = 0.5f - 0.5f * std::cos(juce::MathConstants<float>::twoPi * pa);
            auto wb = 0.5f - 0.5f * std::cos(juce::MathConstants<float>::twoPi * pb);

            for (int ch = 0; ch < channels; ++ch)
            {
                auto* d = audio.getWritePointer(ch);
                buffers[(size_t) ch][(size_t) write] = d[i];
                auto ya = read(ch, baseDelay + pa * window);
                auto yb = read(ch, baseDelay + pb * window);
                d[i] = ya * wa + yb * wb;
            }

            write = (write + 1) % size;
            phase += phaseStep;
            while (phase < 0.0f) phase += 1.0f;
            while (phase >= 1.0f) phase -= 1.0f;
        }
    }

private:
    float read(int ch, float delay) const
    {
        float pos = (float) write - delay;
        while (pos < 0.0f) pos += (float) size;
        int i0 = (int) pos;
        int i1 = (i0 + 1) % size;
        float frac = pos - (float) i0;
        const auto& b = buffers[(size_t) ch];
        return b[(size_t)i0] + frac * (b[(size_t)i1] - b[(size_t)i0]);
    }

    double sampleRate = 44100.0;
    int size = 0, write = 0;
    float phase = 0.0f, targetRatio = 1.0f, ratioSmoothed = 1.0f, smoothing = 0.01f;
    std::vector<std::vector<float>> buffers;
};

class TuneModule final : public Module
{
public:
    explicit TuneModule(APVTS& s) : state(s) {}
    void prepare(const juce::dsp::ProcessSpec& spec) override
    {
        sr = spec.sampleRate;
        detector.prepare(sr);
        shifter.prepare(sr, (int)spec.numChannels);
        wet.setSize((int)spec.numChannels, (int)spec.maximumBlockSize);
        vibratoPhase = 0.0;
    }
    void reset() override { shifter.reset(); }
    void process(juce::AudioBuffer<float>& b, juce::MidiBuffer&) override
    {
        if (b.getNumSamples() <= 0 || b.getNumChannels() <= 0) return;
        detector.push(b.getReadPointer(0), b.getNumSamples());
        auto freq = detector.detect();
        float ratio = 1.0f;
        if (freq > 0.0f)
        {
            double midi = 69.0 + 12.0 * std::log2((double)freq / 440.0);
            int key = (int) value(state, "key");
            int scale = (int) value(state, "scale");
            int mode = (int) value(state, "mode");
            int nearest = nearestAllowed((int) std::lround(midi), key, scale);
            double amount = mode == 0 ? 0.55 : (mode == 1 ? 0.8 : 1.0);
            auto vib = value(state, "vibrato");
            amount *= juce::jlimit(0.45, 1.3, 1.0 + (0.5 - (double)vib) * 0.7);
            auto corrected = midi + (double)value(state, "retune") * amount * ((double)nearest - midi);
            auto extraDepthCents = juce::jmax(0.0f, vib - 0.5f) * 40.0f;
            auto lfo = std::sin(vibratoPhase) * extraDepthCents;
            corrected += lfo / 100.0;
            ratio = (float) std::pow(2.0, (corrected - midi) / 12.0);
        }
        vibratoPhase += juce::MathConstants<double>::twoPi * 5.5 * (double)b.getNumSamples() / sr;
        vibratoPhase = std::fmod(vibratoPhase, juce::MathConstants<double>::twoPi);

        wet.makeCopyOf(b, true);
        auto smooth = value(state, "smooth");
        shifter.setRatio(ratio, juce::jmap(smooth, 0.002f, 0.035f));
        shifter.process(wet);
        auto mix = value(state, "mix");
        for (int ch = 0; ch < b.getNumChannels(); ++ch)
            for (int i = 0; i < b.getNumSamples(); ++i)
                b.setSample(ch, i, b.getSample(ch, i) * (1.0f - mix) + wet.getSample(ch, i) * mix);
    }
private:
    static int nearestAllowed(int midi, int key, int scale)
    {
        if (scale == 2) return midi;
        static constexpr int major[] = {0,2,4,5,7,9,11};
        static constexpr int minor[] = {0,2,3,5,7,8,10};
        auto allowed = scale == 0 ? major : minor;
        int count = 7, best = midi, bestDist = 99;
        for (int d = -12; d <= 12; ++d)
        {
            int note = midi + d;
            int pc = (note - key) % 12; if (pc < 0) pc += 12;
            for (int i = 0; i < count; ++i)
                if (pc == allowed[i] && std::abs(d) < bestDist) { best = note; bestDist = std::abs(d); }
        }
        return best;
    }

    APVTS& state;
    double sr = 44100.0, vibratoPhase = 0.0;
    PitchDetector detector;
    DualDelayPitchShifter shifter;
    juce::AudioBuffer<float> wet;
};

class EQModule final : public Module
{
public:
    explicit EQModule(APVTS& s) : state(s) {}
    void prepare(const juce::dsp::ProcessSpec& sp) override { spec = sp; hp.prepare(sp); body.prepare(sp); presence.prepare(sp); air.prepare(sp); }
    void reset() override { hp.reset(); body.reset(); presence.reset(); air.reset(); }
    void process(juce::AudioBuffer<float>& b, juce::MidiBuffer&) override
    {
        *hp.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(spec.sampleRate, value(state,"lowCut"), 0.7071f);
        *body.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(spec.sampleRate, 190.0, 0.75f, juce::Decibels::decibelsToGain(value(state,"body")));
        *presence.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(spec.sampleRate, 3200.0, 0.85f, juce::Decibels::decibelsToGain(value(state,"presence")));
        *air.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(spec.sampleRate, 10500.0, 0.7f, juce::Decibels::decibelsToGain(value(state,"air")));
        juce::dsp::AudioBlock<float> block(b); juce::dsp::ProcessContextReplacing<float> c(block);
        hp.process(c); body.process(c); presence.process(c); air.process(c);
        b.applyGain(juce::Decibels::decibelsToGain(value(state,"output")));
    }
private:
    APVTS& state; juce::dsp::ProcessSpec spec{};
    using F = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>;
    F hp, body, presence, air;
};

class CompModule final : public Module
{
public:
    explicit CompModule(APVTS& s) : state(s) {}
    void prepare(const juce::dsp::ProcessSpec& sp) override { comp.prepare(sp); wet.setSize((int)sp.numChannels,(int)sp.maximumBlockSize); }
    void reset() override { comp.reset(); }
    void process(juce::AudioBuffer<float>& b, juce::MidiBuffer&) override
    {
        wet.makeCopyOf(b, true);
        auto amount = value(state,"amount"); auto glue = value(state,"glue");
        comp.setThreshold(juce::jmap(amount, 0.0f,1.0f,-6.0f,-30.0f));
        comp.setRatio(juce::jmap(glue, 0.0f,1.0f,2.0f,6.0f));
        comp.setAttack(juce::jmap(glue, 0.0f,1.0f,30.0f,5.0f));
        comp.setRelease(juce::jmap(glue, 0.0f,1.0f,180.0f,60.0f));
        juce::dsp::AudioBlock<float> block(wet); juce::dsp::ProcessContextReplacing<float> c(block); comp.process(c);
        auto mix = value(state,"mix");
        auto makeup = juce::Decibels::decibelsToGain(value(state,"output"));
        for (int ch=0; ch<b.getNumChannels(); ++ch)
            for (int i=0;i<b.getNumSamples();++i)
                b.setSample(ch,i,(b.getSample(ch,i)*(1.0f-mix)+wet.getSample(ch,i)*mix)*makeup);
    }
private: APVTS& state; juce::dsp::Compressor<float> comp; juce::AudioBuffer<float> wet;
};

class DeEssModule final : public Module
{
public:
    explicit DeEssModule(APVTS& s) : state(s) {}
    void prepare(const juce::dsp::ProcessSpec& sp) override { spec=sp; detector.prepare(sp); env=0.0f; }
    void reset() override { detector.reset(); env=0.0f; }
    void process(juce::AudioBuffer<float>& b, juce::MidiBuffer&) override
    {
        float focus=value(state,"focus"); float hz=juce::jmap(focus,0.0f,1.0f,4500.0f,9500.0f);
        *detector.state=*juce::dsp::IIR::Coefficients<float>::makeHighPass(spec.sampleRate,hz,0.707f);
        scratch.makeCopyOf(b,true); juce::dsp::AudioBlock<float> block(scratch); juce::dsp::ProcessContextReplacing<float> c(block); detector.process(c);
        float sensitivity=value(state,"sensitivity"); float depth=value(state,"deess"); float mix=value(state,"mix");
        float threshold=juce::Decibels::decibelsToGain(juce::jmap(sensitivity,0.0f,1.0f,-10.0f,-38.0f));
        for(int i=0;i<b.getNumSamples();++i)
        {
            float d=0.0f; for(int ch=0;ch<scratch.getNumChannels();++ch) d=juce::jmax(d,std::abs(scratch.getSample(ch,i)));
            env = d>env ? 0.35f*d+0.65f*env : 0.015f*d+0.985f*env;
            float over=env>threshold ? juce::jlimit(0.0f,1.0f,(env-threshold)/(threshold+1.0e-6f)) : 0.0f;
            float reduction=1.0f-over*depth*0.8f; float g=1.0f-mix+mix*reduction;
            for(int ch=0;ch<b.getNumChannels();++ch) b.setSample(ch,i,b.getSample(ch,i)*g);
        }
    }
private: APVTS& state; juce::dsp::ProcessSpec spec{}; using F=juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,juce::dsp::IIR::Coefficients<float>>; F detector; juce::AudioBuffer<float> scratch; float env=0.0f;
};

class SatModule final : public Module
{
public:
    explicit SatModule(APVTS& s):state(s){}
    void prepare(const juce::dsp::ProcessSpec& sp) override { wet.setSize((int)sp.numChannels,(int)sp.maximumBlockSize); }
    void reset() override {}
    void process(juce::AudioBuffer<float>& b, juce::MidiBuffer&) override
    {
        wet.makeCopyOf(b,true); auto drive=value(state,"drive"); auto warmth=value(state,"warmth"); auto texture=value(state,"texture"); auto mix=value(state,"mix");
        float pregain=juce::jmap(drive,1.0f,10.0f); float asym=juce::jmap(warmth,-0.15f,0.15f); float hard=juce::jmap(texture,0.8f,1.8f);
        for(int ch=0;ch<wet.getNumChannels();++ch) for(int i=0;i<wet.getNumSamples();++i){ float x=wet.getSample(ch,i)*pregain+asym; float y=std::tanh(x*hard)/std::tanh(pregain*hard); wet.setSample(ch,i,y-asym*0.2f); }
        for(int ch=0;ch<b.getNumChannels();++ch) for(int i=0;i<b.getNumSamples();++i) b.setSample(ch,i,b.getSample(ch,i)*(1.0f-mix)+wet.getSample(ch,i)*mix);
    }
private: APVTS& state; juce::AudioBuffer<float> wet;
};

class SpaceModule final : public Module
{
public:
    explicit SpaceModule(APVTS& s):state(s){}
    void prepare(const juce::dsp::ProcessSpec& sp) override { reverb.prepare(sp); }
    void reset() override { reverb.reset(); }
    void process(juce::AudioBuffer<float>& b, juce::MidiBuffer&) override
    {
        juce::dsp::Reverb::Parameters p; p.roomSize=value(state,"space"); p.width=value(state,"width"); p.damping=1.0f-value(state,"tone"); p.wetLevel=value(state,"mix"); p.dryLevel=1.0f-p.wetLevel; p.freezeMode=0.0f; reverb.setParameters(p);
        juce::dsp::AudioBlock<float> block(b); juce::dsp::ProcessContextReplacing<float> c(block); reverb.process(c);
    }
private: APVTS& state; juce::dsp::Reverb reverb;
};

class EchoModule final : public Module
{
public:
    explicit EchoModule(APVTS& s):state(s){}
    void prepare(const juce::dsp::ProcessSpec& sp) override { sr=sp.sampleRate; int n=(int)(sr*3.0); lines.assign((size_t)sp.numChannels,std::vector<float>((size_t)n,0.0f)); pos=0; toneState.assign((size_t)sp.numChannels,0.0f); }
    void reset() override { for(auto& l:lines)std::fill(l.begin(),l.end(),0.0f); std::fill(toneState.begin(),toneState.end(),0.0f);pos=0; }
    void process(juce::AudioBuffer<float>& b, juce::MidiBuffer&) override
    {
        if(lines.empty())return; int len=(int)lines[0].size(); int delay=juce::jlimit(1,len-2,(int)(value(state,"echoMs")*0.001f*(float)sr)); float fb=value(state,"feedback"); float mix=value(state,"mix"); float tone=value(state,"tone"); float a=juce::jmap(tone,0.02f,0.35f);
        for(int i=0;i<b.getNumSamples();++i){ int read=pos-delay; if(read<0)read+=len; for(int ch=0;ch<b.getNumChannels();++ch){ float x=b.getSample(ch,i); float d=lines[(size_t)ch][(size_t)read]; toneState[(size_t)ch]+=a*(d-toneState[(size_t)ch]); float filtered=toneState[(size_t)ch]; lines[(size_t)ch][(size_t)pos]=x+filtered*fb; b.setSample(ch,i,x*(1.0f-mix)+filtered*mix);} pos=(pos+1)%len; }
    }
private: APVTS& state; double sr=44100.0; int pos=0; std::vector<std::vector<float>> lines; std::vector<float> toneState;
};

class LimiterModule final : public Module
{
public:
    explicit LimiterModule(APVTS& s):state(s){}
    void prepare(const juce::dsp::ProcessSpec& sp) override { limiter.prepare(sp); wet.setSize((int)sp.numChannels,(int)sp.maximumBlockSize); }
    void reset() override { limiter.reset(); }
    void process(juce::AudioBuffer<float>& b, juce::MidiBuffer&) override
    {
        wet.makeCopyOf(b,true); wet.applyGain(juce::Decibels::decibelsToGain(value(state,"loudness"))); limiter.setThreshold(value(state,"ceiling")); limiter.setRelease(juce::jmap(value(state,"character"),15.0f,180.0f)); juce::dsp::AudioBlock<float> block(wet); juce::dsp::ProcessContextReplacing<float> c(block); limiter.process(c); float mix=value(state,"mix"); for(int ch=0;ch<b.getNumChannels();++ch)for(int i=0;i<b.getNumSamples();++i)b.setSample(ch,i,b.getSample(ch,i)*(1.0f-mix)+wet.getSample(ch,i)*mix);
    }
private: APVTS& state; juce::dsp::Limiter<float> limiter; juce::AudioBuffer<float> wet;
};

class ChainModule final : public Module
{
public:
    explicit ChainModule(APVTS& s):state(s){}
    void prepare(const juce::dsp::ProcessSpec& sp) override { spec=sp; hp.prepare(sp); comp.prepare(sp); reverb.prepare(sp); limiter.prepare(sp); }
    void reset() override { hp.reset();comp.reset();reverb.reset();limiter.reset(); }
    void process(juce::AudioBuffer<float>& b, juce::MidiBuffer&) override
    {
        b.applyGain(juce::Decibels::decibelsToGain(value(state,"input")));
        *hp.state=*juce::dsp::IIR::Coefficients<float>::makeHighPass(spec.sampleRate,75.0f,0.707f); juce::dsp::AudioBlock<float> bl(b);juce::dsp::ProcessContextReplacing<float> c(bl);hp.process(c);
        auto presence=value(state,"presence"); auto tone=value(state,"tone"); b.applyGain(juce::Decibels::decibelsToGain(presence*2.0f + tone));
        comp.setThreshold(-18.0f);comp.setRatio(3.0f);comp.setAttack(12.0f);comp.setRelease(100.0f);comp.process(c);
        float sat=0.9f+0.8f*presence; for(int ch=0;ch<b.getNumChannels();++ch)for(int i=0;i<b.getNumSamples();++i)b.setSample(ch,i,std::tanh(b.getSample(ch,i)*sat)/std::tanh(sat));
        juce::dsp::Reverb::Parameters p; p.roomSize=0.35f;p.damping=0.55f;p.width=0.85f;p.wetLevel=value(state,"space")*0.35f;p.dryLevel=1.0f;p.freezeMode=0.0f;reverb.setParameters(p);reverb.process(c);
        b.applyGain(juce::Decibels::decibelsToGain(value(state,"loudness"))); limiter.setThreshold(-1.0f);limiter.setRelease(80.0f);limiter.process(c);
    }
private: APVTS& state;juce::dsp::ProcessSpec spec{};using F=juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,juce::dsp::IIR::Coefficients<float>>;F hp;juce::dsp::Compressor<float> comp;juce::dsp::Reverb reverb;juce::dsp::Limiter<float> limiter;
};
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout(PluginKind kind)
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    switch(kind)
    {
        case PluginKind::tune:
            p.push_back(choice("mode","Mode",{"Natural","Balanced","More"})); p.push_back(choice("key","Key",{"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"})); p.push_back(choice("scale","Scale",{"Major","Minor","Chromatic"})); p.push_back(fparam("retune","Retune",0,1,0.7f)); p.push_back(fparam("vibrato","Vibrato",0,1,0.5f)); p.push_back(fparam("smooth","Smooth",0,1,0.55f)); p.push_back(fparam("mix","Mix",0,1,1.0f)); break;
        case PluginKind::eq:
            p.push_back(choice("mode","Mode",{"Natural","Clean","Modern","More"})); p.push_back(logParam("lowCut","Low Cut",20,500,80)); p.push_back(fparam("body","Body",-12,12,1.5f,0.1f)); p.push_back(fparam("presence","Presence",-12,12,1.0f,0.1f)); p.push_back(fparam("air","Air",-12,12,2.0f,0.1f)); p.push_back(fparam("output","Output",-12,12,0,0.1f)); break;
        case PluginKind::comp:
            p.push_back(choice("mode","Mode",{"Natural","Light","Firm"})); p.push_back(fparam("amount","Amount",0,1,0.45f)); p.push_back(fparam("glue","Glue",0,1,0.5f)); p.push_back(fparam("output","Output",-12,12,2.0f,0.1f)); p.push_back(fparam("mix","Mix",0,1,1)); break;
        case PluginKind::deEss:
            p.push_back(choice("mode","Mode",{"Natural","Wide","Focused"})); p.push_back(fparam("sensitivity","Sensitivity",0,1,0.5f)); p.push_back(fparam("deess","De-Ess",0,1,0.45f)); p.push_back(fparam("focus","Focus",0,1,0.5f)); p.push_back(fparam("mix","Mix",0,1,1)); break;
        case PluginKind::saturation:
            p.push_back(choice("mode","Mode",{"Smooth","Tape","Modern"})); p.push_back(fparam("warmth","Warmth",0,1,0.4f)); p.push_back(fparam("texture","Texture",0,1,0.35f)); p.push_back(fparam("drive","Drive",0,1,0.25f)); p.push_back(fparam("mix","Mix",0,1,0.7f)); break;
        case PluginKind::space:
            p.push_back(choice("mode","Mode",{"Room","Hall","Plate","Live"})); p.push_back(fparam("space","Space",0,1,0.35f)); p.push_back(fparam("width","Width",0,1,0.85f)); p.push_back(fparam("tone","Tone",0,1,0.5f)); p.push_back(fparam("mix","Mix",0,1,0.2f)); break;
        case PluginKind::echo:
            p.push_back(choice("mode","Mode",{"Quarter","Eighth","Dotted","Tape"})); p.push_back(logParam("echoMs","Echo",20,2000,420)); p.push_back(fparam("feedback","Feedback",0,0.95f,0.25f)); p.push_back(fparam("tone","Tone",0,1,0.55f)); p.push_back(fparam("mix","Mix",0,1,0.18f)); break;
        case PluginKind::limiter:
            p.push_back(choice("mode","Mode",{"Natural","Punchy","Loud"})); p.push_back(fparam("ceiling","Ceiling",-3,0,-1,0.1f)); p.push_back(fparam("loudness","Loudness",0,12,4,0.1f)); p.push_back(fparam("character","Character",0,1,0.4f)); p.push_back(fparam("mix","Mix",0,1,1)); break;
        case PluginKind::vocalChain:
            p.push_back(choice("mode","Preset",{"Singer-Songwriter (Natural)","Warm & Intimate","Pop Clean","Big & Airy"})); p.push_back(fparam("input","Input",-12,12,0,0.1f)); p.push_back(fparam("tone","Tone",-6,6,0,0.1f)); p.push_back(fparam("presence","Presence",0,1,0.45f)); p.push_back(fparam("space","Space",0,1,0.18f)); p.push_back(fparam("loudness","Loudness",0,10,3.0f,0.1f)); break;
    }
    return {p.begin(),p.end()};
}

std::vector<std::pair<juce::String, juce::String>> mainKnobsFor(PluginKind k)
{
    switch(k){
        case PluginKind::tune:return {{"Retune","retune"},{"Vibrato","vibrato"},{"Smooth","smooth"},{"Mix","mix"}};
        case PluginKind::eq:return {{"Low Cut","lowCut"},{"Body","body"},{"Presence","presence"},{"Air","air"},{"Output","output"}};
        case PluginKind::comp:return {{"Amount","amount"},{"Glue","glue"},{"Output","output"},{"Mix","mix"}};
        case PluginKind::deEss:return {{"Sensitivity","sensitivity"},{"De-Ess","deess"},{"Focus","focus"},{"Mix","mix"}};
        case PluginKind::saturation:return {{"Warmth","warmth"},{"Texture","texture"},{"Drive","drive"},{"Mix","mix"}};
        case PluginKind::space:return {{"Space","space"},{"Width","width"},{"Tone","tone"},{"Mix","mix"}};
        case PluginKind::echo:return {{"Echo","echoMs"},{"Feedback","feedback"},{"Tone","tone"},{"Mix","mix"}};
        case PluginKind::limiter:return {{"Ceiling","ceiling"},{"Loudness","loudness"},{"Character","character"},{"Mix","mix"}};
        case PluginKind::vocalChain:return {{"Input","input"},{"Tone","tone"},{"Presence","presence"},{"Space","space"},{"Loudness","loudness"}};
    } return {};
}

std::vector<std::pair<juce::String, juce::String>> comboParamsFor(PluginKind k)
{
    if(k==PluginKind::tune) return {{"Mode","mode"},{"Key","key"},{"Scale","scale"}};
    return {{k==PluginKind::vocalChain?"Preset":"Mode","mode"}};
}

juce::String productTitle(PluginKind k)
{
    switch(k){case PluginKind::tune:return"TUNE";case PluginKind::eq:return"EQ";case PluginKind::comp:return"COMP";case PluginKind::deEss:return"DE-ESS";case PluginKind::saturation:return"SAT";case PluginKind::space:return"SPACE";case PluginKind::echo:return"ECHO";case PluginKind::limiter:return"LIMITER";case PluginKind::vocalChain:return"VOCAL CHAIN";}return{};
}
juce::String productSubtitle(PluginKind k)
{
    switch(k){case PluginKind::tune:return"VOCAL PITCH CORRECTION";case PluginKind::eq:return"VOCAL EQ";case PluginKind::comp:return"VOCAL COMPRESSOR";case PluginKind::deEss:return"TAME SIBILANCE";case PluginKind::saturation:return"VOCAL SATURATION";case PluginKind::space:return"VOCAL REVERB";case PluginKind::echo:return"VOCAL DELAY";case PluginKind::limiter:return"FINAL LOUDNESS";case PluginKind::vocalChain:return"COMPLETE VOCAL CHAIN";}return{};
}

std::unique_ptr<Module> createModule(PluginKind kind, APVTS& s)
{
    switch(kind){case PluginKind::tune:return std::make_unique<TuneModule>(s);case PluginKind::eq:return std::make_unique<EQModule>(s);case PluginKind::comp:return std::make_unique<CompModule>(s);case PluginKind::deEss:return std::make_unique<DeEssModule>(s);case PluginKind::saturation:return std::make_unique<SatModule>(s);case PluginKind::space:return std::make_unique<SpaceModule>(s);case PluginKind::echo:return std::make_unique<EchoModule>(s);case PluginKind::limiter:return std::make_unique<LimiterModule>(s);case PluginKind::vocalChain:return std::make_unique<ChainModule>(s);}return{};
}
}
