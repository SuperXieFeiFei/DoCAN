#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define REVEIVER_STATUS_IDLE 0 
#define REVEIVER_STATUS_FC 1 //需要回复FC
#define REVEIVER_STATUS_FC_OVFLW 2 //FF中的长度溢出
#define REVEIVER_STATUS_CF 3 //等待CF

int receiverStatus;
int receiverLength; //需要接收的数据长度
unsigned char reveicerBuff[4095];
int receiverIndex; //已接收到的数据长度
int receiverExpectedSN; //期待接收的下一個CF的SN值
int receiverReqSendFC; //true:请求发送FC Frame

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
    for (int  i = 0; i < receiverIndex; i++)
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
        if (REVEIVER_STATUS_IDLE == receiverStatus) {
            int fflength = 0;
            printf("tempdata[0]:%d\n", tempdata[0]);
            fflength = (fflength | (tempdata[0] & 0x0F)) << 8;
            printf("fflength111:%d\n", fflength);
            fflength = fflength | tempdata[1];
            printf("fflength222:%d\n", fflength);
            printf("fflength333:%d\n", fflength);
            receiverLength = fflength;

            if (receiverLength <= 6) {
                // sendDataToUds();
                receiverStatus = REVEIVER_STATUS_IDLE; //空闲状态
            } else if (receiverLength > 4096) {
                receiverStatus = REVEIVER_STATUS_FC_OVFLW;
                receiverReqSendFC = 1; //true

            }else {
                memcpy(&reveicerBuff[receiverIndex], &tempdata[2], 6);
                receiverIndex += 6;
                receiverStatus = REVEIVER_STATUS_FC; 

                receiverExpectedSN = 1U;

                receiverReqSendFC = 1; //true

                showReceiverBuff();
            }
        }
    } else if (2 == frameType) {
        /* CF */
        if (REVEIVER_STATUS_CF == receiverStatus) {
            int SN = tempdata[0] & 0X0F;
            if (SN == receiverExpectedSN) {
                int receiverOtherLength = receiverLength - receiverIndex;
                if (receiverOtherLength > 7) {
                    memcpy(&reveicerBuff[receiverIndex], &tempdata[1], 7);
                    receiverIndex += 7;
                    receiverExpectedSN++;
                    showReceiverBuff();
                } else {
                    memcpy(&reveicerBuff[receiverIndex], &tempdata[1], receiverOtherLength);
                    receiverIndex += receiverOtherLength;
                    receiverExpectedSN = 0;
                    showReceiverBuff();
                    receiverStatus = REVEIVER_STATUS_IDLE;
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
    if (receiverReqSendFC) {
        switch (receiverStatus)
        {
        case REVEIVER_STATUS_FC:
            sendFCMessage();
            receiverStatus = REVEIVER_STATUS_CF;
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

    receiverStatus = 0;
    receiverLength = 0;
    memset(reveicerBuff, 0, 4095);
    receiverIndex = 0;
    receiverExpectedSN = 0;

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
