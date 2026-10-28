/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <vector>
#include <algorithm>
#include <cmath>

// THIS HANDLES AUDIO AND MIDI IO AND PROCESSING LOGIC
//==============================================================================
KaveenaRamkissoonAssignment1::KaveenaRamkissoonAssignment1()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
 
{
    for (auto* p : getParameters())
    {
        if (p != nullptr)
        {
            
            DBG("Param: " << p->getName(64) << ", versionHint=" << static_cast<int>(p->getVersionHint()));
        }
    }

    DBG("Constructed plugin. Wrapper type (may still be undefined at this point): " << static_cast<int>(WrapperType()));

    DBG("JucePlugin_IsMidiEffect macro: " << (int)JucePlugin_IsMidiEffect);
    DBG("acceptsMidi(): " << (int)acceptsMidi() << ", producesMidi(): " << (int)producesMidi() << ", isMidiEffect(): " << (int)isMidiEffect());
    DBG("Total input channels: " << getTotalNumInputChannels() << ", Total output channels: " << getTotalNumOutputChannels());

}

KaveenaRamkissoonAssignment1::~KaveenaRamkissoonAssignment1()
{
    
}

//==============================================================================
const juce::String KaveenaRamkissoonAssignment1::getName() const
{
    return JucePlugin_Name;
}

bool KaveenaRamkissoonAssignment1::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool KaveenaRamkissoonAssignment1::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool KaveenaRamkissoonAssignment1::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double KaveenaRamkissoonAssignment1::getTailLengthSeconds() const
{
    return 0.0;
}

int KaveenaRamkissoonAssignment1::getNumPrograms()
{
    return 1;
}

int KaveenaRamkissoonAssignment1::getCurrentProgram()
{
    return 0;
}

void KaveenaRamkissoonAssignment1::setCurrentProgram (int index)
{
}

const juce::String KaveenaRamkissoonAssignment1::getProgramName (int index)
{
    return {};
}

void KaveenaRamkissoonAssignment1::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void KaveenaRamkissoonAssignment1::prepareToPlay(double sampleRate, int samplesPerBlock){
    juce::ignoreUnused(samplesPerBlock);

    notes.clear();
    gnotes.clear();
    cnotes.clear();
    snotes.clear();
    arpNotes.clear();
    latchedNotes.clear();

    currentNote = 0;
    lastNoteValue = -1;
    time = 0;

    rate = static_cast<float>(sampleRate);
    arpSpeed = 0.5f;
    octaves = 1;

    previousMode = Mode::Off;
    arpActive = false;

    DBG("Wrapper type: " << static_cast<int>(WrapperType()));
}

void KaveenaRamkissoonAssignment1::releaseResources(){
    notes.clear();
    gnotes.clear();
    cnotes.clear();
    snotes.clear();
    arpNotes.clear();
    latchedNotes.clear();

    currentNote = 0;
    lastNoteValue = -1;
    time = 0;

    arpActive = false;
    previousMode = Mode::Off;
}


