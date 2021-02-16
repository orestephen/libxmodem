#include "xmodem.h"

#include <stdio.h>

#define SOH 0x01

#define MTU128 128

static uint8_t xmodemChecksum(const uint8_t* pBuffer, uint32_t length, uint8_t seed) {
    uint8_t checksum = seed;

    for (uint8_t i = 0; i < length; i++) {
        checksum += pBuffer[i];
    }

    return checksum;
}

static int32_t xmodemWait(xmodemContext_t* pContext, uint32_t size) {
    int32_t retCode = xmodemErrorNone;

    if (pContext != NULL) {

        pContext->iface.timerStart(pContext->timeout);

        do {
            if (pContext->iface.available() >= size) {
                pContext->iface.timerStop();
                retCode = (int32_t)pContext->iface.read();
            }
        } while (pContext->iface.timerIsElapsed() == 0);

        if (pContext->iface.timerIsEnabled() == 0) {
            retCode = xmodemErrorTimeout;
        }

    } else {
        retCode = xmodemErrorContext;
    }

    return retCode;
}

uint32_t xmodemGetMtuSize(xmodemContext_t* pContext) {
    uint32_t mtuSize = 0;

    if (pContext != NULL) {
        mtuSize = pContext->mtu;
    }

    return mtuSize;
}

static int32_t transmitBegin(xmodemContext_t* pContext) {
    int32_t retCode = xmodemErrorNone;

    if (pContext != NULL) {

        for (uint32_t i = 0; i < pContext->attempts; i++) {

            switch (xmodemWait(pContext, 1)) {
                case NAK:
                    retCode = xmodemErrorNone;
                    i = pContext->attempts;
                    break;
                case CAN:
                    pContext->iface.write(ACK);
                    pContext->iface.flush();
                    retCode = xmodemErrorCancel;
                    i = pContext->attempts;
                    break;
                default:
                    retCode = xmodemErrorUnknown;
                    break;
            }
        }

    } else {
        retCode = xmodemErrorContext;
    }

    return retCode;
}

static int32_t transmitData(xmodemContext_t* pContext, uint8_t* pMessage, uint32_t messageLength, uint8_t messageNumber, uint32_t* pBytesCopied) {
    int32_t retCode = xmodemErrorNone;

    if (pContext != NULL && pBytesCopied != NULL) {
        *pBytesCopied = 0;

        for (uint32_t i = 0; i < pContext->attempts; i++) {

            uint32_t bytesRemaining;
            uint32_t bytesToSend;
            uint8_t checksum;

            pContext->iface.write(SOH);
            
            pContext->iface.write(messageNumber);
            pContext->iface.write(~messageNumber);

            if (messageLength < pContext->mtu) {
                bytesRemaining = pContext->mtu - messageLength;
                bytesToSend = messageLength;
            } else {
                bytesRemaining = 0;
                bytesToSend = pContext->mtu;
            }

            for (uint32_t i = 0; i < bytesToSend; i++) {
                pContext->iface.write(pMessage[i]);
                (*pBytesCopied)++;
            }

            checksum = xmodemChecksum(pMessage, bytesToSend, 0);

            for (uint32_t i = 0; i < bytesRemaining; i++) {
                uint8_t temp = SUB;
                checksum = xmodemChecksum(&temp, 1, checksum);
                pContext->iface.write(SUB);
            }

            pContext->iface.write(checksum);

            pContext->iface.flush();

            switch (xmodemWait(pContext, 1)) {
                case ACK:
                    retCode = xmodemErrorNone;
                    i = pContext->attempts;
                    break;
                case NAK:
                    retCode = xmodemErrorNak;
                    break;
                case CAN:
                    i = pContext->attempts;
                    retCode = xmodemErrorCancel;
                    break;
                default:
                    retCode = xmodemErrorTimeout;
                    break;
            }
        }

    } else {
        retCode = xmodemErrorContext;
    }

    return retCode;
}

static int32_t transmitEnd(xmodemContext_t* pContext) {
    int32_t retCode = xmodemErrorNone;

    if (pContext != NULL) {

        for (uint32_t i = 0; i < pContext->attempts; i++) {

            pContext->iface.write(EOT);
            pContext->iface.flush();

            switch (xmodemWait(pContext, 1)) {
                case ACK:
                    retCode = xmodemErrorNone;
                    i = pContext->attempts;
                    break;
                case NAK:
                    retCode = xmodemErrorNak;
                    break;
                case CAN:
                    i = pContext->attempts;
                    retCode = xmodemErrorCancel;
                    break;
                default:
                    retCode = xmodemErrorTimeout;
                    break;
            }
        }
    
    } else {
        retCode = xmodemErrorContext;
    }

    return retCode;
}

