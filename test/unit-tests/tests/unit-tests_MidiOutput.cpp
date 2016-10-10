#include "unit-tests.h"
#include <src/MIDI.h>
#include <test/mocks/test-mocks_SerialMock.h>

BEGIN_MIDI_NAMESPACE

END_MIDI_NAMESPACE

// -----------------------------------------------------------------------------

BEGIN_UNNAMED_NAMESPACE

template<bool RunningStatus, bool OneByteParsing>
struct VariableSettings : public midi::DefaultSettings
{
    static const bool UseRunningStatus = RunningStatus;
    static const bool Use1ByteParsing  = OneByteParsing;
};

template<bool A, bool B>
const bool VariableSettings<A, B>::UseRunningStatus;
template<bool A, bool B>
const bool VariableSettings<A, B>::Use1ByteParsing;

using namespace testing;
typedef test_mocks::SerialMock<32> SerialMock;
typedef midi::MidiInterface<SerialMock> MidiInterface;

typedef std::vector<uint8_t> Buffer;

// --

TEST(MidiOutput, sendInvalid)
{
    SerialMock serial;
    MidiInterface midi(serial);

    midi.begin();
    midi.send(midi::NoteOn, 42, 42, 42);                // Invalid channel > OFF
    EXPECT_EQ(serial.mTxBuffer.getLength(), 0);

    midi.send(midi::InvalidType, 0, 0, 12);             // Invalid type
    EXPECT_EQ(serial.mTxBuffer.getLength(), 0);

    midi.send(midi::NoteOn, 12, 42, MIDI_CHANNEL_OMNI); // OMNI not allowed
    EXPECT_EQ(serial.mTxBuffer.getLength(), 0);
}

TEST(MidiOutput, sendGenericSingle)
{
    SerialMock serial;
    MidiInterface midi(serial);
    Buffer buffer;
    buffer.resize(3);

    midi.begin();
    midi.send(midi::NoteOn, 47, 42, 12);
    EXPECT_EQ(serial.mTxBuffer.getLength(), 3);
    serial.mTxBuffer.read(&buffer[0], 3);
    EXPECT_THAT(buffer, ElementsAreArray({0x9b, 47, 42}));
}

TEST(MidiOutput, sendGenericWithRunningStatus)
{
    SerialMock serial;
    MidiInterface midi(serial);
    Buffer buffer;
    buffer.resize(5);

    midi.begin();
    EXPECT_EQ(MidiInterface::Settings::UseRunningStatus, true);
    EXPECT_EQ(serial.mTxBuffer.isEmpty(), true);
    midi.send(midi::NoteOn, 47, 42, 12);
    midi.send(midi::NoteOn, 42, 47, 12);
    EXPECT_EQ(serial.mTxBuffer.getLength(), 5);
    serial.mTxBuffer.read(&buffer[0], 5);
    EXPECT_THAT(buffer, ElementsAreArray({0x9b, 47, 42, 42, 47}));
}

TEST(MidiOutput, sendGenericWithoutRunningStatus)
{
    typedef VariableSettings<false, true> Settings; // No running status
    typedef midi::MidiInterface<SerialMock, Settings> MidiInterface;

    SerialMock serial;
    MidiInterface midi(serial);
    Buffer buffer;
    buffer.resize(6);

    // Same status byte
    midi.begin();
    EXPECT_EQ(MidiInterface::Settings::UseRunningStatus, false);
    EXPECT_EQ(serial.mTxBuffer.isEmpty(), true);
    midi.send(midi::NoteOn, 47, 42, 12);
    midi.send(midi::NoteOn, 42, 47, 12);
    EXPECT_EQ(serial.mTxBuffer.getLength(), 6);
    serial.mTxBuffer.read(&buffer[0], 6);
    EXPECT_THAT(buffer, ElementsAreArray({0x9b, 47, 42, 0x9b, 42, 47}));

    // Different status byte
    midi.begin();
    midi.send(midi::NoteOn,  47, 42, 12);
    midi.send(midi::NoteOff, 47, 42, 12);
    EXPECT_EQ(serial.mTxBuffer.getLength(), 6);
    serial.mTxBuffer.read(&buffer[0], 6);
    EXPECT_THAT(buffer, ElementsAreArray({0x9b, 47, 42, 0x8b, 47, 42}));
}

