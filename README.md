# libxmodem
XModem C Library

XMODEM communication protocol. learn more [here](https://en.wikipedia.org/wiki/XMODEM) (wiki page)

supported protocol varients:
 - XMODEM (original)
 - ~~XMODEM-CRC~~
 - ~~XMODEM-1K~~

protocol customizations/overrides:
 - timeouts
 - attempts
 - MTU
 - internal functions

## Requirements

#include <stdint.h>

## API

Setup Example

```code
uint32_t timeout = 1000;
uint8_t attempts = 10;

xmodemInterface_t interface = {
    .timerStart = extTimerStartFunc;
    .timerStop = extTimerStopFunc;
    .timerIsElapsed = extTimerIsTimerElapsedFunc;
    .read = extGetCharFunc;
    .write = extPutCharFunc;
    .flush = extFlushFunc;
};

xmodemContext_t xmodem;
```

Transmit Initization Example
```code
xmodemTransmitInit(&xmodem, interface, timeout, attempts);
```

Receive Initization Example
```code
xmodemReceiveInit(&xmodem, interface, timeout, attempts);
```

Transmit Usage Example
```code
int32_t retCode = 0;

uint8_t* buffer = malloc(xmodemGetMtuSize(&modem));

if (modem.begin(&modem) == xmodemErrorNone) {
    uint8_t messageNumber = 1;
    uint32_t bytesSent = 0;
    uint32_t timeout = 1000;

    while (bytesSent < fileSize && retCode != xmodemErrorCancel) {
        uint32_t bytesToSend = 0;
        uint32_t bytesCopied = 0;

        memset(buffer, 0, xmodemGetMtuSize(&modem));

        bytesToSend = extGetDataFunc(bytesSent, buffer, xmodemGetMtuSize(&modem));

        if (bytesToSend > 0) {

            retCode = modem.transceive(&modem, buffer, bytesToSend, messageNumber, &bytesCopied);
            if (retCode == xmodemErrorNone) {
                messageNumber++;
                bytesSent += bytesCopied;
            } else {
                extDelayFunc(timeout);
            }

        } else {
            retCode = xmodemErrorCancel;
        }

    }

    if (retCode == xmodemErrorNone) {
        retCode = modem.end(&modem);
    } else {
        modem.cancel(&modem);
    }

}
```

Receive Usage Example
```code
int32_t retCode = 0;

uint8_t* buffer = malloc(xmodemGetMtuSize(&modem));
uint8_t firstTime = 1;
uint8_t messageNumber = 1;
uint32_t bytesReceived = 0;
uint32_t timeout = 1000;

while (retCode != xmodemErrorCancel && retCode != xmodemErrorTimeout && retCode != xmodemErrorFinished){
    uint32_t bytesCopied = 0;

    if (firstTime == 1) {
        modem.begin(&modem);
        extDelayFunc(timeout);
    }

    retCode = modem.transceive(&modem, buffer, xmodemGetMtuSize(&modem), messageNumber, &bytesCopied);
    if (retCode == xmodemErrorNone) {
        firstTime = 0;

        int32_t bytesWrote = extSetDataFunc(bytesReceived, pMessage, xmodemGetMtuSize(&modem));
        if (bytesWrote > 0) {
            bytesReceived += bytesWrote;
            messageNumber++;
        } else {
            retCode = xmodemErrorCancel;
        }

    }

    memset(buffer, 0, xmodemGetMtuSize(&modem));

}

if (retCode == xmodemErrorFinished) {
    retCode = modem.end(&modem);
} else {
    modem.cancel(&modem);
}
```