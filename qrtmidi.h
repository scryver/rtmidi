#ifndef QRTMIDI_H
#define QRTMIDI_H

#include <string>
#include <vector>
#include <QObject>
#include <QString>
#include <QStringList>
#include "RtMidi.h"

#define MIDI_NO_DEVICE_SELECTED "No midi device selected"
#define MIDI_NO_DEVICE_FOUND "No midi device found"

namespace MIDI {

struct Device
{
    explicit Device(int id = -1, QString name = MIDI_NO_DEVICE_SELECTED) {
        this->id = id;
        this->name = name;
    }

    int     id;
    QString name;
};

} // namespace MIDI

class QRtMidi : public QObject
{
    Q_OBJECT

public:
    QRtMidi(QObject *parent = 0);
    ~QRtMidi();

    // Getters
    const MIDI::Device& getInputDevice() const { return m_deviceIn; }
    const MIDI::Device& getOutputDevice() const { return m_deviceOut; }
    QStringList getInputDevices() const { return getDeviceList(m_midiIn); }
    QStringList getOutputDevices() const { return getDeviceList(m_midiOut); }

    // Emits the received signal, used in the RtMidi message callback
    void emitMidiMessage(double timeStamp, const std::vector<unsigned char>& message);

signals:
    void inputOpened();
    void inputClosed();
    void outputOpened();
    void outputClosed();
    // Received message from RtMidiIn callback
    void received(double timeStamp, const std::vector<unsigned char>& message);

public slots:
    // Send message to RtMidiOut
    void send(const std::vector<unsigned char>& message);
    void send(unsigned char status, unsigned char command);
    void send(unsigned char status, unsigned char command, unsigned char value);
    void setInputDevice(int id);
    void setInputDevice(QString name);
    void setOutputDevice(int id);
    void setOutputDevice(QString name);

private:
    RtMidiIn*       m_midiIn;
    RtMidiOut*      m_midiOut;

    MIDI::Device    m_deviceIn;
    MIDI::Device    m_deviceOut;

    void setDevice(int id, RtMidi* rtMidiHandler, MIDI::Device* device,
                   void (QRtMidi::*closeSignal)(),
                   void (QRtMidi::*openSignal)());
    void setDevice(QString name, RtMidi* rtMidiHandler, MIDI::Device* device,
                   void (QRtMidi::*closeSignal)(),
                   void (QRtMidi::*openSignal)());

    QStringList getDeviceList(RtMidi* rtMidiHandler) const;

    // data storage for sending of message
    std::vector<unsigned char> m_lastSend;
};

#endif // QRTMIDI_H