TEST(MidiOutput, sendGenericBreakingRunningStatus)
{
    SerialMock serial;
    MidiInterface midi(serial);
    Buffer buffer;
    buffer.resize(6);

    midi.begin();
    midi.send(midi::NoteOn,  47, 42, 12);
    midi.send(midi::NoteOff, 47, 42, 12);
    EXPECT_EQ(serial.mTxBuffer.getLength(), 6);
    serial.mTxBuffer.read(&buffer[0], 6);
    EXPECT_THAT(buffer, ElementsAreArray({0x9b, 47, 42, 0x8b, 47, 42}));
}

TEST(MidiOutput, sendGenericRealTimeShortcut)
{
    SerialMock serial;
    MidiInterface midi(serial);
    Buffer buffer;
    buffer.resize(7);

    midi.begin();
    midi.send(midi::TuneRequest,    47, 42, 12);
    midi.send(midi::Clock,          47, 42, 12);
    midi.send(midi::Start,          47, 42, 12);
    midi.send(midi::Continue,       47, 42, 12);
    midi.send(midi::Stop,           47, 42, 12);
    midi.send(midi::ActiveSensing,  47, 42, 12);
    midi.send(midi::SystemReset,    47, 42, 12);

    EXPECT_EQ(serial.mTxBuffer.getLength(), 7);
    serial.mTxBuffer.read(&buffer[0], 7);
    EXPECT_THAT(buffer, ElementsAreArray({0xf6, 0xf8, 0xfa, 0xfb, 0xfc, 0xfe, 0xff}));
}

// --

TEST(MidiOutput, sendNoteOn)
{
    SerialMock serial;
    MidiInterface midi(serial);
    Buffer buffer;
    buffer.resize(5);

    midi.begin();
    midi.sendNoteOn(10, 11, 12);
    midi.sendNoteOn(12, 13, 12);
    EXPECT_EQ(serial.mTxBuffer.getLength(), 5);
    serial.mTxBuffer.read(&buffer[0], 5);
    EXPECT_THAT(buffer, ElementsAreArray({0x9b, 10, 11, 12, 13}));
}

TEST(MidiOutput, sendNoteOff)
{
    SerialMock serial;
    MidiInterface midi(serial);
    Buffer buffer;
    buffer.resize(5);

    midi.begin();
    midi.sendNoteOff(10, 11, 12);
    midi.sendNoteOff(12, 13, 12);
    EXPECT_EQ(serial.mTxBuffer.getLength(), 5);
    serial.mTxBuffer.read(&buffer[0], 5);
    EXPECT_THAT(buffer, ElementsAreArray({0x8b, 10, 11, 12, 13}));
}

TEST(MidiOutput, sendProgramChange)
{
    SerialMock serial;
    MidiInterface midi(serial);
    Buffer buffer;
    buffer.resize(3);

    midi.begin();
    midi.sendProgramChange(42, 12);
    midi.sendProgramChange(47, 12);
    EXPECT_EQ(serial.mTxBuffer.getLength(), 3);
    serial.mTxBuffer.read(&buffer[0], 3);
    EXPECT_THAT(buffer, ElementsAreArray({0xcb, 42, 47}));
}

TEST(MidiOutput, sendControlChange)
{
    SerialMock serial;
    MidiInterface midi(serial);
    Buffer buffer;
    buffer.resize(5);

    midi.begin();
    midi.sendControlChange(42, 12, 12);
    midi.sendControlChange(47, 12, 12);
    EXPECT_EQ(serial.mTxBuffer.getLength(), 5);
    serial.mTxBuffer.read(&buffer[0], 5);
    EXPECT_THAT(buffer, ElementsAreArray({0xbb, 42, 12, 47, 12}));
}

