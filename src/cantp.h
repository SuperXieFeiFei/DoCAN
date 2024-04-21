#ifndef __CANTP_H__
#define __CANTP_H__


typedef struct
{
    int receiverStatus; //RX的当前状态
    int receiverLength; //需要接收的数据长度
    int receiverIndex; //已接收到的数据长度
    int receiverExpectedSN; //期待接收的下一個CF的SN值
    int receiverReqSendFC; //true:请求发送FC Frame
} CAN_TP_RX_Ctrl;



#endif