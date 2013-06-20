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
 *
 */

#include <string.h>

#include <iostream>

#include "Websocket.h"

WebsocketRecvQueue::WebsocketRecvQueue()
: msize(0), maxqueuesize(10)
{
    init();
}

WebsocketRecvQueue::WebsocketRecvQueue(int queuesize)
: msize(0)
{
    maxqueuesize = queuesize;
    init();
}

WebsocketRecvQueue::~WebsocketRecvQueue()
{
    delete[] mdata;
    delete[] mdatasize;
}

bool WebsocketRecvQueue::push(char *data, int datasize)
{
    if (datasize > maxdatasize || (msize + 1) > maxqueuesize) {
        return false;
    }
    memset(mdata[msize], 0, maxdatasize);
    memcpy(mdata[msize], data, datasize);
    mdatasize[msize] = datasize;
    msize++;
    return true;
}

bool WebsocketRecvQueue::front(char *buf) const
{
    if (msize == 0) {
        return false;
    }
    memcpy(buf, mdata[0], mdatasize[0]);
    return true;
}

bool WebsocketRecvQueue::empty() const
{
    return (msize == 0);
}

void WebsocketRecvQueue::pop()
{
    if (msize > 0) {
        msize--;
        for (int i = 0; i < msize; i++) {
            memset(mdata[i], 0, maxdatasize);
            memcpy(mdata[i], mdata[i + 1], mdatasize[i + 1]);
            mdatasize[i] = mdatasize[i + 1];
        }
        memset(mdata[msize], 0, maxdatasize);
        mdatasize[msize] = 0;
    }
}

int WebsocketRecvQueue::size() const
{
    return msize;
}

void WebsocketRecvQueue::init()
{
    mdata = new char[maxqueuesize][maxdatasize];
    memset((void *) mdata, 0, maxqueuesize * maxdatasize);

    mdatasize = new int[maxqueuesize];

    memset(mdatasize, 0, maxqueuesize);
}

WebsocketIF::WebsocketIF()
:  isready(false), threadid(0)
{
}

WebsocketIF::WebsocketIF(int port, char *interface,
                         libwebsocket_protocols * protocols,
                         pthread_mutex_t * mtx,
                         pthread_cond_t * cnd, WebsocketRecvQueue * recvqueue)
:threadid(0), mutex(mtx), cond(cnd), queue(recvqueue)
{
    isready = init(port, interface, protocols, mtx, cnd, recvqueue);
}

WebsocketIF::~WebsocketIF()
{
    if (isready) {
        int ret;
        if (threadid != 0) {
            ret = pthread_cancel(threadid);
            if (ret != 0) {
                std::cerr << "Failed to pthread_cancel" << std::endl;
            }
        }
        ret = pthread_join(threadid, NULL);
        if (ret != 0) {
            std::cerr << "Failed to pthread_join" << std::endl;
        }
        if (context != NULL) {
            libwebsocket_context_destroy(context);
            context = NULL;
        }
    }
}

bool WebsocketIF::start(int port, char *interface,
                        libwebsocket_protocols * protocols,
                        pthread_mutex_t * mtx,
                        pthread_cond_t * cnd, WebsocketRecvQueue * recvqueue)
{
    if (isready) {
        return isready;
    }
    mutex = mtx;
    cond = cnd;
    queue = recvqueue;
    isready = init(port, interface, protocols, mtx, cnd, recvqueue);
    return isready;
}

bool WebsocketIF::send(char *msg, int size)
{
    if (!isready) {
        return isready;
    }

    char *buf;
    memset(sendbuf, 0, sizeof(sendbuf));
    buf = &sendbuf[LWS_SEND_BUFFER_PRE_PADDING];
    memcpy(buf, msg, size);
    int ret = libwebsocket_write(websocket,
                                 reinterpret_cast < unsigned char *>(buf),
                                 size, LWS_WRITE_BINARY);
    return (ret == 0);
}

bool WebsocketIF::recv(char *msg, bool fblocking)
{
    if (!isready) {
        return isready;
    }

    if (fblocking) {
        if (queue->empty()) {
            if (pthread_cond_wait(cond, mutex) != 0) {
                std::cerr << "Failed to wait signal" << std::endl;
            }
        }
        queue->front(msg);
        queue->pop();
    }
    else {
        if (queue->empty()) {
            return false;
        }
        else {
            queue->front(msg);
            queue->pop();
        }
    }
    return true;
}

bool WebsocketIF::poll()
{
    if (!isready) {
        return isready;
    }

    int ret = 0;
    while (true) {
        ret = libwebsocket_service(context, 100);
    }
}

void *WebsocketIF::loop(void *arg)
{
    WebsocketIF *src = reinterpret_cast < WebsocketIF * >(arg);
    return (void *) src->poll();
}

bool WebsocketIF::init(int port, char *interface,
                       libwebsocket_protocols * protocols,
                       pthread_mutex_t * mtx,
                       pthread_cond_t * cnd, WebsocketRecvQueue * recvqueue)
{
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof info);

    info.iface = interface;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;
    info.extensions = libwebsocket_get_internal_extensions();
    info.options = 0;


    context = libwebsocket_create_context(&info);

    if (context == NULL) {
        std::cerr << "Failed to create context." << std::endl;
        return false;
    }
    websocket = libwebsocket_client_connect(context, "127.0.0.1", port,
                                            0, "/", "localhost", "websocket",
                                            protocols[0].name, -1);
    if (websocket == NULL) {
        std::cerr << "Failed to connect server." << std::endl;
        return false;
    }
    if (pthread_create(&threadid, NULL, WebsocketIF::loop,
                       (void *) this) != 0) {
        std::cerr << "Failed to create thread." << std::endl;
        return false;
    }
    return true;
}
