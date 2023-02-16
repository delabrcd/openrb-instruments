/*
 Copyright (c) 2019 Mathieu Laurendeau
 License: GPLv3
 */

#include <stdint.h>
#include <stdbool.h>

#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/io.h>
#include <stdlib.h>

#include <LUFA/Drivers/Peripheral/Serial.h>
#include <LUFA/Drivers/USB/USB.h>

#include "../adapter_protocol.h"
#include "Config/AdapterConfig.h"
#include "wiring_private.h"
#include "XBOXONE.h"

#ifndef ADAPTER_TYPE
#error ADAPTER_TYPE is not defined!
#endif

#ifndef ADAPTER_IN_NUM
#error ADAPTER_IN_NUM is not defined!
#endif

#ifndef ADAPTER_IN_SIZE
#error ADAPTER_IN_SIZE is not defined!
#endif

#ifndef ADAPTER_IN_INTERVAL
#error ADAPTER_IN_INTERVAL is not defined!
#endif

#ifdef ADAPTER_OUT_NUM
#ifndef ADAPTER_OUT_SIZE
#error ADAPTER_OUT_SIZE is not defined!
#endif

#ifndef ADAPTER_OUT_INTERVAL
#error ADAPTER_OUT_INTERVAL is not defined!
#endif
#endif

#define REQ_GetReport 0x01
#define REQ_SetReport 0x09
#define REQ_SetIdle 0x0A

#define REPORT_TYPE_FEATURE 0x03

#define MAX_CONTROL_TRANSFER_SIZE 64

#define USART_BAUDRATE 115200L
#define USART_DOUBLE_SPEED false
// Declared weak in Arduino.h to allow user redefinitions.
int atexit(void (* /*func*/)()) {
    return 0;
}
const uint8_t version_major = 8;
const uint8_t version_minor = 0;

static uint8_t report[ADAPTER_IN_SIZE] = {};

static uint8_t buf[MAX_CONTROL_TRANSFER_SIZE];

static uint8_t *pdata;
// static uint8_t  i = 0;

/*
 * These variables are used in both the main and serial interrupt,
 * therefore they have to be declared as volatile.
 */
static volatile uint8_t sendReport        = 0;
static volatile uint8_t reportLen         = 0;
static volatile uint8_t started           = 0;
static volatile uint8_t packet_type       = 0;
static volatile uint8_t value_len         = 0;
static volatile uint8_t spoofReply        = 0;
static volatile uint8_t spoofReplyLen     = 0;
static volatile uint8_t spoof_initialized = BYTE_STATUS_NSPOOFED;
volatile uint16_t       vid               = 0;
volatile uint16_t       pid               = 0;
static volatile uint8_t baudrate          = USART_BAUDRATE;
static volatile uint8_t sent_count        = 0;

// A linked list (LL) node to store a queue entry
struct QNode {
    uint8_t        packet_len;
    const uint8_t *packet;
    struct QNode  *next;
};

// The queue, front stores the front node of LL and rear
// stores the last node of LL
struct Queue {
    struct QNode *front, *rear;
};

struct Queue *packet_queue = NULL;

// A utility function to create a new linked list node.
struct QNode *newNode(const uint8_t *packet, const uint8_t &packet_len) {
    struct QNode *temp = (struct QNode *)malloc(sizeof(struct QNode));
    temp->packet       = packet;
    temp->packet_len   = packet_len;
    temp->next         = NULL;
    return temp;
}

// A utility function to create an empty queue
struct Queue *createQueue() {
    struct Queue *q = (struct Queue *)malloc(sizeof(struct Queue));
    q->front = q->rear = NULL;
    return q;
}

// The function to add a key k to q
void enQueue(struct Queue *q, const uint8_t *packet, const uint8_t &packet_len) {
    // Create a new LL node
    struct QNode *temp = newNode(packet, packet_len);
    sendReport++;

    // If queue is empty, then new node is front and rear
    // both
    if (q->rear == NULL) {
        q->front = q->rear = temp;
        return;
    }
    // Add the new node at the end of queue and change rear
    q->rear->next = temp;
    q->rear       = temp;
}

// Function to remove a key from given queue q
void deQueue(struct Queue *q) {
    // If queue is empty, return NULL.
    if (q->front == NULL)
        return;

    // Store previous front and move front one node ahead
    struct QNode *temp = q->front;

    q->front = q->front->next;
    sendReport--;
    // If front becomes NULL, then change rear also as NULL
    if (q->front == NULL)
        q->rear = NULL;

    free(temp);
}

