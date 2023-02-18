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
#include "wiring_private.h"
#include "XBOXONE.h"
#include "adapter_structs.h"
#include "helpers.h"

#define MAX_CONTROL_TRANSFER_SIZE 64
#define VELOCITY_THRESH 10
#define USART_BAUDRATE 115200LL
#define USART_DOUBLE_SPEED false

#define OUT_DESCRIPTION "OUT: "
#define IN_DESCRIPTION " IN: "

// Declared weak in Arduino.h to allow user redefinitions.
int atexit(void (* /*func*/)()) {
    return 0;
}

MIDI_CREATE_DEFAULT_INSTANCE()

static XBPACKET      out_packet    = {.header = {true, 0}};
static XBPACKET      in_packet     = {.header = {true, 0}};
static ADAPTER_STATE adapter_state = none;

static DrumInputData  drum_state;
static output_state_t outputStates[NUM_OUT];

void forceHardReset(void) {
    cli();                  // disable interrupts
    wdt_enable(WDTO_15MS);  // enable watchdog
    while (1) {
    }  // wait for watchdog to reset processor
}

void XbPacketReceived(const uint8_t *data, const uint8_t &ndata) {
    if (ndata < sizeof(Frame))
        return;

    if (adapter_state == authenticating) {
        FillPacket(data, ndata, &out_packet);
        return;
    }

    if (adapter_state != running)
        return;

    auto frame = (Frame *)data;
    switch (frame->command) {
        case CMD_INPUT:
            FillInputPacketFromControllerData(data, ndata, &out_packet);
            break;
        default:
            break;
    }
}

static USB     Usb;
static XBOXONE Xbox(&Usb, XbPacketReceived);

void SetupHardware(void) {
    wdt_disable();
    init();

    Serial1.begin(USART_BAUDRATE);
    GlobalInterruptEnable();
    Serial1.print("\r\nHELLO WORLD\r\n");
    if (Usb.Init() == -1) {
        Serial1.print("\r\nOSC did not start");
        while (1)
            ;
    }
    USB_Init();

    adapter_state = init_state;
}

void EVENT_USB_Device_Connect(void) {}

void EVENT_USB_Device_Disconnect(void) {}

void EVENT_USB_Device_ConfigurationChanged(void) {
    Endpoint_ConfigureEndpoint(ADAPTER_IN_NUM, EP_TYPE_INTERRUPT, ADAPTER_IN_SIZE, 1);
    Endpoint_ConfigureEndpoint(ADAPTER_OUT_NUM, EP_TYPE_INTERRUPT, ADAPTER_OUT_SIZE, 1);
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
    printPacket(out_packet, OUT_DESCRIPTION);
    out_packet.header.handled = true;
}

const uint8_t drumreport2[] = {
    0x04, 0xf0, 0x01, 0x3a, 0xc5, 0x01, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xc5, 0x00, 0x9d, 0x00, 0x16, 0x00, 0x1b, 0x00, 0x1c, 0x00, 0x23, 0x00,
    0x29, 0x00, 0x6c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x06, 0x01, 0x02, 0x03, 0x04, 0x06, 0x07, 0x05, 0x01, 0x04, 0x05, 0x06, 0x0a, 0x02};

const uint8_t drumreport3[] = {
    0x04, 0xa0, 0x01, 0xba, 0x00, 0x3a, 0x17, 0x00, 0x4d, 0x61, 0x64, 0x43, 0x61, 0x74, 0x7a, 0x2e,
    0x58, 0x62, 0x6f, 0x78, 0x2e, 0x44, 0x72, 0x75, 0x6d, 0x73, 0x2e, 0x47, 0x6c, 0x61, 0x6d, 0x27,
    0x00, 0x57, 0x69, 0x6e, 0x64, 0x6f, 0x77, 0x73, 0x2e, 0x58, 0x62, 0x6f, 0x78, 0x2e, 0x49, 0x6e,
    0x70, 0x75, 0x74, 0x2e, 0x4e, 0x61, 0x76, 0x69, 0x67, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x43, 0x6f};