TEST(MidiOutput, sendPitchBend)
{
    SerialMock serial;
    MidiInterface midi(serial);
    Buffer buffer;

    // Int signature - arbitrary values
    {
        buffer.clear();
        buffer.resize(7);

        midi.begin();
        midi.sendPitchBend(0, 12);
        midi.sendPitchBend(100, 12);
        midi.sendPitchBend(-100, 12);
        EXPECT_EQ(serial.mTxBuffer.getLength(), 7);
        serial.mTxBuffer.read(&buffer[0], 7);
        EXPECT_THAT(buffer, ElementsAreArray({0xeb,
                                              0x00, 0x40,
                                              0x64, 0x40,
                                              0x1c, 0x3f}));
    }
    // Int signature - min/max
    {
        buffer.clear();
        buffer.resize(7);

        midi.begin();
        midi.sendPitchBend(0, 12);
        midi.sendPitchBend(MIDI_PITCHBEND_MAX, 12);
        midi.sendPitchBend(MIDI_PITCHBEND_MIN, 12);
        EXPECT_EQ(serial.mTxBuffer.getLength(), 7);
        serial.mTxBuffer.read(&buffer[0], 7);
        EXPECT_THAT(buffer, ElementsAreArray({0xeb,
                                              0x00, 0x40,
                                              0x7f, 0x7f,
                                              0x00, 0x00}));
    }
    // Float signature
    {
        buffer.clear();
        buffer.resize(7);

        midi.begin();
        midi.sendPitchBend(0.0,  12);
        midi.sendPitchBend(1.0,  12);
        midi.sendPitchBend(-1.0, 12);
        EXPECT_EQ(serial.mTxBuffer.getLength(), 7);
        serial.mTxBuffer.read(&buffer[0], 7);
        EXPECT_THAT(buffer, ElementsAreArray({0xeb,
                                              0x00, 0x40,
                                              0x7f, 0x7f,
                                              0x00, 0x00}));
    }
}

TEST(MidiOutput, sendPolyPressure)
{
    SerialMock serial;
    MidiInterface midi(serial);
    Buffer buffer;
    buffer.resize(5);

    midi.begin();
    midi.sendPolyPressure(42, 12, 12);
    midi.sendPolyPressure(47, 12, 12);
    EXPECT_EQ(serial.mTxBuffer.getLength(), 5);
    serial.mTxBuffer.read(&buffer[0], 5);
    EXPECT_THAT(buffer, ElementsAreArray({0xab, 42, 12, 47, 12}));
}

TEST(MidiOutput, sendAfterTouch)
{
    SerialMock serial;
    MidiInterface midi(serial);
    Buffer buffer;
    buffer.resize(3);

    midi.begin();
    midi.sendAfterTouch(42, 12);
    midi.sendAfterTouch(47, 12);
    EXPECT_EQ(serial.mTxBuffer.getLength(), 3);
    serial.mTxBuffer.read(&buffer[0], 3);
    EXPECT_THAT(buffer, ElementsAreArray({0xdb, 42, 47}));
}

