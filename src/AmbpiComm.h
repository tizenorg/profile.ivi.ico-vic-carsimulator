/*
 * Copyright (c) 2013, TOYOTA MOTOR CORPORATION.
 *
 * This program is licensed under the terms and conditions of the 
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */
/**
 * @brief   AMB plug-in communication I/F
 */

#ifndef _ICO_VIC_AMBPICOMM_H_
#define _ICO_VIC_AMBPICOMM_H_

#include <sys/time.h>
#include <string>
#include "ico-util/ico_uws.h"

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

class AmbpiCommRecvQueue
{
  public:
        AmbpiCommRecvQueue();
        AmbpiCommRecvQueue(int queuesize);
        ~AmbpiCommRecvQueue();
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

class AmbpiCommIF
{
  public:
    AmbpiCommIF();
    AmbpiCommIF(const char* uri, const char* protocolName);
    ~AmbpiCommIF();
    bool start(const char* uri, const char* protocolName);
    bool send(const char *msg, const int size);
    bool recv(char *msg, bool fbolcking);
    bool poll();
    static void *loop(void *arg);
    bool isContextMatch(const ico_uws_context* context) const;
//    bool threadCondWait();
    void event_cb(const ico_uws_evt_e event, const void *id,
                  const ico_uws_detail *detail, void *user_data);
    void reset_ercode();
    char *getM_id()
    {
        return (char *)m_id;
    };
  private:
    bool init(const char* uri, const char* protocolName);

    bool isready;
    ico_uws_context* m_context;
    pthread_t  m_threadid;
    pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;
    char m_sendbuf[AmbpiCommRecvQueue::maxdatasize];
    AmbpiCommRecvQueue m_queue;
    std::string m_uri; // URI
    std::string m_pNm; // protocol name
    ico_uws_error_e m_ercode;
    void* m_id;
};

/**
 * @brief context compare
 * @param context 
 * @return true:agree / false:not agree
 */
inline bool AmbpiCommIF::isContextMatch(const ico_uws_context* context) const
{
    if (context == m_context) {
        return true;
    }
    return false;
}

/**
 * @brief thread wait
 * @return true:success / false:fail
 *//*
inline bool AmbpiCommIF::threadCondWait()
{
    if (0 != pthread_cond_wait(&m_cond, &m_mutex)) {
        return false;
    }
    return true;
}
*/
/**
 * @brief reset error code
 */
inline void AmbpiCommIF::reset_ercode()
{
    m_ercode = ICO_UWS_ERR_UNKNOWN;
}

#endif //#ifndef _ICO_VIC_AMBPICOMM_H_