const uint8_t drumreport4[] = {
    0x04, 0xa0, 0x01, 0xba, 0x00, 0x74, 0x6e, 0x74, 0x72, 0x6f, 0x6c, 0x6c, 0x65, 0x72, 0x03, 0x93,
    0x28, 0x18, 0x06, 0xe0, 0xcc, 0x85, 0x4b, 0x92, 0x71, 0x0a, 0x10, 0xdb, 0xab, 0x7e, 0x07, 0xe7,
    0x1f, 0xf3, 0xb8, 0x86, 0x73, 0xe9, 0x40, 0xa9, 0xf8, 0x2f, 0x21, 0x26, 0x3a, 0xcf, 0xb7, 0x56,
    0xff, 0x76, 0x97, 0xfd, 0x9b, 0x81, 0x45, 0xad, 0x45, 0xb6, 0x45, 0xbb, 0xa5, 0x26, 0xd6, 0x01};

const uint8_t drumreport5[] = {0x04, 0xb0, 0x01, 0x17, 0xae, 0x01, 0x17, 0x00, 0x20, 0x0a, 0x00,
                               0x01, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

const uint8_t drumreport6[] = {0x04, 0xa0, 0x01, 0x00, 0xc5, 0x01, 0x00, 0x00};

const struct {
    uint8_t        size;
    const uint8_t *data;
} identify_packets[] = {{sizeof(drumreport2), drumreport2},
                        {sizeof(drumreport3), drumreport3},
                        {sizeof(drumreport4), drumreport4},
                        {sizeof(drumreport5), drumreport5},
                        {sizeof(drumreport6), drumreport6}};

const uint8_t ack[] = {0x01, 0x20, 0x05, 0x09, 0x00, 0x04, 0x20,
                       0xda, 0x00, 0x00, 0x00, 0x00, 0x00};

static void noteOn(uint8_t note, uint8_t velocity) {
    if (velocity <= VELOCITY_THRESH)
        return;

    output_t out = outputForNote(note);
    if (out == NO_OUT)
        return;

    while (!out_packet.header.handled) {
        USB_USBTask();
        SendNextReport();
    }

    switch (out) {
        case OUT_KICK:
            drum_state.kick = 1;
            break;
        case OUT_PAD_RED:
            drum_state.padRed = 1;
            break;
        case OUT_PAD_BLUE:
            drum_state.padBlue = 1;
            break;
        case OUT_PAD_GREEN:
            drum_state.padGreen = 1;
            break;
        case OUT_PAD_YELLOW:
            drum_state.padYellow = 1;
            break;
        case OUT_CYM_YELLOW:
            drum_state.cymbalYellow = 1;
            break;
        case OUT_CYM_BLUE:
            drum_state.cymbalBlue = 1;
            break;
        case OUT_CYM_GREEN:
            drum_state.cymbalGreen = 1;
            break;
        default:
            break;
    }

    drum_state.command  = CMD_INPUT;
    drum_state.type     = TYPE_COMMAND;
    drum_state.deviceId = 0;
    drum_state.sequence = getSequence();
    drum_state.length   = sizeof(DrumInputData) - sizeof(Frame);

    auto packet = &out_packet;
    memset(packet->buf.buffer, 0, sizeof(packet->buf.buffer));
    packet->header.length  = sizeof(DrumInputData);
    packet->header.handled = false;
    memcpy(&packet->buf.drum_input, &drum_state, sizeof(drum_state));

    outputStates[out].triggered   = true;
    outputStates[out].triggeredAt = millis();
    return;
}

static void HandlePacketAuth(XBPACKET &packet) {
    if (packet.buf.frame.command == CMD_AUTHENTICATE && packet.header.length == 6 &&
        packet.buf.buffer[3] == 2 && packet.buf.buffer[4] == 1 && packet.buf.buffer[5] == 0) {
        Serial1.print("AUTHENTICATED!\r\n");
        adapter_state = running;
        // MIDI.setHandleNoteOn(noteOn);
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
            FillPacket(identify_packets[identify_sequence].data,
                       identify_packets[identify_sequence].size, &out_packet);
            packet.header.handled = true;
            break;
        case CMD_AUTHENTICATE:
            Serial1.print("Moving to Authenticate\r\n");
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
            Serial1.print("Moving to Identify\r\n");
            adapter_state = identifying;
            return HandlePacketIdentify(packet);
        default:
            break;
    }
}

void HandlePacketRunning(XBPACKET &packet) {
    // i'm sure i'll figure out something to do here :^)
}

void HandlePacket(XBPACKET &packet) {
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

void ReceiveNextReport(void) {
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
            printPacket(in_packet, IN_DESCRIPTION);
            HandlePacket(in_packet);
        }
    }
}

