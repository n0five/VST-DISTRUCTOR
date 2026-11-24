#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr int kNumParams = 11;
    const char* const kIds[kNumParams] = { "drive", "hard", "fold", "bias", "rate", "bits",
                                           "muffle", "wobble", "room", "out", "mix" };

    struct Preset { const char* name; float v[kNumParams]; };

    const Preset kPresets[] =
    {
        //                            drive  hard   fold   bias  rate  bits  muffle wobble room   out    mix
        // --- ENCLOSED : son "enferme" ---
        { "Cup Of Drank",           {  3.f, .30f, 0.f,  0.f,  1.f, 16.f, .72f, .15f, .25f,  -2.f, 1.f } },
        { "Locked In The Trunk",    {  6.f, .40f, 0.f, .05f,  1.f, 16.f, .90f, .10f, .45f,  -3.f, 1.f } },
        { "Underwater Sip",         {  4.f, .20f, 0.f,  0.f,  1.f, 16.f, .82f, .65f, .50f,  -3.f, 1.f } },
        { "Party In The Next Room", {  2.f, .20f, 0.f,  0.f,  1.f, 16.f, .62f, 0.f,  .35f,  -1.f, 1.f } },
        { "Codeine Haze",           {  8.f, .35f, .05f, .05f, 1.f, 14.f, .55f, .45f, .60f,  -4.f, .90f } },
        // --- DISTORTION : style rage ---
        { "Rage Syrup",             { 30.f, .80f, .30f, .15f, 4.f,  9.f, .08f, 0.f,  0.f,  -12.f, 1.f } },
        { "Sprite Demon",           { 36.f, .90f, .50f, .20f, 6.f,  7.f, .05f, 0.f,  0.f,  -14.f, 1.f } },
        { "Purple Meltdown",        { 42.f, 1.f,  .60f, .30f, 8.f,  6.f, .12f, 0.f,  .05f, -16.f, 1.f } },
        { "808 Lean",               { 24.f, .70f, .20f, .10f, 2.f, 12.f, .20f, 0.f,  0.f,  -10.f, .55f } },
        { "Vocal Gargle",           { 22.f, .60f, .15f, .10f, 2.f, 12.f, .10f, 0.f,  0.f,   -9.f, .70f } }
    };

    inline float shape (float x, float bias, float fold, float hard)
    {
        x += bias;
        const float soft = std::tanh (x);
        const float clip = juce::jlimit (-1.f, 1.f, x);
        float y = soft + (clip - soft) * hard;

        if (fold > 0.f)
        {
            const float f = 1.f + fold * 4.f;
            const float folded = std::sin (y * f * juce::MathConstants<float>::halfPi);
            y += (folded - y) * fold;
        }
        return y;
    }
}

int DrankAudioProcessor::numPresets() { return (int) (sizeof (kPresets) / sizeof (kPresets[0])); }

// ---------------------------------------------------------------- parameters
juce::AudioProcessorValueTreeState::ParameterLayout DrankAudioProcessor::createLayout()
{
    using P  = juce::AudioParameterFloat;
    using NR = juce::NormalisableRange<float>;
    using A  = juce::AudioParameterFloatAttributes;

    const auto pct = A().withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v * 100.f)) + " %"; })
                        .withValueFromStringFunction ([] (const juce::String& s) { return s.getFloatValue() / 100.f; });
    const auto db   = A().withStringFromValueFunction ([] (float v, int) { return juce::String (v, 1) + " dB"; });
    const auto rate = A().withStringFromValueFunction ([] (float v, int) { return juce::String (v, 1) + "x"; });
    const auto bits = A().withStringFromValueFunction ([] (float v, int) { return juce::String (v, 1) + " bit"; });
    const auto num  = A().withStringFromValueFunction ([] (float v, int) { return juce::String (v, 2); });

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back (std::make_unique<P> ("drive",  "Drive",       NR (0.f, 48.f, 0.1f),        12.f, db));
    p.push_back (std::make_unique<P> ("hard",   "Hard Clip",   NR (0.f, 1.f, 0.001f),       .5f,  num));
    p.push_back (std::make_unique<P> ("fold",   "Fold",        NR (0.f, 1.f, 0.001f),       0.f,  num));
    p.push_back (std::make_unique<P> ("bias",   "Asym",        NR (0.f, 0.6f, 0.001f),      0.f,  num));
    p.push_back (std::make_unique<P> ("rate",   "Crush Rate",  NR (1.f, 40.f, 0.1f, 0.5f),  1.f,  rate));
    p.push_back (std::make_unique<P> ("bits",   "Crush Bits",  NR (3.f, 16.f, 0.1f),        16.f, bits));
    p.push_back (std::make_unique<P> ("muffle", "Muffle",      NR (0.f, 1.f, 0.001f),       0.f,  pct));
    p.push_back (std::make_unique<P> ("wobble", "Wobble",      NR (0.f, 1.f, 0.001f),       0.f,  pct));
    p.push_back (std::make_unique<P> ("room",   "Room",        NR (0.f, 1.f, 0.001f),       0.f,  pct));
    p.push_back (std::make_unique<P> ("out",    "Output",      NR (-30.f, 6.f, 0.1f),       -6.f, db));
    p.push_back (std::make_unique<P> ("mix",    "Mix",         NR (0.f, 1.f, 0.001f),       1.f,  pct));
    return { p.begin(), p.end() };
}

DrankAudioProcessor::DrankAudioProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

bool DrankAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
           && out == l.getMainInputChannelSet();
}

// ---------------------------------------------------------------- presets
int DrankAudioProcessor::getNumPrograms() { return numPresets(); }

