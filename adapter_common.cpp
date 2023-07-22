#include <stdint.h>
#include <stdbool.h>

#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/io.h>
#include <stdlib.h>

#include <LUFA/Drivers/USB/USB.h>

#include "Arduino.h"
#include "HardwareSerial.h"
#include "Config/adapter_config.h"
#include "XBOXONE.h"
#include "usbh_midi.h"
#include "usbhub.h"
#include "adapter_structs.h"
#include "packet_helpers.h"
#include "packet_circ_buf.h"
#include "Config/adapter_identifiers.h"

// Declared weak in Arduino.h to allow user redefinitions.
int atexit(void (* /*func*/)()) {
    return 0;
}
#ifdef SERIAL_DEBUG
#include "debug_helpers.h"
#else
#include "MIDI.h"
MIDI_CREATE_DEFAULT_INSTANCE()
#endif

#ifdef SERIAL_DEBUG
#define debug(msg) Notify(PSTR(msg), 0x80);
#else
#define debug(msg) (void)0
#endif

static xb_packet_t     in_packet     = {.header = {true, 0}};
static adapter_state_t adapter_state = none;

static uint8_t          drum_state_flag = no_flag;
static drum_input_pkt_t drum_state;
static output_state_t   outputStates[NUM_OUT];

static USB    Usb;
static USBHub UsbHub(&Usb);

void xbPacketReceivedCB(uint8_t *data, const uint8_t &ndata);

static XBOXONE   Xbox(&Usb, xbPacketReceivedCB);
static USBH_MIDI UsbMidi(&Usb);

void xbPacketReceivedCB(uint8_t *data, const uint8_t &ndata) {
    if (ndata < sizeof(frame_pkt_t))
        return;
    auto frame = reinterpret_cast<frame_pkt_t *>(data);
    switch (adapter_state) {
        case authenticating:
            fillPacket(data, ndata, packet_circ_buf::get_write());
            return;
        case power_off:
            // move back to init if someone hits the guide button - technically you need to ACK
            // these but the xbox will do that once we're authenticating
            if (frame->command == CMD_GUIDE_BTN) {
                adapter_state = init_state;
            }
            return;
        case running:
            switch (frame->command) {
                case CMD_GUIDE_BTN:
                    frame->sequence = getSequence();
                    fillPacket(data, ndata, packet_circ_buf::get_write());
                    return;
                case CMD_INPUT:
                    fillInputPacketFromControllerData(data, ndata, packet_circ_buf::get_write());
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}

static void forceHardReset(void) {
    cli();                  // disable interrupts
    wdt_enable(WDTO_15MS);  // enable watchdog
    while (1) {
    }                       // wait for watchdog to reset processor
}

static void noteOn(uint8_t note, uint8_t velocity) {
    if (velocity <= VELOCITY_THRESH)
        return;

    output_t out = outputForNote(note);
    if (out == NO_OUT)
        return;

    if (outputStates[out].triggered)
        return;

    updateDrumStateWithDrumInput(static_cast<output_t>(out), 1, &drum_state);
    drum_state_flag |= changed_flag;

    outputStates[out].triggered   = true;
    outputStates[out].triggeredAt = millis();
    return;
}

static void HandlePacketAuth(xb_packet_t &packet) {
    if (packet.buf.frame.command == CMD_AUTHENTICATE && packet.header.length == 6 &&
        packet.buf.buffer[3] == 2 && packet.buf.buffer[4] == 1 && packet.buf.buffer[5] == 0) {
        digitalWrite(LED_BUILTIN, HIGH);
        debug("AUTHENTICATED!\r\n");
        adapter_state = running;
    }

    while (Xbox.XboxCommand(packet.buf.buffer, packet.header.length) != 0) {
        Usb.Task();
    }
    return;
}

static void HandlePacketIdentify(xb_packet_t &packet) {
    static uint8_t identify_sequence = 0;
    switch (packet.buf.frame.command) {
        case CMD_IDENTIFY:
        case CMD_ACKNOWLEDGE:
            if (identify_sequence >= identifiers::get_n_identify()) {
                debug("Starting identify sequence over\r\n");
                identify_sequence = 0;
            }
            identifiers::get_identify(identify_sequence, packet_circ_buf::get_write());
            identify_sequence++;
            break;
        case CMD_AUTHENTICATE:
            debug("Moving to Authenticate\r\n");
            adapter_state = authenticating;
            return HandlePacketAuth(packet);
            break;
        default:
            break;
    }
    return;
}

static void HandlePacketInit(xb_packet_t &packet) {
    switch (packet.buf.frame.command) {
        case CMD_IDENTIFY:
            debug("Moving to Identify\r\n");
            adapter_state = identifying;
            return HandlePacketIdentify(packet);
        default:
            break;
    }
}

static void HandlePacketRunning(xb_packet_t &packet) {
    switch (packet.buf.frame.command) {
        case CMD_POWER_MODE:
            if (packet.buf.buffer[sizeof(frame_pkt_t)] == POWER_OFF) {
                // xbox has told us to power off for some reason, move to power_off state and turn
                // off the indicator LED
                debug("Powering down... \r\n");
                digitalWrite(LED_BUILTIN, LOW);
                adapter_state = power_off;
            }
            break;
        case CMD_ACKNOWLEDGE:
            // pass ACK's to controller - this is pretty much only for the guide button to work
            // correctly
            while (Xbox.XboxCommand(packet.buf.buffer, packet.header.length) != 0) {
                Usb.Task();
            }
            break;
        default:
            break;
    }
    return;
}

static void HandlePacket(xb_packet_t &packet) {
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

            in_packet.header.length =
                (ErrorCode == ENDPOINT_RWSTREAM_NoError) ? sizeof(in_packet.buf.buffer) : length;
        }

        Endpoint_ClearOUT();
        if (in_packet.header.length) {
#ifdef SERIAL_DEBUG
            printPacket(in_packet, IN_DESCRIPTION);
#endif
            HandlePacket(in_packet);
        }
    }
}

