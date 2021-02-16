#ifndef _XMODEM_H_
#define _XMODEM_H_

#include <stdint.h>

#define xmodemErrorFinished 1
#define xmodemErrorNone 0
#define xmodemErrorNak -1
#define xmodemErrorCancel -2
#define xmodemErrorPacket -3
#define xmodemErrorTimeout -4
#define xmodemErrorContext -5
#define xmodemErrorUnknown -6

#define EOT 0x04
#define ACK 0x06
#define NAK 0x15
#define CAN 0x18
#define SUB 0x1A

typedef struct _xmodemInterfaceStruct {
    uint32_t (*available)(void);
    uint8_t (*read)(void);
    void (*write)(uint8_t);
    int32_t (*flush)(void);
    void (*timerStart)(uint32_t timeout);
    void (*timerStop)(void);
    uint8_t (*timerIsElapsed)(void);
    uint8_t (*timerIsEnabled)(void);
} xmodemInterface_t;

typedef struct _xmodemContextStruct {
    uint8_t attempts;
    uint32_t timeout;
    uint32_t mtu;
    xmodemInterface_t iface;
    int32_t (*begin)(struct _xmodemContextStruct* pContext);
    int32_t (*transceive)(struct _xmodemContextStruct* pContext, uint8_t* pMessage, uint32_t messageLength, uint8_t messageNumber, uint32_t* pBytesCopied);
    int32_t (*end)(struct _xmodemContextStruct* pContext);
    int32_t (*cancel)(struct _xmodemContextStruct* pContext);
} xmodemContext_t;

uint32_t xmodemGetMtuSize(xmodemContext_t* pContext);
xmodemContext_t* xmodemTransmitInit(xmodemContext_t* pContext, xmodemInterface_t iface, uint32_t timeout, uint8_t attempts);
xmodemContext_t* xmodemReceiveInit(xmodemContext_t* pContext, xmodemInterface_t iface, uint32_t timeout, uint8_t attempts);

#endif /*_XMODEM_H_*/