void forceHardReset(void) {
    cli();                  // disable interrupts
    wdt_enable(WDTO_15MS);  // enable watchdog
    while (1) {
    }  // wait for watchdog to reset processor
}

void SetupHardware(void) {
    wdt_disable();
    init();

    Serial_Init(USART_BAUDRATE, USART_DOUBLE_SPEED);
    GlobalInterruptEnable();

    Serial_SendString("\r\nHELLO WORLD\r\n");
    USB_Init();
}

void EVENT_USB_Device_Connect(void) {}

void EVENT_USB_Device_Disconnect(void) {}

static int cfg_count = 0;
void       EVENT_USB_Device_ConfigurationChanged(void) {
    cfg_count++;
    Serial_SendString("USB CONFIG CHANGED\r\n");
    Endpoint_ConfigureEndpoint(ADAPTER_IN_NUM, EP_TYPE_INTERRUPT, ADAPTER_IN_SIZE, 1);
#ifdef ADAPTER_OUT_NUM
    Endpoint_ConfigureEndpoint(ADAPTER_OUT_NUM, EP_TYPE_INTERRUPT, ADAPTER_OUT_SIZE, 1);
#endif
}

void SendNextReport(void) {
    Endpoint_SelectEndpoint(ADAPTER_IN_NUM);

    if (!Endpoint_IsINReady()) {
        // Serial_SendString("Not ready\r\n");
        return;
    }

    if (!packet_queue || !packet_queue->front)
        return;

    Endpoint_Write_Stream_LE(packet_queue->front->packet, packet_queue->front->packet_len, NULL);
    Endpoint_ClearIN();
    Serial_SendString("Report Sent\r\n");
    deQueue(packet_queue);
}

enum FrameCommand {
    CMD_ACKNOWLEDGE   = 0x01,
    CMD_ANNOUNCE      = 0x02,
    CMD_STATUS        = 0x03,
    CMD_IDENTIFY      = 0x04,
    CMD_POWER_MODE    = 0x05,
    CMD_AUTHENTICATE  = 0x06,
    CMD_GUIDE_BTN     = 0x07,
    CMD_AUDIO_CONFIG  = 0x08,
    CMD_RUMBLE        = 0x09,
    CMD_LED_MODE      = 0x0a,
    CMD_SERIAL_NUM    = 0x1e,
    CMD_INPUT         = 0x20,
    CMD_AUDIO_SAMPLES = 0x60,
};

struct Frame {
    uint8_t command;
    uint8_t deviceId : 4;
    uint8_t type : 4;
    uint8_t sequence;
    uint8_t length;
} __attribute__((packed));

struct InputData {
    struct Buttons {
        uint32_t unknown : 2;
        uint32_t start : 1;
        uint32_t select : 1;
        uint32_t a : 1;
        uint32_t b : 1;
        uint32_t x : 1;
        uint32_t y : 1;
        uint32_t dpadUp : 1;
        uint32_t dpadDown : 1;
        uint32_t dpadLeft : 1;
        uint32_t dpadRight : 1;
        uint32_t bumperLeft : 1;
        uint32_t bumperRight : 1;
        uint32_t stickLeft : 1;
        uint32_t stickRight : 1;
    } __attribute__((packed)) buttons;

    uint16_t triggerLeft;
    uint16_t triggerRight;
    int16_t  stickLeftX;
    int16_t  stickLeftY;
    int16_t  stickRightX;
    int16_t  stickRightY;
} __attribute__((packed));

static char *getCommandName(int cmd) {
    switch (cmd) {
        case CMD_ACKNOWLEDGE:
            return "ACK";
        case CMD_ANNOUNCE:
            return "ANNOUNCE";
        case CMD_STATUS:
            return "CMD_STATUS";
        case CMD_IDENTIFY:
            return "CMD_IDENTIFY";
        case CMD_POWER_MODE:
            return "CMD_POWER_MODE";
        case CMD_AUTHENTICATE:
            return "CMD_AUTHENTICATE";
        case CMD_GUIDE_BTN:
            return "CMD_GUIDE_BTN";
        case CMD_AUDIO_CONFIG:
            return "CMD_AUDIO_CONFIG";
        case CMD_RUMBLE:
            return "CMD_RUMBLE";
        case CMD_LED_MODE:
            return "CMD_LED_MODE";
        case CMD_SERIAL_NUM:
            return "CMD_SERIAL_NUM";
        case CMD_INPUT:
            return "CMD_INPUT";
        case CMD_AUDIO_SAMPLES:
            return "CMD_AUDIO_SAMPLES";
        default:
            return "Unknown CMD";
    }
}

