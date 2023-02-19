#include <stdint.h>
#include <stdbool.h>

#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/io.h>
#include <stdlib.h>

#include <LUFA/Drivers/USB/USB.h>

#include "Arduino.h"
#include "HardwareSerial.h"
#include "MIDI.h"
#include "Config/AdapterConfig.h"
#include "XBOXONE.h"
#include "adapter_structs.h"
#include "packet_helpers.h"
#include "Identifiers.hpp"

#ifdef SERIAL_DEBUG
#include "debug_helpers.h"
#endif

// Declared weak in Arduino.h to allow user redefinitions.
int atexit(void (* /*func*/)()) {
    return 0;
}

MIDI_CREATE_DEFAULT_INSTANCE()

static XBPACKET      out_packet    = {.header = {true, 0}};
static XBPACKET      in_packet     = {.header = {true, 0}};
static ADAPTER_STATE adapter_state = none;

static uint8_t        drum_state_flag = no_flag;
static DrumInputData  drum_state;
static output_state_t outputStates[NUM_OUT];
static USB            Usb;

void xbPacketReceivedCB(const uint8_t *data, const uint8_t &ndata) {
    if (ndata < sizeof(Frame))
        return;

    if (adapter_state == authenticating) {
        fillPacket(data, ndata, &out_packet);
        return;
    }

    if (adapter_state != running)
        return;

    auto frame = (Frame *)data;
    switch (frame->command) {
        case CMD_INPUT:
            fillInputPacketFromControllerData(data, ndata, &out_packet);
            break;
        default:
            break;
    }
}

static XBOXONE Xbox(&Usb, xbPacketReceivedCB);

#if 0
static void forceHardReset(void) {
    cli();                  // disable interrupts
    wdt_enable(WDTO_15MS);  // enable watchdog
    while (1) {
    }  // wait for watchdog to reset processor
}
#endif

static void noteOn(uint8_t note, uint8_t velocity) {
    if (velocity <= VELOCITY_THRESH)
        return;

    output_t out = outputForNote(note);
    if (out == NO_OUT)
        return;

    if (outputStates[out].triggered)
        return;

    drum_state.command  = CMD_INPUT;
    drum_state.type     = TYPE_COMMAND;
    drum_state.deviceId = 0;
    drum_state.length   = sizeof(DrumInputData) - sizeof(Frame);
    updateDrumStateWithDrumInput(out, 1, &drum_state);

    drum_state_flag |= changed_flag;

    outputStates[out].triggered   = true;
    outputStates[out].triggeredAt = millis();
    return;
}

static void HandlePacketAuth(XBPACKET &packet) {
    if (packet.buf.frame.command == CMD_AUTHENTICATE && packet.header.length == 6 &&
        packet.buf.buffer[3] == 2 && packet.buf.buffer[4] == 1 && packet.buf.buffer[5] == 0) {
        digitalWrite(LED_BUILTIN, HIGH);
#ifdef SERIAL_DEBUG
        SERIAL_DEBUG.print("AUTHENTICATED!\r\n");
#endif
        adapter_state = running;
        return;
    }

    while (Xbox.XboxCommand(packet.buf.buffer, packet.header.length) != 0) {
        Usb.Task();
    }
    return;
}

static void HandlePacketIdentify(XBPACKET &packet) {
    static uint8_t identify_sequence = 0;
    switch (packet.buf.frame.command) {
        case CMD_IDENTIFY:
        case CMD_ACKNOWLEDGE:
            if (identify_sequence >= sizeof(identify_packets))
                identify_sequence = 0;
            fillPacket(identify_packets[identify_sequence].data,
                       identify_packets[identify_sequence].size, &out_packet);
            packet.header.handled = true;
            break;
        case CMD_AUTHENTICATE:
#ifdef SERIAL_DEBUG
            SERIAL_DEBUG.print("Moving to Authenticate\r\n");
#endif
            adapter_state = authenticating;
            return HandlePacketAuth(packet);
            break;
        default:
            break;
    }
    identify_sequence++;
    return;
}

static void HandlePacketInit(XBPACKET &packet) {
    switch (packet.buf.frame.command) {
        case CMD_IDENTIFY:
#ifdef SERIAL_DEBUG
            SERIAL_DEBUG.print("Moving to Identify\r\n");
#endif
            adapter_state = identifying;
            return HandlePacketIdentify(packet);
        default:
            break;
    }
}

