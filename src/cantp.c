#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "cantp.h"

#define REVEIVER_STATUS_IDLE 0 
#define REVEIVER_STATUS_FC 1 //需要回复FC
#define REVEIVER_STATUS_FC_OVFLW 2 //FF中的长度溢出
#define REVEIVER_STATUS_CF 3 //等待CF

CAN_TP_RX_Ctrl CanTP_RX_Ctrl;
unsigned char reveicerBuff[4095];
pthread_t myThread;

void sendDataToUds()
{

}

void sendFCMessage()
{
    printf("send FC Message\n");
}

void showReceiverBuff()
{
    for (int  i = 0; i < CanTP_RX_Ctrl.receiverIndex; i++)
    {
        printf("0x%X ", reveicerBuff[i]);
    }

    printf("\n");
    
}

void receiveMessage(char* data, int length)
{
    unsigned char tempdata[4095];
    int frameType;
    memset(tempdata, 0, 4095);

    printf("receiveMessage\n");

    memcpy(tempdata, data, length);
    frameType = tempdata[0] >> 4;


    if (0 == frameType) {
        /* Signle Frame */
        sendDataToUds();
    } else if (1 == frameType) {
        /* FF */
        if (REVEIVER_STATUS_IDLE == CanTP_RX_Ctrl.receiverStatus) {
            int fflength = 0;
            printf("tempdata[0]:%d\n", tempdata[0]);
            fflength = (fflength | (tempdata[0] & 0x0F)) << 8;
            printf("fflength111:%d\n", fflength);
            fflength = fflength | tempdata[1];
            printf("fflength222:%d\n", fflength);
            printf("fflength333:%d\n", fflength);
            CanTP_RX_Ctrl.receiverLength = fflength;

            if (CanTP_RX_Ctrl.receiverLength <= 6) {
                // sendDataToUds();
                CanTP_RX_Ctrl.receiverStatus = REVEIVER_STATUS_IDLE; //空闲状态
            } else if (CanTP_RX_Ctrl.receiverLength > 4096) {
                CanTP_RX_Ctrl.receiverStatus = REVEIVER_STATUS_FC_OVFLW;
                CanTP_RX_Ctrl.receiverReqSendFC = 1; //true
            }else {
                memcpy(&reveicerBuff[CanTP_RX_Ctrl.receiverIndex], &tempdata[2], 6);
                CanTP_RX_Ctrl.receiverIndex += 6;
                CanTP_RX_Ctrl.receiverStatus = REVEIVER_STATUS_FC; 

                CanTP_RX_Ctrl.receiverExpectedSN = 1U;

                CanTP_RX_Ctrl.receiverReqSendFC = 1; //true

                showReceiverBuff();
            }
        }
    } else if (2 == frameType) {
        /* CF */
        if (REVEIVER_STATUS_CF == CanTP_RX_Ctrl.receiverStatus) {
            int SN = tempdata[0] & 0X0F;
            if (SN == CanTP_RX_Ctrl.receiverExpectedSN) {
                int receiverOtherLength = CanTP_RX_Ctrl.receiverLength - CanTP_RX_Ctrl.receiverIndex;
                if (receiverOtherLength > 7) {
                    memcpy(&reveicerBuff[CanTP_RX_Ctrl.receiverIndex], &tempdata[1], 7);
                    CanTP_RX_Ctrl.receiverIndex += 7;
                    CanTP_RX_Ctrl.receiverExpectedSN++;
                    showReceiverBuff();
                } else {
                    memcpy(&reveicerBuff[CanTP_RX_Ctrl.receiverIndex], &tempdata[1], receiverOtherLength);
                    CanTP_RX_Ctrl.receiverIndex += receiverOtherLength;
                    CanTP_RX_Ctrl.receiverExpectedSN = 0;
                    showReceiverBuff();
                    CanTP_RX_Ctrl.receiverStatus = REVEIVER_STATUS_IDLE;
                }
            } else {
                printf("Error:expected SN Error!\n");
            }
        } else {
            printf("not wiat cf status\n");
        }
    } else if (3 == frameType) {
        /* FC */
    } else {

    }
}

void tp_receiver_process()
{
    if (CanTP_RX_Ctrl.receiverReqSendFC) {
        switch (CanTP_RX_Ctrl.receiverStatus)
        {
        case REVEIVER_STATUS_FC:
            sendFCMessage();
            CanTP_RX_Ctrl.receiverStatus = REVEIVER_STATUS_CF;
            break;
        case REVEIVER_STATUS_FC_OVFLW:
            sendFCMessage();
            break;
        default:
            break;
        }
    }
}

void* myFunc(void* arg)
{
    int param = *(int*)arg;
    printf("This is a thread! Parameter = %d\n", param);

    while (1)
    {
        Sleep(10);

        tp_receiver_process();

        /* code */
    }
    

    return NULL;
}

void initThread()
{
    printf("thread create ok\n");

    int param = 42;

    if (pthread_create(&myThread, NULL, myFunc, &param) != 0) {
        printf("thread create error\n");
    }

}

int main(int argc, char const *argv[])
{
    printf("aaa\n");

    initThread();

    CanTP_RX_Ctrl.receiverStatus = 0;
    CanTP_RX_Ctrl.receiverLength = 0;
    memset(reveicerBuff, 0, 4095);
    CanTP_RX_Ctrl.receiverIndex = 0;
    CanTP_RX_Ctrl.receiverExpectedSN = 0;

    unsigned char testarray[8] = {0x10, 0x14, 0x2E, 0xF1, 0x90, 0x01, 0x02, 0xF1};
    receiveMessage(testarray, 8);
    Sleep(20);

    unsigned char testarray_fc[8] = {0x21, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11};
    receiveMessage(testarray_fc, 8);
    Sleep(20);

    unsigned char testarray_f2[8] = {0x22, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11};
    receiveMessage(testarray_f2, 8);
    Sleep(20);

    unsigned char testarray_f3[8] = {0x23, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11};
    receiveMessage(testarray_f3, 8);


    pthread_join(myThread, NULL);

    return 0;
}