// announce packet
const uint8_t drumreport1[] = {
    0x02, 0x20, 0x06, 0x1c, 0x7e, 0xed, 0x82, 0x8b, 0xec, 0x97, 0x00, 0x00, 0x5e, 0x04, 0xd1, 0x02,
    0x02, 0x00, 0x03, 0x00, 0x4d, 0x09, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00,
};

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

static uint8_t identify_sequence = 0;

USB Usb;
XBOXONE Xbox(&Usb);

void ReceiveNextReport(void) {
    static struct {
        struct {
            uint8_t type;
            uint8_t length;
        } header;
        union buf {
            uint8_t          buffer[ADAPTER_OUT_SIZE];
            struct Frame     frame;
            struct InputData input_data;
        } buf;
    } packet;
    packet.header.type = BYTE_OUT_REPORT;  // = {.header.type = BYTE_OUT_REPORT};

    Endpoint_SelectEndpoint(ADAPTER_OUT_NUM);

    if (Endpoint_IsOUTReceived()) {
        uint16_t length = 0;

        packet.header.length = 0;

        if (Endpoint_IsReadWriteAllowed()) {
            uint8_t ErrorCode =
                Endpoint_Read_Stream_LE(packet.buf.buffer, sizeof(packet.buf.buffer), &length);

            packet.header.length =
                (ErrorCode == ENDPOINT_RWSTREAM_NoError) ? sizeof(packet.buf.buffer) : length;
        }

        Endpoint_ClearOUT();

        if (packet.header.length) {
            Serial_SendString("RECIEVED: ");
            Serial_SendString(getCommandName(packet.buf.frame.command));
            Serial_SendString("\r\n");
            switch (packet.buf.frame.command) {
                case CMD_IDENTIFY:
                    if (identify_sequence == 0) {
                        identify_sequence++;
                        // enQueue(packet_queue, drumreport2, sizeof(drumreport2));
                        // enQueue(packet_queue, drumreport3, sizeof(drumreport2));
                        // enQueue(packet_queue, drumreport4, sizeof(drumreport4));
                        // enQueue(packet_queue, drumreport5, sizeof(drumreport5));
                        // enQueue(packet_queue, drumreport6, sizeof(drumreport6));
                    }
                    break;
                case CMD_ACKNOWLEDGE:
                    break;
                case CMD_POWER_MODE:
                    break;
                case CMD_AUTHENTICATE:
                    break;
                default:
                    break;
            }
        }
    }
}

void HID_Task(void) {
    if (USB_DeviceState != DEVICE_STATE_Configured)
        return;

    SendNextReport();
    ReceiveNextReport();
}
void halt55() {
    E_Notify(PSTR("\r\nUnrecoverable error - test halted!!"), 0x80);
    E_Notify(PSTR("\r\n0x55 pattern is transmitted via SPI"), 0x80);
    E_Notify(PSTR("\r\nPress RESET to restart test"), 0x80);

    while (1) {
        Usb.regWr(0x55, 0x55);
#ifdef ESP8266
        yield();  // needed in order to reset the watchdog timer on the ESP8266
#endif
    }
}