static void HandlePacketRunning(XBPACKET &packet) {
    // i'm sure i'll figure out something to do here :^)
    return;
}

static void HandlePacket(XBPACKET &packet) {
    switch (adapter_state) {
        case none:
            return;
        case init_state:
            return HandlePacketInit(packet);
        case identifying:
            return HandlePacketIdentify(packet);
        case authenticating:
            return HandlePacketAuth(packet);
        case running:
            return HandlePacketRunning(packet);
        default:
            break;
    }
    return;
}

static void ReceiveNextReport(void) {
    Endpoint_SelectEndpoint(ADAPTER_OUT_NUM);

    if (Endpoint_IsOUTReceived()) {
        uint16_t length = 0;

        if (Endpoint_IsReadWriteAllowed()) {
            uint8_t ErrorCode = Endpoint_Read_Stream_LE(in_packet.buf.buffer,
                                                        sizeof(in_packet.buf.buffer), &length);

            in_packet.header.handled = false;
            in_packet.header.length =
                (ErrorCode == ENDPOINT_RWSTREAM_NoError) ? sizeof(in_packet.buf.buffer) : length;
        }

        Endpoint_ClearOUT();
        if (in_packet.header.length && !in_packet.header.handled) {
#ifdef SERIAL_DEBUG
            printPacket(in_packet, IN_DESCRIPTION);
#endif
            HandlePacket(in_packet);
        }
    }
}

void SendNextReport(void) {
    Endpoint_SelectEndpoint(ADAPTER_IN_NUM);

    if (!Endpoint_IsINReady()) {
        return;
    }
    if (out_packet.header.handled)
        return;

    Endpoint_Write_Stream_LE(out_packet.buf.buffer, out_packet.header.length, NULL);
    Endpoint_ClearIN();
#ifdef SERIAL_DEBUG
    printPacket(out_packet, OUT_DESCRIPTION);
#endif
    out_packet.header.handled = true;
}

static void HID_Task(void) {
    if (USB_DeviceState != DEVICE_STATE_Configured)
        return;

    SendNextReport();
    ReceiveNextReport();
}

static void MIDI_Task() {
    while (MIDI.read()) {
        auto location_byte = MIDI.getType();
        auto note          = MIDI.getData1();
        auto velocity      = MIDI.getData2();
        if (location_byte == midi::MidiType::NoteOn) {
            noteOn(note, velocity);
        }
    }
}

static void DRUM_STATE_Task() {
    if (adapter_state != running)
        return;
    auto current_time = millis();
    for (auto out = 0; out < NUM_OUT; out++) {
        if (outputStates[out].triggered &&
            current_time - outputStates[out].triggeredAt > TRIGGER_HOLD_MS) {
            updateDrumStateWithDrumInput(static_cast<output_t>(out), 0, &drum_state);
            outputStates[out].triggered = false;
            drum_state_flag |= changed_flag;
        }
    }

    if (drum_state_flag & changed_flag) {
        drum_state.sequence = getSequence();
        fillPacket((uint8_t *)&drum_state, sizeof(drum_state), &out_packet);
        drum_state_flag = no_flag;
    }
}

static inline void DoTasks() {
    Usb.Task();
    HID_Task();
    USB_USBTask();
#ifndef SERIAL_DEBUG
    MIDI_Task();
    DRUM_STATE_Task();
#endif
}

static void SetupHardware(void) {
    wdt_disable();
    init();
#ifdef SERIAL_DEBUG
    SERIAL_DEBUG.begin(DEBUG_USART_BAUDRATE);
    SERIAL_DEBUG.print("\r\nHELLO WORLD\r\n");
#endif
    GlobalInterruptEnable();
    if (Usb.Init() == -1) {
#ifdef SERIAL_DEBUG
        SERIAL_DEBUG.print("\r\nOSC did not start");
#endif
        while (1)
            ;
    }
#ifndef SERIAL_DEBUG
    MIDI.begin(MIDI_CHANNEL_OMNI);
#endif
    USB_Init();
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    adapter_state = init_state;
}

int main(void) {
    SetupHardware();

    unsigned long last_announce_time = 0, current_time = millis();
    while (adapter_state == init_state) {
        DoTasks();
        current_time = millis();
        if (((current_time - last_announce_time) > ANNOUNCE_INTERVAL_MS) && Xbox.XboxOneConnected) {
            fillPacket(announce, sizeof(announce), &out_packet);
            last_announce_time = millis();
        }
    }

    while (true) {
        DoTasks();
    }
}