TEST(MidiOutput, sendSysEx)
{
    typedef test_mocks::SerialMock<1024> SerialMock;
    typedef midi::MidiInterface<SerialMock> MidiInterface;


    SerialMock serial;
    MidiInterface midi(serial);
    Buffer buffer;

    // Small frame
    {
        static const char* frame = "Hello, World!";
        static const unsigned frameLength = strlen(frame);
        static const byte expected[] = {
            0xf0,
            'H', 'e', 'l', 'l', 'o', ',', ' ', 'W', 'o', 'r', 'l', 'd', '!',
            0xf7,
        };

        buffer.clear();
        buffer.resize(frameLength + 2);

        midi.begin();
        midi.sendSysEx(frameLength, reinterpret_cast<const byte*>(frame), false);
        EXPECT_EQ(serial.mTxBuffer.getLength(), frameLength + 2);
        serial.mTxBuffer.read(&buffer[0], frameLength + 2);
        EXPECT_THAT(buffer, ElementsAreArray(expected));
    }
    // Large frame
    {
        static const char* frame = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Proin maximus dui a massa maximus, a vestibulum mi venenatis. Cras sit amet ex id velit suscipit pharetra eget a turpis. Phasellus interdum metus ac sagittis cursus. Nam quis est at nisl ullamcorper egestas pulvinar eu erat. Duis a elit dignissim, vestibulum eros vel, tempus nisl. Aenean turpis nunc, cursus vel lacinia non, pharetra eget sapien. Duis condimentum, lacus at pulvinar tempor, leo libero volutpat nisl, eget porttitor lorem mi sed magna. Duis dictum, massa vel euismod interdum, lorem mi egestas elit, hendrerit tincidunt est arcu a libero. Interdum et malesuada fames ac ante ipsum primis in faucibus. Curabitur vehicula magna libero, at rhoncus sem ornare a. In elementum, elit et congue pulvinar, massa velit commodo velit, non elementum purus ligula eget lacus. Donec efficitur nisi eu ultrices efficitur. Donec neque dui, ullamcorper id molestie quis, consequat sit amet ligula.";
        static const unsigned frameLength = strlen(frame);
        static const byte expected[] = {
            0xf0,
            'L','o','r','e','m',' ','i','p','s','u','m',' ','d','o','l','o','r',' ','s','i','t',' ','a','m','e','t',',',' ',
            'c','o','n','s','e','c','t','e','t','u','r',' ','a','d','i','p','i','s','c','i','n','g',' ','e','l','i','t','.',' ','P','r','o','i','n',' ','m','a','x','i','m','u','s',' ','d','u','i',' ','a',' ','m','a','s','s','a',' ','m','a','x','i','m','u','s',',',' ',
            'a',' ','v','e','s','t','i','b','u','l','u','m',' ','m','i',' ','v','e','n','e','n','a','t','i','s','.',' ','C','r','a','s',' ','s','i','t',' ','a','m','e','t',' ','e','x',' ','i','d',' ','v','e','l','i','t',' ','s','u','s','c','i','p','i','t',' ','p','h','a','r','e','t','r','a',' ','e','g','e','t',            ' ','a',' ','t','u','r','p','i','s','.',' ','P','h','a','s','e','l','l','u','s',' ','i','n','t','e','r','d','u','m',' ','m','e','t','u','s',' ','a','c',' ','s','a','g','i','t','t','i','s',' ','c','u','r','s','u','s','.',' ','N','a','m',' ','q','u','i','s',' ','e','s','t',' ','a','t',' ','n','i','s',            'l',' ','u','l','l','a','m','c','o','r','p','e','r',' ','e','g','e','s','t','a','s',' ','p','u','l','v','i','n','a','r',' ','e','u',' ','e','r','a','t','.',' ','D','u','i','s',' ','a',' ','e','l','i','t',' ','d','i','g','n','i','s','s','i','m',',',' ',
            'v','e','s','t','i','b','u','l','u','m',' ','e','r','o','s',' ','v','e','l',',',' ',
            't','e','m','p','u','s',' ','n','i','s','l','.',' ','A','e','n','e','a','n',' ','t','u','r','p','i','s',' ','n','u','n','c',',',' ',
            'c','u','r','s','u','s',' ','v','e','l',' ','l','a','c','i','n','i','a',' ','n','o','n',',',' ',
            'p','h','a','r','e','t','r','a',' ','e','g','e','t',' ','s','a','p','i','e','n','.',' ','D','u','i','s',' ','c','o','n','d','i','m','e','n','t','u','m',',',' ',
            'l','a','c','u','s',' ','a','t',' ','p','u','l','v','i','n','a','r',' ','t','e','m','p','o','r',',',' ',
            'l','e','o',' ','l','i','b','e','r','o',' ','v','o','l','u','t','p','a','t',' ','n','i','s','l',',',' ',
            'e','g','e','t',' ','p','o','r','t','t','i','t','o','r',' ','l','o','r','e','m',' ','m','i',' ','s','e','d',' ','m','a','g','n','a','.',' ','D','u','i','s',' ','d','i','c','t','u','m',',',' ',
            'm','a','s','s','a',' ','v','e','l',' ','e','u','i','s','m','o','d',' ','i','n','t','e','r','d','u','m',',',' ',
            'l','o','r','e','m',' ','m','i',' ','e','g','e','s','t','a','s',' ','e','l','i','t',',',' ',
            'h','e','n','d','r','e','r','i','t',' ','t','i','n','c','i','d','u','n','t',' ','e','s','t',' ','a','r','c','u',' ','a',' ','l','i','b','e','r','o','.',' ','I','n','t','e','r','d','u','m',' ','e','t',' ','m','a','l','e','s','u','a','d','a',' ','f','a','m','e','s',' ','a','c',' ','a','n','t','e',' ',            'i','p','s','u','m',' ','p','r','i','m','i','s',' ','i','n',' ','f','a','u','c','i','b','u','s','.',' ','C','u','r','a','b','i','t','u','r',' ','v','e','h','i','c','u','l','a',' ','m','a','g','n','a',' ','l','i','b','e','r','o',',',' ',
            'a','t',' ','r','h','o','n','c','u','s',' ','s','e','m',' ','o','r','n','a','r','e',' ','a','.',' ','I','n',' ','e','l','e','m','e','n','t','u','m',',',' ',
            'e','l','i','t',' ','e','t',' ','c','o','n','g','u','e',' ','p','u','l','v','i','n','a','r',',',' ',
            'm','a','s','s','a',' ','v','e','l','i','t',' ','c','o','m','m','o','d','o',' ','v','e','l','i','t',',',' ',
            'n','o','n',' ','e','l','e','m','e','n','t','u','m',' ','p','u','r','u','s',' ','l','i','g','u','l','a',' ','e','g','e','t',' ','l','a','c','u','s','.',' ','D','o','n','e','c',' ','e','f','f','i','c','i','t','u','r',' ','n','i','s','i',' ','e','u',' ','u','l','t','r','i','c','e','s',' ','e','f','f',            'i','c','i','t','u','r','.',' ','D','o','n','e','c',' ','n','e','q','u','e',' ','d','u','i',',',' ',
            'u','l','l','a','m','c','o','r','p','e','r',' ','i','d',' ','m','o','l','e','s','t','i','e',' ','q','u','i','s',',',' ',
            'c','o','n','s','e','q','u','a','t',' ','s','i','t',' ','a','m','e','t',' ','l','i','g','u','l','a','.',
            0xf7,
        };

        buffer.clear();
        buffer.resize(frameLength + 2);

        midi.begin();
        midi.sendSysEx(frameLength, reinterpret_cast<const byte*>(frame), false);
        EXPECT_EQ(serial.mTxBuffer.getLength(), frameLength + 2);
        serial.mTxBuffer.read(&buffer[0], frameLength + 2);
        EXPECT_THAT(buffer, ElementsAreArray(expected));
    }
}