static int32_t receiveBegin(xmodemContext_t* pContext) {
    int32_t retCode = xmodemErrorNone;

    if (pContext != NULL) {

        pContext->iface.write(NAK);
        pContext->iface.flush();

    } else {
        retCode = xmodemErrorContext;
    }

    return retCode;
}

static int32_t receiveData(xmodemContext_t* pContext, uint8_t* pMessage, uint32_t messageLength, uint8_t messageNumber, uint32_t* pBytesCopied) {
    int32_t retCode = xmodemErrorNone;
    

    if (pContext != NULL && pBytesCopied != NULL) {
        *pBytesCopied = 0;

        if (messageLength >= pContext->mtu) {

            for (uint32_t i = 0; i < pContext->attempts; i++) {
                uint8_t packetNum;
                uint8_t checksum;
                uint8_t invertedPacketNum;

                switch (xmodemWait(pContext, 1)) {
                    case SOH:
                        packetNum = (uint8_t)xmodemWait(pContext, 1);
                        invertedPacketNum = (uint8_t)~xmodemWait(pContext, 1);

                        if (messageNumber == packetNum && messageNumber == invertedPacketNum) {
                            
                            for (uint32_t i = 0; i < pContext->mtu; i++) {
                                pMessage[i] = (uint8_t)xmodemWait(pContext, pContext->mtu - i);
                                (*pBytesCopied)++;
                            }

                            checksum = (uint8_t)xmodemWait(pContext, 1);
                            
                            if (checksum == xmodemChecksum(pMessage, pContext->mtu, 0)) {
                                retCode = xmodemErrorNone;
                            } else {
                                retCode = xmodemErrorNak;
                            }

                        } else {
                            retCode = xmodemErrorNak;
                        }

                        i = pContext->attempts;
                        break;
                    case CAN:
                        retCode = xmodemErrorCancel;
                        i = pContext->attempts;
                        break;
                    case EOT:
                        retCode = xmodemErrorFinished;
                        i = pContext->attempts;
                        break;
                    default:
                        retCode = xmodemErrorTimeout;
                        break;
                }

                if (retCode >= xmodemErrorNone) {
                    if (retCode != 1) {
                        pContext->iface.write(ACK);
                    }
                } else {
                    pContext->iface.write(NAK);
                }

                pContext->iface.flush();
            }

        } else {
            retCode = xmodemErrorContext;
        }

    } else {
        retCode = xmodemErrorContext;
    }    

    return retCode;
}

static int32_t receiveEnd(xmodemContext_t* pContext) {
    int32_t retCode = xmodemErrorNone;

    if (pContext != NULL) {

        pContext->iface.write(ACK);
        pContext->iface.flush();

    } else {
        retCode = xmodemErrorContext;
    }

    return retCode;
}

static int32_t transceiveCansel(xmodemContext_t* pContext) {
    int32_t retCode = xmodemErrorNone;

    if (pContext != NULL) {
        pContext->iface.write(CAN);
        pContext->iface.write(CAN);
        pContext->iface.write(CAN);
        pContext->iface.flush();
    } else {
        retCode = xmodemErrorContext;
    }

    return retCode;
}

xmodemContext_t* xmodemTransmitInit(xmodemContext_t* pContext, xmodemInterface_t iface, uint32_t timeout, uint8_t attempts) {
    xmodemContext_t* context = NULL;

    if (pContext != NULL) {

        pContext->iface = iface;

        pContext->mtu = MTU128;
        pContext->attempts = attempts;
        pContext->timeout = timeout;

        pContext->begin = transmitBegin;
        pContext->transceive = transmitData;
        pContext->end = transmitEnd;
        pContext->cancel = transceiveCansel;

        context = pContext;
    }

    return context;
}

xmodemContext_t* xmodemReceiveInit(xmodemContext_t* pContext, xmodemInterface_t iface, uint32_t timeout, uint8_t attempts) {
    xmodemContext_t* context = NULL;

    if (pContext != NULL) {

        pContext->iface = iface;

        pContext->mtu = MTU128;
        pContext->attempts = attempts;
        pContext->timeout = timeout;

        pContext->begin = receiveBegin;
        pContext->transceive = receiveData;
        pContext->end = receiveEnd;
        pContext->cancel = transceiveCansel;

        context = pContext;
    }

    return context;
}