static void SendNextReport(void) {
    Endpoint_SelectEndpoint(ADAPTER_IN_NUM);

    auto         current_time = millis();
    xb_packet_t *out_packet   = packet_circ_buf::get_read();
    while (out_packet && ((current_time - out_packet->header.triggered_time) > ON_DELAY_MS)) {
        if (!Endpoint_IsINReady()) {
            return;
        }

        Endpoint_Write_Stream_LE(out_packet->buf.buffer, out_packet->header.length, NULL);
        Endpoint_ClearIN();
#ifdef SERIAL_DEBUG
        printPacket(*out_packet, OUT_DESCRIPTION);
#endif
        packet_circ_buf::incr_read();
        out_packet = packet_circ_buf::get_read();
    }
}

static void HID_Task(void) {
    if (USB_DeviceState != DEVICE_STATE_Configured)
        return;

    SendNextReport();
    ReceiveNextReport();
}

static void MIDI_Task() {
#ifndef SERIAL_DEBUG
    while (MIDI.read()) {
        auto location_byte = MIDI.getType();
        auto note          = MIDI.getData1();
        auto velocity      = MIDI.getData2();
        if (location_byte == midi::MidiType::NoteOn) {
            noteOn(note, velocity);
        }
    }
#endif
    if (UsbMidi.UsbMidiConnected) {
        uint8_t outBuf[3], size;
        if ((size = UsbMidi.RecvData(outBuf)) > 0)
            // filter top four bits for "noteon" without channel data
            if ((outBuf[0] >> 4) == 0x9) {
                noteOn(outBuf[1], outBuf[2]);
            }
    }
}

static void DRUM_STATE_Task() {
    if (adapter_state != running)
        return;

    auto current_time = millis();
    for (auto out = 0; out < NUM_OUT; out++) {
        if (!outputStates[out].triggered) {
            continue;
        }

        auto time_since_trigger = current_time - outputStates[out].triggeredAt;
        if (time_since_trigger > TRIGGER_HOLD_MS) {
            updateDrumStateWithDrumInput(static_cast<output_t>(out), 0, &drum_state);
            outputStates[out].triggered = false;
            drum_state_flag |= changed_flag;
        }
    }

    if (drum_state_flag & changed_flag) {
        drum_state.sequence = getSequence();
        fillPacket(reinterpret_cast<uint8_t *>(&drum_state), sizeof(drum_state),
                   packet_circ_buf::get_write(), current_time);
        drum_state_flag = no_flag;
    }
}

static inline void DoTasks() {
    Usb.Task();
    HID_Task();
    USB_USBTask();
    MIDI_Task();
    DRUM_STATE_Task();
}

static void SetupHardware(void) {
    wdt_disable();
    init();
#ifdef SERIAL_DEBUG
    SERIAL_DEBUG.begin(DEBUG_USART_BAUDRATE);
#endif
    debug("\r\nopenrb-instruments, built with serial debug enabled\r\nstarting...\r\n");
    if (Usb.Init() == -1) {
        debug("\r\nOSC did not start");
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

    drum_state.command  = CMD_INPUT;
    drum_state.type     = TYPE_COMMAND;
    drum_state.deviceId = 0;
    drum_state.length   = sizeof(drum_input_pkt_t) - sizeof(frame_pkt_t);

    unsigned long last_announce_time = 0, current_time = millis();
    for (;;) {
        DoTasks();
        if (adapter_state == init_state) {
            current_time = millis();
            if ((current_time - last_announce_time) > ANNOUNCE_INTERVAL_MS) {
                if (Xbox.XboxOneConnected) {
                    debug("ANNOUNCING\r\n");
                    identifiers::get_announce(packet_circ_buf::get_write());
                    last_announce_time = millis();
                } else {
                    debug("WAITING FOR CONTROLLER\r\n");
                }
            }
        }
    }
}
