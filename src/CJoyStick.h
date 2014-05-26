/*
 * Copyright (c) 2013, TOYOTA MOTOR CORPORATION.
 *
 * This program is licensed under the terms and conditions of the 
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */

/**
 * @brief   Gets the value of the joystick operation
 * @file    CJoyStick.h
 */

#ifndef CJOYSTICK_H_
#define CJOYSTICK_H_

#include <fcntl.h>
#include <linux/joystick.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>
#include <string>
#include <queue>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include "ico-util/ico_log.h"

#define D_DEV_DIR_PATH      "/dev/input/"
#define D_DEV_NAME_PARTS_JS "js"
#define D_DEV_NAME_G25      "Driving Force GT"
#define D_DEV_NAME_G27      "G27 Racing Wheel"

template <typename T>
class CJoyStickQueue
{
public:
    CJoyStickQueue()
    {
/*
        pthread_mutex_init(&mutex, NULL);
        pthread_cond_init(&cond, NULL);
*/
        mutex = PTHREAD_MUTEX_INITIALIZER;
        cond = PTHREAD_COND_INITIALIZER;
    }

    ~CJoyStickQueue()
    {
        if (pthread_mutex_destroy(&mutex) != 0) {
            ICO_ERR("pthread_mutex_destroy error[errno=%d]", errno);
        }
        if (pthread_cond_destroy(&cond) != 0) {
            ICO_ERR("pthread_cond_destroy error[errno=%d]", errno);
        }
    }

    int size()
    {
        if (pthread_mutex_lock(&mutex) != 0) {
            ICO_ERR("pthread_mutex_lock error[errno=%d]", errno);
        }
        int ret = mQueue.size();
        if (pthread_mutex_unlock(&mutex) != 0) {
            ICO_ERR("pthread_mutex_unlock error[errno=%d]", errno);
        }
        return ret;
    }

    int pop(T& item, double msec)
    {
        struct timespec now;
        struct timespec timeout;
        double sec;

        if (pthread_mutex_lock(&mutex) != 0) {
            ICO_ERR("pthread_mutex_lock error[errno=%d]", errno);
        }

        if (!mQueue.size()) {
            clock_gettime(CLOCK_REALTIME, &now);
            sec = (double)now.tv_sec*1000000000.0 + (double)now.tv_nsec
                    + msec*1000000.0;
            timeout.tv_sec = (time_t)(sec / 1000000000.0);
            timeout.tv_nsec = fmod(sec, 1000000000.0);
//            ICO_DBG("[%ld.%ld]-[%ld.%ld]", now.tv_sec, now.tv_nsec,
//                    timeout.tv_sec, timeout.tv_nsec);
            if (pthread_cond_timedwait(&cond, &mutex, &timeout) != 0){
                if (pthread_mutex_unlock(&mutex) != 0) {
                    ICO_ERR("pthread_mutex_unlock error[errno=%d]", errno);
                }
                return -1;
            }
        }

        item = mQueue.front();
        mQueue.pop();

        if (pthread_mutex_unlock(&mutex) != 0) {
            ICO_ERR("pthread_mutex_unlock error[errno=%d]", errno);
        }

        return 0;
    }

    void push(T item)
    {
        if (pthread_mutex_lock(&mutex) != 0) {
            ICO_ERR("pthread_mutex_lock error[errno=%d]", errno);
        }

        mQueue.push(item);
        pthread_cond_signal(&cond);

        if (pthread_mutex_unlock(&mutex) != 0) {
            ICO_ERR("pthread_mutex_unlock error[errno=%d]", errno);
        }
    }

private:
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    std::queue<T> mQueue;
};

class CJoyStick
{
  public:
            CJoyStick();
    virtual ~CJoyStick();

    virtual int Open();
    virtual int Close();
    virtual int Read(int *number, int *value);
    virtual int ReadData();

    int GetAxisCount() const;
    int GetButtonsCount() const;

    enum TYPE
    {
        AXIS = 0,
        BUTTONS
    };
    int  deviceOpen(const std::string& dirNM, const std::string& fileParts,
                    const std::string& deviceNM, std::string& getDevice);
    void getDevices(const std::string& dir,
                    const std::vector<std::string>& matching,
                    std::vector<std::string>& filesList) const;
    virtual bool getDeviceName(int fd, char* devNM, size_t sz);
    bool recvJS();
    static void *loop(void *arg);

  protected:
    char m_strJoyStick[64];
    int m_nJoyStickID;

    unsigned char m_ucAxes;
    unsigned char m_ucButtons;
    char m_strDevName[80];

    fd_set m_stSet;
    struct pollfd fds;
     CJoyStickQueue<struct js_event > queue;
 private:
    pthread_t  m_threadid;
};

#endif /* CJOYSTICK_H_ */
/**
 * End of File.(CJoyStick.h)
 */