TEST(MidiOutput, sendTimeCodeQuarterFrame)
{

}

TEST(MidiOutput, sendSongPosition)
{

}

TEST(MidiOutput, sendSongSelect)
{

}

TEST(MidiOutput, sendTuneRequest)
{

}

TEST(MidiOutput, sendRealTime)
{

}

TEST(MidiOutput, RPN)
{
    SerialMock serial;
    MidiInterface midi(serial);
    std::vector<test_mocks::uint8> buffer;

    // 14-bit Value Single Frame
    {
        buffer.clear();
        buffer.resize(13);

        midi.begin();
        midi.beginRpn(1242, 12);
        midi.sendRpnValue(12345, 12);
        midi.endRpn(12);

        EXPECT_EQ(serial.mTxBuffer.getLength(), 13);
        serial.mTxBuffer.read(&buffer[0], 13);
        EXPECT_THAT(buffer, ElementsAreArray({0xbb,
                                              0x64, 0x5a,
                                              0x65, 0x09,
                                              0x06, 0x60,
                                              0x26, 0x39,
                                              0x64, 0x7f,
                                              0x65, 0x7f}));
    }
    // MSB/LSB Single Frame
    {
        buffer.clear();
        buffer.resize(13);

        midi.begin();
        midi.beginRpn(1242, 12);
        midi.sendRpnValue(12, 42, 12);
        midi.endRpn(12);

        EXPECT_EQ(serial.mTxBuffer.getLength(), 13);
        serial.mTxBuffer.read(&buffer[0], 13);
        EXPECT_THAT(buffer, ElementsAreArray({0xbb,
                                              0x64, 0x5a,
                                              0x65, 0x09,
                                              0x06, 0x0c,
                                              0x26, 0x2a,
                                              0x64, 0x7f,
                                              0x65, 0x7f}));
    }
    // Increment Single Frame
    {
        buffer.clear();
        buffer.resize(11);

        midi.begin();
        midi.beginRpn(1242, 12);
        midi.sendRpnIncrement(42, 12);
        midi.endRpn(12);

        EXPECT_EQ(serial.mTxBuffer.getLength(), 11);
        serial.mTxBuffer.read(&buffer[0], 11);
        EXPECT_THAT(buffer, ElementsAreArray({0xbb,
                                              0x64, 0x5a,
                                              0x65, 0x09,
                                              0x60, 0x2a,
                                              0x64, 0x7f,
                                              0x65, 0x7f}));
    }
    // Decrement Single Frame
    {
        buffer.clear();
        buffer.resize(11);

        midi.begin();
        midi.beginRpn(1242, 12);
        midi.sendRpnDecrement(42, 12);
        midi.endRpn(12);

        EXPECT_EQ(serial.mTxBuffer.getLength(), 11);
        serial.mTxBuffer.read(&buffer[0], 11);
        EXPECT_THAT(buffer, ElementsAreArray({0xbb,
                                              0x64, 0x5a,
                                              0x65, 0x09,
                                              0x61, 0x2a,
                                              0x64, 0x7f,
                                              0x65, 0x7f}));
    }
    // Multi Frame
    {
        buffer.clear();
        buffer.resize(21);

        midi.begin();
        midi.beginRpn(1242, 12);
        midi.sendRpnValue(12345, 12);
        midi.sendRpnValue(12, 42, 12);
        midi.sendRpnIncrement(42, 12);
        midi.sendRpnDecrement(42, 12);
        midi.endRpn(12);

        EXPECT_EQ(serial.mTxBuffer.getLength(), 21);
        serial.mTxBuffer.read(&buffer[0], 21);
        EXPECT_THAT(buffer, ElementsAreArray({0xbb,
                                              0x64, 0x5a,
                                              0x65, 0x09,
                                              0x06, 0x60,
                                              0x26, 0x39,
                                              0x06, 0x0c,
                                              0x26, 0x2a,
                                              0x60, 0x2a,
                                              0x61, 0x2a,
                                              0x64, 0x7f,
                                              0x65, 0x7f}));
    }
}

