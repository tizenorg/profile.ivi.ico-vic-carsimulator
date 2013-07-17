/*
 * Copyright (c) 2013, TOYOTA MOTOR CORPORATION.
 *
 * This program is licensed under the terms and conditions of the 
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */
/**
 * @brief   AMB plug-inn Communication I/F
 *
 */

#include <string.h>
#include <iostream>
#include <vector>
#include <unistd.h>
#include "AmbpiComm.h"
using namespace std;

static vector<AmbpiCommIF*> _commList;

bool _addAmbCommIFList(AmbpiCommIF* comm);
void _eraseAmbCommIFList(const AmbpiCommIF* comm);
static void _icoUwsCallback(const struct ico_uws_context *context,
                           const ico_uws_evt_e event, const void *id,
                           const ico_uws_detail *detail, void *user_data);


AmbpiCommRecvQueue::AmbpiCommRecvQueue()
: msize(0), maxqueuesize(10)
{
    init();
}

AmbpiCommRecvQueue::AmbpiCommRecvQueue(int queuesize)
: msize(0)
{
    maxqueuesize = queuesize;
    init();
}

AmbpiCommRecvQueue::~AmbpiCommRecvQueue()
{
    delete[] mdata;
    delete[] mdatasize;
}

bool AmbpiCommRecvQueue::push(char *data, int datasize)
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

bool AmbpiCommRecvQueue::front(char *buf) const
{
    if (msize == 0) {
        return false;
    }
    memcpy(buf, mdata[0], mdatasize[0]);
    return true;
}

bool AmbpiCommRecvQueue::empty() const
{
    return (msize == 0);
}

void AmbpiCommRecvQueue::pop()
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

int AmbpiCommRecvQueue::size() const
{
    return msize;
}

void AmbpiCommRecvQueue::init()
{
    mdata = new char[maxqueuesize][maxdatasize];
    memset((void *) mdata, 0, maxqueuesize * maxdatasize);

    mdatasize = new int[maxqueuesize];

    memset(mdatasize, 0, maxqueuesize);
}

AmbpiCommIF::AmbpiCommIF()
{
    isready = false;
    m_threadid = 0;
    m_mutex = PTHREAD_MUTEX_INITIALIZER;
    m_cond = PTHREAD_COND_INITIALIZER;
    m_id = NULL;
    m_context = NULL;
    reset_ercode();
}

AmbpiCommIF::AmbpiCommIF(const char* uri, const char* protocolName)
{
    isready = false;
    m_threadid = 0;
    m_mutex = PTHREAD_MUTEX_INITIALIZER;
    m_cond = PTHREAD_COND_INITIALIZER;
    m_id = NULL;
    m_context = NULL;
    reset_ercode();
    start(uri, protocolName);
}

AmbpiCommIF::~AmbpiCommIF()
{
    if (isready) {
        int ret;
        if (m_threadid != 0) {
            ret = pthread_cancel(m_threadid);
            if (ret != 0) {
                cerr << m_pNm << ":Failed to pthread_cancel" << endl;
            }
        }
        ret = pthread_join(m_threadid, NULL);
        if (ret != 0) {
            cerr << m_pNm << ":Failed to pthread_join" << endl;
        }
        if (m_context != NULL) {
            ico_uws_close(m_context);
            m_context = NULL;
        }
        _eraseAmbCommIFList(this);
    }
}

bool AmbpiCommIF::start(const char* uri, const char* protocolName)
{
    if (isready) {
        return isready;
    }
    isready = init(uri, protocolName);
    return isready;
}

bool AmbpiCommIF::send(const char *msg, const int size)
{
    if (!isready) {
        return isready;
    }
    reset_ercode();
    ico_uws_send(m_context, m_id, (unsigned char*)msg, (size_t)size);
    if ((ICO_UWS_ERR_UNKNOWN != m_ercode) && (ICO_UWS_ERR_NONE != m_ercode)) {
        return false;
    }
    return true;
}

bool AmbpiCommIF::recv(char *msg, bool fblocking)
{
    if (!isready) {
        return isready;
    }

    if (fblocking) {
        if (m_queue.empty()) {
            if (pthread_cond_wait(&m_cond, &m_mutex) != 0) {
                cerr << m_pNm << ":Failed to wait signal" << endl;
            }
        }
        m_queue.front(msg);
        m_queue.pop();
    }
    else {
        if (m_queue.empty()) {
            return false;
        }
        else {
            m_queue.front(msg);
            m_queue.pop();
        }
    }
    return true;
}

bool AmbpiCommIF::poll()
{
    if (!isready) {
        return isready;
    }

    while (true) {
        ico_uws_service(m_context);
        usleep(100000);
    }
}

void *AmbpiCommIF::loop(void *arg)
{
    AmbpiCommIF *src = reinterpret_cast < AmbpiCommIF * >(arg);
    return (void *) src->poll();
}

