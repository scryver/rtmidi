#include "qrtmidi.h"

#include <QLoggingCategory>
#include <QDebug>

QLoggingCategory midiLogCat("midi");

void midiError(RtMidiError::Type type, const std::string& errorText,
               void *userData = nullptr) {
    switch (type) {
        case RtMidiError::WARNING:
            qCWarning(midiLogCat) << errorText.c_str();
            break;
        case RtMidiError::DEBUG_WARNING:
            qCDebug(midiLogCat) << errorText.c_str();
            break;
        case RtMidiError::UNSPECIFIED:
        case RtMidiError::NO_DEVICES_FOUND:
        case RtMidiError::INVALID_DEVICE:
        case RtMidiError::INVALID_PARAMETER:
        case RtMidiError::INVALID_USE:
            qCCritical(midiLogCat) << errorText.c_str();
            break;
        case RtMidiError::MEMORY_ERROR:
        case RtMidiError::DRIVER_ERROR:
        case RtMidiError::SYSTEM_ERROR:
        case RtMidiError::THREAD_ERROR:
            qFatal("%s", errorText.c_str());
            break;
    }
}

void midiCallback(double timeStamp, std::vector<unsigned char> *message,
                  void *userData) {
    QRtMidi* qtMidiObj = reinterpret_cast<QRtMidi*>(userData);
    qtMidiObj->emitMidiMessage(timeStamp, *message);
}

QRtMidi::QRtMidi(QObject *parent) : QObject(parent),
        m_midiIn(nullptr),
        m_midiOut(nullptr)
{
    try {
        m_midiIn = new RtMidiIn(RtMidi::UNSPECIFIED, "QRtMidiIn", 256);
        m_midiIn->setErrorCallback(&midiError);
        m_midiIn->setCallback(&midiCallback, this);
    } catch (RtMidiError& e) {
        midiError(e.getType(), e.getMessage());
        m_midiIn = nullptr;
    }

    try {
        m_midiOut = new RtMidiOut(RtMidi::UNSPECIFIED, "QRtMidiOut");
        m_midiOut->setErrorCallback(&midiError);
    } catch (RtMidiError& e) {
        midiError(e.getType(), e.getMessage());
        m_midiOut = nullptr;
    }
}

QRtMidi::~QRtMidi() {
    if (m_midiIn != nullptr) {
        delete m_midiIn;
        m_midiIn = nullptr;
    }

    if (m_midiOut != nullptr) {
        delete m_midiOut;
        m_midiOut = nullptr;
    }
}

void QRtMidi::setInputDevice(int id) {
    if (id != m_deviceIn.id) {
        setDevice(id, m_midiIn, &m_deviceIn,
                  &QRtMidi::inputClosed, &QRtMidi::inputOpened);
    }
}
void QRtMidi::setInputDevice(QString name) {
    if (name != m_deviceIn.name) {
        setDevice(name, m_midiIn, &m_deviceIn,
                  &QRtMidi::inputClosed, &QRtMidi::inputOpened);
    }
}
void QRtMidi::setOutputDevice(int id) {
    if (id != m_deviceOut.id) {
        setDevice(id, m_midiOut, &m_deviceOut,
                  &QRtMidi::outputClosed, &QRtMidi::outputOpened);
    }
}
void QRtMidi::setOutputDevice(QString name) {
    if (name != m_deviceOut.name) {
        setDevice(name, m_midiOut, &m_deviceOut,
                  &QRtMidi::outputClosed, &QRtMidi::outputOpened);
    }
}

void QRtMidi::emitMidiMessage(double timeStamp,
                              const std::vector<unsigned char>& message) {
//    m_lastReceived.clear();
//    for (size_t i = 0; i < message->size(); i++) {
//        m_lastReceived.push_back(message->at(i));
//    }
    emit received(timeStamp, message);
}

void QRtMidi::setDevice(int id, RtMidi* rtMidiHandler, MIDI::Device* device,
                        void (QRtMidi::*closeSignal)(),
                        void (QRtMidi::*openSignal)()) {
    if (rtMidiHandler == nullptr) {
        return;
    }

    rtMidiHandler->closePort();
    emit (this->*closeSignal)();
    if (id >= 0 && id < static_cast<int>(rtMidiHandler->getPortCount())) {
        rtMidiHandler->openPort(id);
        device->id = id;
        device->name = (rtMidiHandler->getPortName(static_cast<unsigned int>(id))).c_str();
        emit (this->*openSignal)();
    }
}

void QRtMidi::setDevice(QString name, RtMidi* rtMidiHandler,
                        MIDI::Device* device,
                        void (QRtMidi::*closeSignal)(),
                        void (QRtMidi::*openSignal)()) {
    if (rtMidiHandler == nullptr) {
        return;
    }

    unsigned int numPorts = rtMidiHandler->getPortCount();
    QString portName;
    int deviceID = -1;
    for (unsigned int i = 0; i < numPorts; i++) {
        portName = (rtMidiHandler->getPortName(i)).c_str();
        if (portName == name) {
            deviceID = static_cast<int>(i);
            break;
        }
    }

    setDevice(deviceID, rtMidiHandler, device, closeSignal, openSignal);
}

QStringList QRtMidi::getDeviceList(RtMidi* rtMidiHandler) const {
    QStringList driverNames;
    driverNames.clear();
    if (rtMidiHandler == nullptr) {
        return driverNames;
    }

    unsigned int numPorts = rtMidiHandler->getPortCount();

    driverNames << QString(tr(MIDI_NO_DEVICE_SELECTED));
    for (unsigned int i = 0; i < numPorts; i++) {
        driverNames << QString::fromStdString(rtMidiHandler->getPortName(i));
    }

    if (driverNames.size() == 1) {
        driverNames.clear();
        driverNames << QString(tr(MIDI_NO_DEVICE_FOUND));
    }

    return driverNames;
}

void QRtMidi::send(const std::vector<unsigned char>& message) {
    if (m_midiOut == nullptr) {
        return;
    }

    m_lastSend = message;
    m_midiOut->sendMessage(&m_lastSend);
}

void QRtMidi::send(unsigned char status, unsigned char command) {
    std::vector<unsigned char> message = { status, command };
    send(message);
}

void QRtMidi::send(unsigned char status, unsigned char command, unsigned char value) {
    std::vector<unsigned char> message = { status, command, value };
    send(message);
}