TEST(MidiOutput, NRPN)
{
    SerialMock serial;
    MidiInterface midi(serial);
    std::vector<test_mocks::uint8> buffer;

    // 14-bit Value Single Frame
    {
        buffer.clear();
        buffer.resize(13);

        midi.begin();
        midi.beginNrpn(1242, 12);
        midi.sendNrpnValue(12345, 12);
        midi.endNrpn(12);

        EXPECT_EQ(serial.mTxBuffer.getLength(), 13);
        serial.mTxBuffer.read(&buffer[0], 13);
        EXPECT_THAT(buffer, ElementsAreArray({0xbb,
                                              0x62, 0x5a,
                                              0x63, 0x09,
                                              0x06, 0x60,
                                              0x26, 0x39,
                                              0x62, 0x7f,
                                              0x63, 0x7f}));
    }
    // MSB/LSB Single Frame
    {
        buffer.clear();
        buffer.resize(13);

        midi.begin();
        midi.beginNrpn(1242, 12);
        midi.sendNrpnValue(12, 42, 12);
        midi.endNrpn(12);

        EXPECT_EQ(serial.mTxBuffer.getLength(), 13);
        serial.mTxBuffer.read(&buffer[0], 13);
        EXPECT_THAT(buffer, ElementsAreArray({0xbb,
                                              0x62, 0x5a,
                                              0x63, 0x09,
                                              0x06, 0x0c,
                                              0x26, 0x2a,
                                              0x62, 0x7f,
                                              0x63, 0x7f}));
    }
    // Increment Single Frame
    {
        buffer.clear();
        buffer.resize(11);

        midi.begin();
        midi.beginNrpn(1242, 12);
        midi.sendNrpnIncrement(42, 12);
        midi.endNrpn(12);

        EXPECT_EQ(serial.mTxBuffer.getLength(), 11);
        serial.mTxBuffer.read(&buffer[0], 11);
        EXPECT_THAT(buffer, ElementsAreArray({0xbb,
                                              0x62, 0x5a,
                                              0x63, 0x09,
                                              0x60, 0x2a,
                                              0x62, 0x7f,
                                              0x63, 0x7f}));
    }
    // Decrement Single Frame
    {
        buffer.clear();
        buffer.resize(11);

        midi.begin();
        midi.beginNrpn(1242, 12);
        midi.sendNrpnDecrement(42, 12);
        midi.endNrpn(12);

        EXPECT_EQ(serial.mTxBuffer.getLength(), 11);
        serial.mTxBuffer.read(&buffer[0], 11);
        EXPECT_THAT(buffer, ElementsAreArray({0xbb,
                                              0x62, 0x5a,
                                              0x63, 0x09,
                                              0x61, 0x2a,
                                              0x62, 0x7f,
                                              0x63, 0x7f}));
    }
    // Multi Frame
    {
        buffer.clear();
        buffer.resize(21);

        midi.begin();
        midi.beginNrpn(1242, 12);
        midi.sendNrpnValue(12345, 12);
        midi.sendNrpnValue(12, 42, 12);
        midi.sendNrpnIncrement(42, 12);
        midi.sendNrpnDecrement(42, 12);
        midi.endNrpn(12);

        EXPECT_EQ(serial.mTxBuffer.getLength(), 21);
        serial.mTxBuffer.read(&buffer[0], 21);
        EXPECT_THAT(buffer, ElementsAreArray({0xbb,
                                              0x62, 0x5a,
                                              0x63, 0x09,
                                              0x06, 0x60,
                                              0x26, 0x39,
                                              0x06, 0x0c,
                                              0x26, 0x2a,
                                              0x60, 0x2a,
                                              0x61, 0x2a,
                                              0x62, 0x7f,
                                              0x63, 0x7f}));
    }
}

END_UNNAMED_NAMESPACE