void HID_Task(void) {
    if (USB_DeviceState != DEVICE_STATE_Configured)
        return;

    SendNextReport();
    ReceiveNextReport();
}

void DoTasks() {
    Usb.Task();
    HID_Task();
    USB_USBTask();
}

const uint8_t announce[] = {0x02, 0x20, 0x01, 0x1c, 0x7e, 0xed, 0x82, 0x8b, 0xec, 0x97, 0x00,
                            0x00, 0x38, 0x07, 0x62, 0x42, 0x01, 0x00, 0x00, 0x00, 0xe6, 0x00,
                            0x00, 0x00, 0x00, 0x02, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00};

const uint16_t announce_interval = 2000;

#define TRIGGER_HOLD 25

int main(void) {
    SetupHardware();
    unsigned long last_announce_time = 0, current_time = millis();
    while (adapter_state == init_state) {
        DoTasks();
        current_time = millis();
        if ((current_time - last_announce_time) > announce_interval) {
            FillPacket(announce, sizeof(announce), &out_packet);
            last_announce_time = millis();
        }
    }

    while (adapter_state != running) {
        DoTasks();
    }
    // MIDI.begin(MIDI_CHANNEL_OMNI);
    while (true) {
        DoTasks();
        current_time = millis();
        if ((current_time - last_announce_time) > announce_interval) {
            noteOn(38, 50);
            last_announce_time = current_time;
        }  // you can technically add a note ON handler in the midi library, but i like this
           // better
        if (outputStates[OUT_PAD_RED].triggered &&
            current_time - outputStates[OUT_PAD_RED].triggeredAt > TRIGGER_HOLD)
            drum_state.padRed = 0;
    }
}

const uint8_t PROGMEM request144_index_4[] = {
    0x28, 0x00, 0x00, 0x00, 0x00, 0x01, 0x04, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01, 0x58, 0x47, 0x49, 0x50, 0x31, 0x30, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

void EVENT_USB_Device_ControlRequest(void) {
    if (USB_ControlRequest.bmRequestType & REQTYPE_VENDOR) {
        if (USB_ControlRequest.bmRequestType & REQDIR_DEVICETOHOST) {
            if (USB_ControlRequest.bRequest == 144) {
                uint8_t recipient = USB_ControlRequest.bmRequestType & 0x1F;
                if (recipient == REQREC_DEVICE) {
                    if (USB_ControlRequest.wIndex == 0x0004) {
                        Endpoint_ClearSETUP();
                        Endpoint_Write_Control_PStream_LE(request144_index_4,
                                                          USB_ControlRequest.wLength);
                        Endpoint_ClearOUT();
                    }
                } else if (recipient == REQREC_INTERFACE) {
                    if (USB_ControlRequest.wIndex == 0x0005) {
                        Endpoint_ClearSETUP();
                        Endpoint_ClearOUT();
                    }
                }
            }
        }
    } else {
        if (USB_ControlRequest.bmRequestType & REQREC_INTERFACE) {
            if (USB_ControlRequest.bmRequestType & REQDIR_DEVICETOHOST) {
                if (USB_ControlRequest.bRequest == REQ_GetInterface) {
                    uint8_t data[1] = {0x00};
                    Endpoint_ClearSETUP();
                    Endpoint_Write_Control_Stream_LE(data, sizeof(data));
                    Endpoint_ClearOUT();
                }
            } else {
                if (USB_ControlRequest.bRequest == REQ_SetInterface) {
                    Endpoint_ClearSETUP();
                    // wLength is 0
                    Endpoint_ClearIN();
                }
            }
        }
    }
}