void KaveenaRamkissoonAssignment1::resetPlaybackState(
    juce::MidiBuffer& midi, int offset){
    for (int note : snotes){
        midi.addEvent(juce::MidiMessage::noteOff(1, note), offset);
    }

    snotes.clear();
    cnotes.clear();
    gnotes.clear();
    arpNotes.clear();
    latchedNotes.clear();

    notes.clear();

    lastNoteValue = -1;
    currentNote = 0;
    time = 0;
    arpActive = false;
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool KaveenaRamkissoonAssignment1::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    return layouts.getMainInputChannelSet()  == juce::AudioChannelSet::disabled()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::disabled();
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif
void KaveenaRamkissoonAssignment1::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{

    /*=========================================================================================
     processBlock: receives and produces audio and midi data to output
     
     IF MODE = 0: DO NOTHING
     IF MODE = 1: CHORD GENERATOR, ARPEGGIATES IF MORE THAN ONE NOTE
     IF MODE = 2: CHORD GENERATOR, ARPEGGIATES WHAT IS GENERATED. ARPEGGIATES IF MORE THAN ONE NOTE.
    
     IF LATCH = 0: DO NOTHING
     IF LATCH = 1: MODE MUST BE 2 TO ENABLE, AND ONLY WORKS IF YOU PRESS ONE NOTE, UNTIL ANOTHER SINGLE NOTE IS PRESSED.
     
     ==========================================================================================*/
    

        // lets copy the events in case
        std::vector<std::pair<int, juce::MidiMessage>> inputEvents;

        for (const auto metadata : midi)
        {
            // THIS IS WHERE WE TAKE THE DATA FROM THE MIDI
            inputEvents.emplace_back(
                metadata.samplePosition, metadata.getMessage());
        }

        midi.clear();

        const bool modeChanged = (mode != previousMode);

        if (latch != previousLatch)
        {
            if (!latch && !latchedNotes.isEmpty())
            {
                stopCurrentArpNote(midi, 0);

                {
                    auto expanded = createOctaves(latchedNotes);
                    juce::SortedSet<int> expandedSet;
                    for (int n : expanded)
                        expandedSet.add(n);

                    stopNotes(midi, expandedSet, 0);
                }

                stopGeneratedNotes(midi, 0);

                latchedNotes.clear();
                arpNotes.clear();
                arpActive = false;

                time = 0;
                currentNote = 0;
            }

            previousLatch = latch;
        }

        if (modeChanged)
        {
            resetPlaybackState(midi, 0);
            previousMode = mode;
        }

        // pass through the input MIDI unchanged.
        // any note-offs needed to stop old generated notes have already been added to the output buffer.
        if (mode == Mode::Off)
        {
            for (const auto& event : inputEvents)
            {
                midi.addEvent(event.second, event.first);
            }

            return;
        }

        // if there is a currently sounding arpeggiator note, stop it
        auto stopArpNote = [this, &midi](int offset)
        {
            stopCurrentArpNote(midi, offset);
        };

        // stop all chord notes currently being tracked
        auto stopChordNotes = [this, &midi](int offset)
        {
            if (cnotes.isEmpty())
                return;

            stopNotes(midi, cnotes, offset);
        };

        auto setArpSource = [this, &stopArpNote, &midi](const juce::SortedSet<int>& source, int offset, bool preserveLastGenerated = false)
        {
            // Stop any previously generated notes (that aren't chord notes)
            stopGeneratedNotes(midi, offset);

            stopArpNote(offset);

            arpNotes = source;
            arpActive = (arpNotes.size() >= 2);

            if (!preserveLastGenerated)
                lastGeneratedFromSingle = false;

            time = 0;
            currentNote = 0;
        };

        int cursor = 0;
        const int numSamples = buffer.getNumSamples();

        for (size_t i = 0; i < inputEvents.size(); ++i)
        {
            const auto& event = inputEvents[i];
            const int offset = juce::jlimit(cursor, numSamples, event.first);

            // Process the existing arpeggio up to this event before changing its source.
            if (arpActive && !arpNotes.isEmpty())
            {
                arpeggiator(buffer, midi, arpNotes, cursor, offset);
            }

            cursor = offset;

            const auto& msg = event.second;

            if (!msg.isNoteOn() && !msg.isNoteOff())
            {
                midi.addEvent(msg, offset);
                continue;
            }

            const int note = msg.getNoteNumber();

            // NOTE ON
            if (msg.isNoteOn())
            {
                // ignore duplicates
                if (notes.contains(note))
                {
                    continue;
                }

                notes.add(note);

                // MODE 1:
                // One Physical Note = Chord Generated
                // Two or More Physical Notes = Arpeggiate Physical Notes.
                if (mode == Mode::ChordGenerator)
                {
                    if (notes.size() == 1)
                    {
                        // if there is only one note, we must generate a chord based on this note
                        // clear previous generated notes, and generated new ones
                        gnotes.clear();
                        chordGenerator();

                        if (latch)
                        {
                            latchedNotes = gnotes;
                            latchedIsArpeggio = false; // sustain chord, do not arp
                        }

                        // expanded is the generated notes including the octaves chosen by the user
                        auto expanded = createOctaves(gnotes);
                        stopGeneratedNotes(midi, offset);
                        stopCurrentArpNote(midi, offset);
                        startNotes(midi, expanded, offset);
                    }
                    else
                    {
                        // we are leaving the chord playback and entering the arpeggiator
                        // stopChordNotes just checks to see if cnotes is not empty before stopping the notes
                        stopChordNotes(offset);
                        gnotes.clear();

                        setArpSource(notes, offset);
                        if (latch)
                        {
                            latchedNotes = notes;
                            latchedIsArpeggio = true;
                        }
                    }
                }

                // MODE 2:
                // One Physical Note = Chord Generated and Arpeggiate Generated Notes
                // Two or More Physical Notes = Arpeggiate Physical Notes
                else if (mode == Mode::ChordGeneratorAndArpeggiator)
                {
                    // any new physical input replaces any previous arpeggiated source, including the latched chord
                    latchedNotes.clear();
                    lastGeneratedFromSingle = false;

                    if (notes.size() == 1)
                    {
                        // generating chord and arpeggiating generated notes
                        gnotes.clear();
                        chordGenerator();

                        lastGeneratedFromSingle = true;

                        setArpSource(gnotes, offset, true);
                    }
                    else
                    {
                        // we are now arpeggiating the physical keys
                        gnotes.clear();

                        setArpSource(notes, offset);
                    }
                }
            }

            // NOTE OFF
            else if (msg.isNoteOff())
            {
                // we must ignore note-offs for keys we are not tracking
                if (!notes.contains(note))
                {
                    continue;
                }

                const bool physicalNote = (notes.size() == 1);
                
                bool futureOffsExist = false;
                for (size_t j = i + 1; j < inputEvents.size(); ++j)
                {
                    const auto& laterMsg = inputEvents[j].second;
                    if (laterMsg.isNoteOff())
                    {
                        const int laterNote = laterMsg.getNoteNumber();
                        if (notes.contains(laterNote) && laterNote != note)
                        {
                            futureOffsExist = true;
                            break;
                        }
                    }
                }

                // Remove the note now from our tracked physical-key set.
                notes.removeValue(note);


                if (futureOffsExist)
                    continue;

                // MODE 1
                if (mode == Mode::ChordGenerator)
                {
                    if (notes.isEmpty())
                    {
                        // no physical keys being pressed remain
                        if (latch && !latchedNotes.isEmpty())
                        {
                            // Keep latched notes sounding. If the latched source
                            // is an arpeggio, start it; otherwise, leave the
                            // sustained chord notes as-is.
                            if (latchedIsArpeggio && latchedNotes.size() >= 2)
                            {
                                setArpSource(latchedNotes, offset);
                            }
                            else
                            {
                                // sustain: do not send note-offs for cnotes so
                                // latched chord remains sounding.
                            }
                        }
                        else
                        {
                            stopChordNotes(offset);
                            stopArpNote(offset);
                            stopGeneratedNotes(midi, offset);

                            arpNotes.clear();
                            arpActive = false;

                            gnotes.clear();
                            time = 0;
                            currentNote = 0;
                        }
                    }
                    else if (notes.size() == 1)
                    {
                        // we have returned from a multinote arpeggiation to a 1 chord
                        // we just stop arpeggiating and generate a chord based on the new note
                        stopArpNote(offset);

                        arpNotes.clear();
                        arpActive = false;

                        gnotes.clear();
                        time = 0;
                        currentNote = 0;

                        chordGenerator();

                        if (latch)
                        {
                            latchedNotes = gnotes;
                            latchedIsArpeggio = false;
                        }

                        auto expanded = createOctaves(gnotes);
                        stopGeneratedNotes(midi, offset);
                        stopCurrentArpNote(midi, offset);
                        startNotes(midi, expanded, offset);
                    }
                    else
                    {
                        // continue arpeggiation
                        setArpSource(notes, offset);
                    }
                }

                // MODE 2
                else if (mode == Mode::ChordGeneratorAndArpeggiator){
                    if (physicalNote && latch && lastGeneratedFromSingle){
                        latchedNotes = gnotes;
                        latchedIsArpeggio = true;
                        setArpSource(latchedNotes, offset);
                        lastGeneratedFromSingle = false;
                    }
                    else if (physicalNote && !latch){
                    // No latch: stop the generated chord arpeggio when the key is released.
                    stopArpNote(offset);
                    stopGeneratedNotes(midi, offset);

                    latchedNotes.clear();
                    arpNotes.clear();
                    arpActive = false;

                    gnotes.clear();
                    time = 0;
                    currentNote = 0;
                }
                else if (notes.isEmpty()){
                        // all physical keys have been released
                    stopArpNote(offset);

                    arpNotes.clear();
                    arpActive = false;

                    gnotes.clear();
                    time = 0;
                    currentNote = 0;
                }
                else if (notes.size() == 1){
                    // multi note arpeggiated has returned to one physical key
                    // generated a chord from the key
                    stopArpNote(offset);

                    gnotes.clear();
                    time = 0;
                    currentNote = 0;

                    chordGenerator();

                    // Mark that we generated gnotes from a single key so
                    // subsequent latch logic knows this was a generated chord.
                    lastGeneratedFromSingle = true;

                    setArpSource(gnotes, offset, true);
                }
                else{
                // two or more physical keys remain
                    setArpSource(notes, offset);
                }
            }
        }
    }

        // continue arpeggiating across blocks even when there are no new MIDI events.
        // arpNotes persists as the playback source until it is deliberately replaced or cleared.
    if (arpActive && !arpNotes.isEmpty()){
        arpeggiator(buffer, midi, arpNotes, cursor, numSamples);
    }
}
    
void KaveenaRamkissoonAssignment1::chordGenerator()
{
    // THE GENERATOR MUST SUPPORT:
    // 1. major chords ( +0, +4, +7)
    // 2. minor chords (+0, +3, +7)
    // 3. add9 (+0, +3, +5, +9)

    if (notes.size() != 1){
        return;
    }

    const int root = notes[0];

    auto addChordTone = [&](int interval){
        const int pitch = root + interval;
        if (pitch >= 0 && pitch <= 127){
            gnotes.add(pitch);
        }
    };

    if (chord == ChordType::Major){
        // major
        addChordTone(0);
        addChordTone(4);
        addChordTone(7);
    }
    else if (chord == ChordType::Minor){
        // minor
        addChordTone(0);
        addChordTone(3);
        addChordTone(7);
    }
    else{
        // add9
        addChordTone(0);
        addChordTone(3);
        addChordTone(5);
        addChordTone(9);
    }
    // Mark that these gnotes were produced from a single physical note.
    lastGeneratedFromSingle = true;
}

std::vector<int> KaveenaRamkissoonAssignment1::createOctaves(
    const juce::SortedSet<int>& notesToBeModified){
    // MUST ALLOW USER TO CHOOSE DESCENDING OR ASCENDING FOR ARPEG.
    // USER NEEDS TO BE ALLOWED TO CHOOSE # OF OCTAVES (+12)
    // Separately called function as this is used no matter mode 1 or 2.

    std::vector<int> octaved;

    // Octave Range is the total number of octaves, starting at 1.
    const int octaveCount = juce::jlimit(1, 3, octaves);
    for (int octave = 0; octave < octaveCount; ++octave){
        // For ascending order we expand upward (0, +12, +24 ...).
        // For descending order we expand downward (0, -12, -24 ...).
        const int offset = (order == Order::Descending) ? -(octave * 12) : (octave * 12);
        // Preserve the original note order within each octave.
        for (int note : notesToBeModified){
            const int expanded = note + offset;
            if (expanded >= 0 && expanded <= 127){
                octaved.push_back(expanded);
            }
        }
    }

    return octaved;
}

void KaveenaRamkissoonAssignment1::startNotes(juce::MidiBuffer& midi, const std::vector<int>& notesToBePlayed, int offset){
    // this will be used in mode 1 for the chord generation
    // startNotes goes through every note in the given list and outputs them
    // cnotes = current notes, we use this to track notes that need note-offs.

    for (int note : notesToBePlayed){
        if (cnotes.contains(note)){
            continue;
        }

        midi.addEvent(juce::MidiMessage::noteOn(1, note, static_cast<juce::uint8>(127)), offset);

        cnotes.add(note);
        snotes.add(note);
    }
}
    
void KaveenaRamkissoonAssignment1::stopNotes(juce::MidiBuffer& midi, const juce::SortedSet<int> notesToBeStopped, int offset){
    // this is used in mode 2 for the chord generation
    // stopNotes goes through every note in the list, checks if they are in need to be turned off and stops them
    // cnotes = current notes, it is used here to make sure that every value that needs to be removed is removed

    const auto notesToStop = notesToBeStopped;

    for (int note : notesToStop){
        if (!cnotes.contains(note)){
            continue;
        }

        midi.addEvent(juce::MidiMessage::noteOff(1, note), offset);

        cnotes.removeValue(note);
        snotes.removeValue(note);
    }
}

void KaveenaRamkissoonAssignment1::stopGeneratedNotes(juce::MidiBuffer& midi, int offset)
{
    // Stop any sounding notes that are not part of the current chord (cnotes).
    juce::Array<int> toStop;

    for (int n : snotes)
    {
        if (!cnotes.contains(n))
            toStop.add(n);
    }

    for (int n : toStop)
    {
        midi.addEvent(juce::MidiMessage::noteOff(1, n), offset);
        snotes.removeValue(n);
    }
}

void KaveenaRamkissoonAssignment1::stopCurrentArpNote(juce::MidiBuffer& midi, int offset){
    if (lastNoteValue >= 0){
        midi.addEvent(
            juce::MidiMessage::noteOff(1, lastNoteValue), offset);

        snotes.removeValue(lastNoteValue);
        lastNoteValue = -1;
    }
}


void KaveenaRamkissoonAssignment1::arpeggiator(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, const juce::SortedSet<int>& notesToBePlayed, int eventOffset, int endOffset){
    // this is where we will arpeggiate whatever is in notesToBePlayed
    // this will be used by mode 1, when more than one note appears
    // this will be used by mode 2, no matter what
    // if latch is on, arpeggiator will happen until latch = 0 or until the values are removed from notes.
    // order of loading depends on asc or dsc.
    // pattern + timing in the buffer depends on pattern clause
    // pattern 1: standard timing
    // pattern 2: first two notes last 2x longer
    // pattern 3: randomized durations (per-step multipliers within a safe range)

    const int numSamples = buffer.getNumSamples();

    const int startOffset = juce::jlimit(0, numSamples, eventOffset);

    const int finishOffset = (endOffset < 0) ? numSamples: juce::jlimit(startOffset, numSamples, endOffset);

    if (finishOffset <= startOffset){
        return;
    }

    std::vector<int> baseSequence;
    baseSequence.reserve(notesToBePlayed.size());
    for (int n : notesToBePlayed)
        baseSequence.push_back(n);

    if (order == Order::Descending)
        std::reverse(baseSequence.begin(), baseSequence.end());

    const int baseCount = static_cast<int>(baseSequence.size());

    // if source is invalid stop any arpeggiator
    if (baseCount < 2)
    {
        stopCurrentArpNote(midi, startOffset);

        time = 0;
        currentNote = 0;

        return;
    }

    // expand the base sequence across octaves (ascending by octave) to
    // produce the playback sequence.
    const int octaveCount = juce::jlimit(1, 3, octaves);
    std::vector<int> sequence;
    sequence.reserve(baseCount * octaveCount);
    for (int octave = 0; octave < octaveCount; ++octave)
    {
        // For ascending order expand upward; for descending expand downward
        const int offset = (order == Order::Descending) ? -(octave * 12) : (octave * 12);
        for (int n : baseSequence)
        {
            const int expanded = n + offset;
            if (expanded >= 0 && expanded <= 127)
                sequence.push_back(expanded);
        }
    }

    const int noteCount = static_cast<int>(sequence.size());

    // get note duration
    // rate is the internal sample rate; arpSpeed is the user's parameter.
    const int noteDuration = juce::jmax(1, static_cast<int>(std::ceil(rate * 0.25f * (0.1f + (1.0f - arpSpeed)))));

    // this is where we get the duration for the current sequence position
    // this is where the different patterns come into play
    auto getStepDuration = [&](int index){
        const int baseIndex = baseCount > 0 ? (index % baseCount) : index;

        float multiplier = 1.0f;

        if (pattern == Pattern::FirstTwoLonger)
        {
            // For every iteration of the base sequence, the first two
            // positions are longer.
            if (baseIndex == 0 || baseIndex == 1)
                multiplier = 2.0f;
        }
        else if (pattern == Pattern::Randomized)
        {
            multiplier = (baseIndex % 2 == 0) ? 1.5f : 0.67f;
        }

        return juce::jmax(1, static_cast<int>(std::round(noteDuration * multiplier)));
    };

    // the first note starts immediately when no arp note is active
    if (lastNoteValue < 0){
        currentNote = 0;
        lastNoteValue = sequence[currentNote];

        midi.addEvent(juce::MidiMessage::noteOn(1, lastNoteValue, static_cast<juce::uint8>(127)), startOffset);

        snotes.add(lastNoteValue);
        time = 0;
    }

    // process only the samples between this event and the next event
    int outputOffset = startOffset;

    // "time" is the number of samples that have already elapsed in the current step
    while (outputOffset < finishOffset){
        const int stepDuration = getStepDuration(currentNote);
        const int samplesUntilNextStep = stepDuration - time;
        const int samplesRemaining = finishOffset - outputOffset;

        // no note transitions happen in the rest of this segment
        if (samplesUntilNextStep > samplesRemaining){
            time += samplesRemaining;
            break;
        }

        // Advance to the next step's exact sample position.
        outputOffset += samplesUntilNextStep;

        // If the step ends exactly at the segment boundary, preserve that
        // state and perform the transition when the next segment begins.
        if (outputOffset >= finishOffset){
            time = stepDuration;
            break;
        }

        // handles turning off previous generated note
        stopCurrentArpNote(midi, outputOffset);

        // advance through sequence and wrap around
        currentNote = (currentNote + 1) % noteCount;

        lastNoteValue = sequence[currentNote];

        midi.addEvent(juce::MidiMessage::noteOn(1, lastNoteValue, static_cast<juce::uint8>(127)), outputOffset);

        snotes.add(lastNoteValue);

        // new note started
        time = 0;
    }
}



int KaveenaRamkissoonAssignment1::getChoiceParameter (const juce::String& parameterID) const
{
    if (parameterID == "mode") return static_cast<int>(mode);
    if (parameterID == "chord") return static_cast<int>(chord);
    if (parameterID == "order") return static_cast<int>(order);
    if (parameterID == "pattern") return static_cast<int>(pattern);
    if (parameterID == "latch") return latch ? 1 : 0;
    if (parameterID == "octaves") return octaves;
    return 0;
}

void KaveenaRamkissoonAssignment1::setChoiceParameter (const juce::String& parameterID, int newValue)
{
    if (parameterID == "mode") mode = static_cast<Mode>(newValue);
    else if (parameterID == "chord") chord = static_cast<ChordType>(newValue);
    else if (parameterID == "order") order = static_cast<Order>(newValue);
    else if (parameterID == "pattern") pattern = static_cast<Pattern>(newValue);
    else if (parameterID == "octaves") octaves = newValue;
}

bool KaveenaRamkissoonAssignment1::getBooleanParameter (const juce::String& parameterID) const
{
    if (parameterID == "latch") return latch;
    return false;
}

void KaveenaRamkissoonAssignment1::setBooleanParameter (const juce::String& parameterID, bool newValue)
{
    if (parameterID == "latch") latch = newValue;
}

int KaveenaRamkissoonAssignment1::getIntParameter (const juce::String& parameterID) const
{
    if (parameterID == "octaves") return octaves;
    return 0;
}

void KaveenaRamkissoonAssignment1::setIntParameter (const juce::String& parameterID, int newValue)
{
    if (parameterID == "octaves") octaves = newValue;
}

float KaveenaRamkissoonAssignment1::getFloatParameter (const juce::String& parameterID) const
{
    if (parameterID == "arpSpeed") return arpSpeed;
    return 0.0f;
}

void KaveenaRamkissoonAssignment1::setFloatParameter (const juce::String& parameterID, float newValue)
{
    if (parameterID == "arpSpeed") arpSpeed = newValue;
}

//==============================================================================
bool KaveenaRamkissoonAssignment1::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* KaveenaRamkissoonAssignment1::createEditor()
{
    return new KaveenaRamkissoonAssignment1Editor (*this);
}

//==============================================================================
void KaveenaRamkissoonAssignment1::getStateInformation (juce::MemoryBlock& destData)
{
}

void KaveenaRamkissoonAssignment1::setStateInformation (const void* data, int sizeInBytes)
{
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KaveenaRamkissoonAssignment1();
}
