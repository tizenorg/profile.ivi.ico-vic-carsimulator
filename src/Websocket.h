/*
 * Copyright (c) 2013, TOYOTA MOTOR CORPORATION.
 *
 * This program is licensed under the terms and conditions of the 
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */
/**
 * @brief   websocket I/F
 */

#ifndef _ICO_VIC_WEBSOCKET_H_
#define _ICO_VIC_WEBSOCKET_H_

#include <sys/time.h>

#include <libwebsockets.h>

const int MsgQueueMaxMsgSize = 128;

struct KeyEventOptMsg_t
{
    char KeyEventType[64];
    struct timeval recordtime;
    struct KeyEventOpt
    {
        int common;
        int sense;
        int event_mask;
    } eventopt;
};

struct KeyDataMsg_t
{
    char KeyEventType[64];
    struct timeval recordtime;
    struct KeyData
    {
        int common_status;
        char status[];
    } data;
};

class WebsocketRecvQueue
{
  public:
        WebsocketRecvQueue();
        WebsocketRecvQueue(int queuesize);
        ~WebsocketRecvQueue();
    bool push(char *data, int datasize);
    bool front(char *buf) const;
    bool empty() const;
    void pop();
    int size() const;

    static const int maxdatasize = sizeof(KeyDataMsg_t) + MsgQueueMaxMsgSize;
  private:
    void init();

    int maxqueuesize;
    int msize;
    char (*mdata)[maxdatasize];
    int *mdatasize;
};

class WebsocketIF
{
  public:
    WebsocketIF();
    WebsocketIF(int port, char *interface, libwebsocket_protocols *protocol,
                pthread_mutex_t *mtx, pthread_cond_t *cnd,
                WebsocketRecvQueue *recvqueue);
        ~WebsocketIF();
    bool start(int port, char *interface, libwebsocket_protocols *protocol,
               pthread_mutex_t *mtx, pthread_cond_t *cnd,
               WebsocketRecvQueue *recvqueue);
    bool send(char *msg, int size);
    bool recv(char *msg, bool fbolcking);
    bool poll();
    static void *loop(void *arg);
  private:
    bool init(int port, char *interface,
              libwebsocket_protocols *protocol, pthread_mutex_t *mtx,
              pthread_cond_t *cnd, WebsocketRecvQueue *recvqueue);

    bool isready;
    libwebsocket_context *context;
    libwebsocket *websocket;
    pthread_t threadid;
    pthread_mutex_t *mutex;
    pthread_cond_t *cond;
    char sendbuf[LWS_SEND_BUFFER_PRE_PADDING +
                 WebsocketRecvQueue::maxdatasize +
                 LWS_SEND_BUFFER_POST_PADDING];
    WebsocketRecvQueue *queue;
};

#endif //#ifndef _ICO_VIC_WEBSOCKET_H_