int main(void) {
    SetupHardware();
    packet_queue = createQueue();
    enQueue(packet_queue, drumreport1, sizeof(drumreport1));
    enQueue(packet_queue, drumreport1, sizeof(drumreport1));

    if (Usb.Init() == -1) {
        Serial_SendString("\r\nOSC did not start");
        while (1)
            ;  // halt
    }
    Serial_SendString("\r\nXBOX ONE USB Library Started\r\n");
    USB_USBTask();
    for (;;) {
        HID_Task();
        USB_USBTask();
        Usb.Task();
        if (Xbox.XboxOneConnected) {
            if (Xbox.getAnalogHat(LeftHatX) > 7500 || Xbox.getAnalogHat(LeftHatX) < -7500 ||
                Xbox.getAnalogHat(LeftHatY) > 7500 || Xbox.getAnalogHat(LeftHatY) < -7500 ||
                Xbox.getAnalogHat(RightHatX) > 7500 || Xbox.getAnalogHat(RightHatX) < -7500 ||
                Xbox.getAnalogHat(RightHatY) > 7500 || Xbox.getAnalogHat(RightHatY) < -7500) {
                if (Xbox.getAnalogHat(LeftHatX) > 7500 || Xbox.getAnalogHat(LeftHatX) < -7500) {
                    Serial_SendString(("LeftHatX: "));
                    // Serial_SendString(std::string(Xbox.getAnalogHat(LeftHatX)));
                    // Serial_SendString("\t");
                }
                if (Xbox.getAnalogHat(LeftHatY) > 7500 || Xbox.getAnalogHat(LeftHatY) < -7500) {
                    Serial_SendString(("LeftHatY: "));
                    // Serial_SendString(Xbox.getAnalogHat(LeftHatY));
                    // Serial_SendString("\t");
                }
                if (Xbox.getAnalogHat(RightHatX) > 7500 || Xbox.getAnalogHat(RightHatX) < -7500) {
                    Serial_SendString(("RightHatX: "));
                    // Serial_SendString(Xbox.getAnalogHat(RightHatX));
                    // Serial_SendString("\t");
                }
                if (Xbox.getAnalogHat(RightHatY) > 7500 || Xbox.getAnalogHat(RightHatY) < -7500) {
                    Serial_SendString(("RightHatY: "));
                    // Serial_SendString(Xbox.getAnalogHat(RightHatY));
                }
                Serial_SendString("\r\n");
            }

            if (Xbox.getButtonPress(LT) > 0 || Xbox.getButtonPress(RT) > 0) {
                if (Xbox.getButtonPress(LT) > 0) {
                    Serial_SendString(("LT: "));
                    // Serial_SendString(Xbox.getButtonPress(LT));
                    // Serial_SendString("\t");
                }
                if (Xbox.getButtonPress(RT) > 0) {
                    Serial_SendString(("RT: "));
                    // Serial_SendString(Xbox.getButtonPress(RT));
                    // Serial_SendString("\t");
                }
                Serial_SendString("\r\n");
            }

            // Set rumble effect
            static uint16_t oldLTValue, oldRTValue;
            if (Xbox.getButtonPress(LT) != oldLTValue || Xbox.getButtonPress(RT) != oldRTValue) {
                oldLTValue = Xbox.getButtonPress(LT);
                oldRTValue = Xbox.getButtonPress(RT);
                uint8_t leftRumble =
                    map(oldLTValue, 0, 1023, 0, 255);  // Map the trigger values into a byte
                uint8_t rightRumble = map(oldRTValue, 0, 1023, 0, 255);
                if (leftRumble > 0 || rightRumble > 0)
                    Xbox.setRumbleOn(leftRumble, rightRumble, leftRumble, rightRumble);
                else
                    Xbox.setRumbleOff();
            }

            if (Xbox.getButtonClick(UP))
                Serial_SendString(("\r\nUp"));
            if (Xbox.getButtonClick(DOWN))
                Serial_SendString(("\r\nDown"));
            if (Xbox.getButtonClick(LEFT))
                Serial_SendString(("\r\nLeft"));
            if (Xbox.getButtonClick(RIGHT))
                Serial_SendString(("\r\nRight"));

            if (Xbox.getButtonClick(START))
                Serial_SendString(("\r\nStart"));
            if (Xbox.getButtonClick(BACK))
                Serial_SendString(("\r\nBack"));
            if (Xbox.getButtonClick(XBOX))
                Serial_SendString(("\r\nXbox"));
            if (Xbox.getButtonClick(SYNC))
                Serial_SendString(("\r\nSync"));
            if (Xbox.getButtonClick(SHARE))
                Serial_SendString(("\r\nShare"));

            if (Xbox.getButtonClick(LB))
                Serial_SendString(("\r\nLB"));
            if (Xbox.getButtonClick(RB))
                Serial_SendString(("\r\nRB"));
            if (Xbox.getButtonClick(LT))
                Serial_SendString(("\r\nLT"));
            if (Xbox.getButtonClick(RT))
                Serial_SendString(("\r\nRT"));
            if (Xbox.getButtonClick(L3))
                Serial_SendString(("\r\nL3"));
            if (Xbox.getButtonClick(R3))
                Serial_SendString(("\r\nR3"));

            if (Xbox.getButtonClick(A))
                Serial_SendString(("\r\nA"));
            if (Xbox.getButtonClick(B))
                Serial_SendString(("\r\nB"));
            if (Xbox.getButtonClick(X))
                Serial_SendString(("\r\nX"));
            if (Xbox.getButtonClick(Y))
                Serial_SendString(("\r\nY"));
        }
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