bool AmbpiCommIF::init(const char* uri, const char* protocolName)
{
    if (NULL == uri) {
        cerr << "Failed create context param" << endl;
        return false;
    }
    if (NULL == protocolName) {
        cerr << "Failed create context param 2" << endl;
        return false;
    }
    m_uri = uri;
    m_pNm = protocolName;
    m_context = ico_uws_create_context(uri, protocolName);
    if (NULL == m_context) {
        cerr << m_pNm << ":Failed to create context." << endl;
        return false;
    }
    int r = ico_uws_set_event_cb(m_context, _icoUwsCallback, (void*)this);
    if (ICO_UWS_ERR_NONE != r) {
        cerr << m_pNm << ":Failed to callback entry(" << r << ")." << endl;
        return false;
    }
    _addAmbCommIFList(this);
    if (pthread_create(&m_threadid, NULL, AmbpiCommIF::loop,
                       (void *) this) != 0) {
        cerr << m_pNm << ":Failed to create thread." << endl;
        return false;
    }
    return true;
}

/**
 * @brief event
 * @param event ico_uws_evt_e event code
 * @param id unique id
 * @param detail infomation or data detail
 * @param user_data added user data
 */
void AmbpiCommIF::event_cb(const ico_uws_evt_e event, const void *id,
                           const ico_uws_detail *d, void *user_data)
{
    switch (event) {
    case ICO_UWS_EVT_RECEIVE:
    {
        if (NULL == d) {
            cerr << m_pNm << ":Failed Receive event" << endl;
            break;
        }
        if (0 != pthread_mutex_lock(&m_mutex)) {
            cerr << m_pNm << ":Failed to lock mutex" << endl;
        }
        char buf[sizeof(KeyDataMsg_t) + MsgQueueMaxMsgSize];
        memset(buf, 0, sizeof(buf));
        memcpy(buf, reinterpret_cast<char*>(d->_ico_uws_message.recv_data),
               d->_ico_uws_message.recv_len);
        m_queue.push((&buf[0]), sizeof(buf));
        pthread_cond_signal(&m_cond);
        if (0 != pthread_mutex_unlock(&m_mutex)) {
            cerr << m_pNm << ":Failed to unlock mutex" << endl;
        }
        break;
    }
    case ICO_UWS_EVT_OPEN:
    {
        m_id = (void*)id;
        if (0 != pthread_mutex_lock(&m_mutex)) {
            cerr << m_pNm << ":Failed to lock mutex" << endl;
        }
        if (0 != pthread_cond_signal(&m_cond)) {
            cerr << m_pNm << ":Failed to issue cond_signal" << endl;
        }
        if (0 != pthread_mutex_unlock(&m_mutex)) {
            cerr << m_pNm << ":Failed to unlock mutex" << endl;
        }
        break;
    }
    case ICO_UWS_EVT_ERROR:
    {
        if (NULL == d) {
            cerr << m_pNm << ":Failed ERROR event" << endl;
            break;
        }
        m_ercode = d->_ico_uws_error.code;
        break;
    }
    default:
        break;
    }
    return;
}

/**
 * @brief _addAmbCommIFList
 * @param comm entry item address
 * @return true:succes false:fail
 */
bool _addAmbCommIFList(AmbpiCommIF* comm)
{
    if (NULL == comm) {
        return false;
    }
    _commList.push_back(comm);
    return true;
}

/**
 * @brief _eraseAmbCommIFList
 * @param comm erase target
 */
void _eraseAmbCommIFList(const AmbpiCommIF* comm)
{
    bool bMatch = false;
    for (int i = 0; i < _commList.size(); i++) {
        if (_commList[i] == comm) {
            bMatch = true;
            break; // break of for i
        }
    }
    if (false == bMatch) {
        return;
    }
    vector<AmbpiCommIF*> v_tmp(_commList);
    _commList.clear();
    for (int i = 0; i < v_tmp.size(); i++) {
        if (v_tmp[i] != comm) {
            _commList.push_back(v_tmp[i]);
        }
    }
    return;
}

/**
 * @brief _icoUwsCallback
 * @param context identify connect
 * @param event ico_uws_evt_e event code
 * @param id unique id
 * @param detail infomation or data detail
 * @param user_data added user data
 */
static void _icoUwsCallback(const struct ico_uws_context *context,
                           const ico_uws_evt_e event, const void *id,
                           const ico_uws_detail *detail, void *user_data)
{
    for (int i = 0; i < _commList.size(); i++) {
        AmbpiCommIF* comm = _commList[i];
        if (NULL == comm) {
            continue;
        }
        if (false == comm->isContextMatch(context)) {
            continue;
        }
        comm->event_cb(event, id, detail, user_data);
    }
}