const juce::String DrankAudioProcessor::getProgramName (int index)
{
    if (index < 0 || index >= numPresets()) return {};
    return kPresets[index].name;
}

void DrankAudioProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= numPresets()) return;
    currentPreset = index;

    for (int k = 0; k < kNumParams; ++k)
        if (auto* prm = apvts.getParameter (kIds[k]))
            prm->setValueNotifyingHost (prm->convertTo0to1 (kPresets[index].v[k]));
}

// ---------------------------------------------------------------- lifecycle
void DrankAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock,
                                  (juce::uint32) getTotalNumOutputChannels() };

    lowpass.prepare (spec);  lowpass.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    lowpass.setResonance (0.8f);
    dcBlock.prepare (spec);  dcBlock.setType (juce::dsp::StateVariableTPTFilterType::highpass);
    dcBlock.setCutoffFrequency (20.f);

    reverb.prepare (spec);
    reverb.reset();

    oversampling.initProcessing ((size_t) samplesPerBlock);
    oversampling.reset();

    mixer.prepare (spec);
    mixer.setWetLatency (oversampling.getLatencyInSamples());
    setLatencySamples (juce::roundToInt (oversampling.getLatencyInSamples()));

    for (int c = 0; c < 2; ++c) { held[c] = 0.f; counter[c] = 0.f; }
    lfoPhase = 0.f;
}

// ---------------------------------------------------------------- audio
void DrankAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    const float drive  = juce::Decibels::decibelsToGain (get ("drive"));
    const float hard   = get ("hard");
    const float fold   = get ("fold");
    const float bias   = get ("bias");
    const float rate   = get ("rate");
    const float bits   = get ("bits");
    const float muffle = get ("muffle");
    const float wobble = get ("wobble");
    const float room   = get ("room");
    const float outG   = juce::Decibels::decibelsToGain (get ("out"));

    const int nCh = juce::jmin (buffer.getNumChannels(), 2);
    const int nS  = buffer.getNumSamples();
    if (nCh == 0 || nS == 0) return;

    float* d[2] = { buffer.getWritePointer (0), nCh > 1 ? buffer.getWritePointer (1) : nullptr };

    juce::dsp::AudioBlock<float> full (buffer);
    auto block = full.getSubsetChannelBlock (0, (size_t) nCh);

    mixer.setWetMixProportion (get ("mix"));
    mixer.pushDrySamples (block);

    // 1) drive
    for (int ch = 0; ch < nCh; ++ch)
        for (int i = 0; i < nS; ++i)
            d[ch][i] *= drive;

    // 2) distorsion (surechantillonnee x4)
    auto up = oversampling.processSamplesUp (block);
    for (size_t ch = 0; ch < up.getNumChannels(); ++ch)
    {
        auto* p = up.getChannelPointer (ch);
        for (size_t i = 0; i < up.getNumSamples(); ++i)
            p[i] = shape (p[i], bias, fold, hard);
    }
    oversampling.processSamplesDown (block);

    // 3) bitcrusher
    if (rate > 1.01f || bits < 15.9f)
    {
        const float levels = std::pow (2.f, bits) * 0.5f;
        for (int ch = 0; ch < nCh; ++ch)
        {
            for (int i = 0; i < nS; ++i)
            {
                counter[ch] -= 1.f;
                if (counter[ch] <= 0.f)
                {
                    held[ch] = std::round (juce::jlimit (-1.f, 1.f, d[ch][i]) * levels) / levels;
                    counter[ch] += rate;
                }
                d[ch][i] = held[ch];
            }
        }
    }

    // 4) muffle : passe-bas qui ferme le son, avec un wobble lent (effet "lean")
    const float sr      = (float) getSampleRate();
    const float baseCut = 20000.f * std::pow (300.f / 20000.f, muffle);
    const float depth   = wobble * 1.3f;
    const float lfoInc  = 0.45f * juce::MathConstants<float>::twoPi / sr;
    const float maxCut  = juce::jmin (20000.f, sr * 0.45f);

    for (int i = 0; i < nS; ++i)
    {
        if ((i & 31) == 0)
            lowpass.setCutoffFrequency (juce::jlimit (100.f, maxCut, baseCut * std::pow (2.f, depth * std::sin (lfoPhase))));

        lfoPhase += lfoInc;
        if (lfoPhase > juce::MathConstants<float>::twoPi) lfoPhase -= juce::MathConstants<float>::twoPi;

        for (int ch = 0; ch < nCh; ++ch)
            d[ch][i] = lowpass.processSample (ch, d[ch][i]);
    }

    // 5) petite piece (reverb sombre) pour l'effet "enferme"
    if (room > 0.001f)
    {
        juce::dsp::Reverb::Parameters rp;
        rp.roomSize   = 0.3f + room * 0.4f;
        rp.damping    = 0.9f;
        rp.wetLevel   = room * 0.45f;
        rp.dryLevel   = 1.f - room * 0.35f;
        rp.width      = 0.7f;
        rp.freezeMode = 0.f;
        reverb.setParameters (rp);
        reverb.process (juce::dsp::ProcessContextReplacing<float> (block));
    }

    // 6) anti-DC, volume de sortie, securite
    for (int ch = 0; ch < nCh; ++ch)
        for (int i = 0; i < nS; ++i)
            d[ch][i] = juce::jlimit (-1.f, 1.f, dcBlock.processSample (ch, d[ch][i]) * outG);

    mixer.mixWetSamples (block);
}

// ---------------------------------------------------------------- editor / state
juce::AudioProcessorEditor* DrankAudioProcessor::createEditor()
{
    return new DrankEditor (*this);
}

void DrankAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void DrankAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DrankAudioProcessor();
}
