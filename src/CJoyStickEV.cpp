/*
 * Copyright (c) 2013, TOYOTA MOTOR CORPORATION.
 *
 * This program is licensed under the terms and conditions of the 
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */
/**
 * @file    CJoyStickEV.h
 *
 * Note:
 * processing in the value of the joystick-event in Tizen IVI 2.0
 *  joystick-event
 *    event-type is used JS_EVENT_BUTTON and JS_EVENT_AXIS 
 *    axis/button number value is determine in configuration file
 *      G27.conf G25.conf
 *    event-value memo
 *      Steering value: -32767(Right) <-> 32767(Left)
 *      Accel value:0(full throttle) <-> 32767(free throttle)
 *      Brake value:0(full throttle) <-> 32767(free throttle)
 */
#include <unistd.h>
#include <iostream>
#include <string>
#include <vector>
#include <stdio.h>
#include <poll.h>
#include <sys/io.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/input.h>
#include "CJoyStick.h"
#include "CJoyStickEV.h"
#include "CConf.h"

#include "ico-util/ico_log.h"
#include <errno.h>

using namespace std;

CJoyStickEV::CJoyStickEV()
{
    // TODO Auto-generated constructor stub
    memset(m_absInf, 0, sizeof(m_absInf));
    m_devName = std::string(D_DEV_NAME_G27);
}

CJoyStickEV::~CJoyStickEV()
{
    // TODO Auto-generated destructor stub
}

/**
 * @brief joystick device(event) open
 * @retval number of file descriptor / error
 */
int CJoyStickEV::Open()
{
    string dirpath(D_DEV_DIR_PATH);
    string nameparts(D_DEV_NAME_PARTS_EV);
    string devPath;
    /**
     * get device file
     */
    printf("Open Device Name is <%s>\n", m_devName.c_str());
    int rfd = deviceOpen(dirpath, nameparts, m_devName, devPath);
    if (0 > rfd) {
        printf("Can't open device.\n");
        return rfd;
    }
    m_nJoyStickID = rfd;

    fds.fd = m_nJoyStickID;

    if (pthread_create(&m_threadid, NULL, CJoyStickEV::loop,
                       (void *) this) != 0) {
        cerr << "Failed to create thread." << endl;
        return -1;
    }

    return m_nJoyStickID;
}

/**
 * @brief joystick device file close
 * @return 0:close success
 */
int CJoyStickEV::Close()
{
    if (0 > m_nJoyStickID) {
        return 0;
    }
    return CJoyStick::Close();
}

/**
 * @brief joystick event read loop entrance
 * @return
 */
void *CJoyStickEV::loop(void *arg)
{
    CJoyStickEV *src = reinterpret_cast < CJoyStickEV * >(arg);
    return (void *) src->recvEV();
}

/**
 * @brief joystick event read loop
 * @return
 */
bool CJoyStickEV::recvEV()
{
    while(1) {
        fds.events = POLLIN;
        if (poll(&fds, 1, -1) < 0) {
            ICO_ERR("poll error");
            return false;
        }
        if (0 != (fds.revents & POLLIN)) {
            /**
             * read event
             */
            struct input_event ie;
            int rc;
            while ((rc = read(m_nJoyStickID, &ie, sizeof(ie))) > 0) {
                queue.push(ie);
                if (sizeof(ie) != rc) {
                    ICO_DBG("data not enough [%d/%d]", rc, sizeof(ie));
                }
            }
            if (0 > rc && errno !=11) {  /* read error(errno:11=EAGAIN) */
                ICO_ERR("read error[ret=%d, errno=%d]", rc, errno);
            }
        }
    }
    return false;
}

/**
 * @brief joystick event read
 * @param nubmer joystick event data(convert input_event.code)
 * @param value joystick event data(convert input_event.value)
 * @return convert input_event.type
 *         not -1:joystick event data -1:read data nothing
 */
int CJoyStickEV::Read(int *num, int *val)
{
    /**
     * read event
     */
    struct input_event ie;
    int ret;
    ret = queue.pop(ie, 50.0);
    if (ret != 0) {
        return -1;
    }

    /**
     * event convert
     */
    int r = -1;
    switch (ie.type) {
    case EV_SYN:
        break;
    case EV_KEY:
        r = getJS_EVENT_BUTTON(*num, *val, ie);
        break;
    case EV_REL:
        break;
    case EV_ABS:
        r = getJS_EVENT_AXIS(*num, *val, ie);
        break;
    case EV_MSC:
        break;
    case EV_SW:
        break;
    case EV_LED:
        break;
    case EV_SND:
        break;
    case EV_REP:
        break;
    case EV_FF:
        break;
    case EV_PWR:
        break;
    case EV_FF_STATUS:
        break;
    }
    return r;
}

/**
 * test read
 */
int CJoyStickEV::ReadData()
{
    struct input_event ie;

    int r = read(m_nJoyStickID, &ie, sizeof(ie));

    return 0;
}

/**
 * @brief change input_event value to js_event value
 */
int CJoyStickEV::getJS_EVENT_BUTTON(int& num, int& val,
                                    const struct input_event& s)
{
/*
    if ((BTN_JOYSTICK <= s.code) && (s.code <= BTN_GEAR_UP)) {
        num = s.code - BTN_JOYSTICK;
        val = s.value;
        return JS_EVENT_BUTTON;
    }
    return -1;
*/
    num = s.code;
    val = s.value;
    return JS_EVENT_BUTTON;
}

/**
 * @brief change input_event value to js_event value
 */
int CJoyStickEV::getJS_EVENT_AXIS(int& num, int& val,
                                  const struct input_event& s)
{
    int r = -1;
    switch (s.code) {
    case ABS_X:
        // Convert value Steering
        //    0 to 16353 -> -32766 to 32767
        r = JS_EVENT_AXIS;
        num = (int)s.code;
        val = calc1pm32767((int)s.value, m_absInf[E_ABSX]);
        break;
    case ABS_Y:
        r = JS_EVENT_AXIS;
        num = (int)s.code;
        val = calc1pm32767((int)s.value, m_absInf[E_ABSY]);
        break;
    case ABS_Z:
        r = JS_EVENT_AXIS;
        num = (int)s.code;
        val = calc3p32767Reverse((int)s.value, m_absInf[E_ABSZ]);
        break;
    case ABS_RZ:
        r = JS_EVENT_AXIS;
        num = (int)s.code;
        val = calc3p32767Reverse((int)s.value, m_absInf[E_ABSRZ]);
        break;
    case ABS_HAT0X:
        r = JS_EVENT_AXIS;
        num = (int)s.code;
        val = (int)s.value;
        break;
    case ABS_HAT0Y:
        r = JS_EVENT_AXIS;
        num = (int)s.code;
        val = (int)s.value;
        break;
    defaulr:
        break;
    }
    return r;
}
/**
 * @brief calc value case 1
 *        change to -32767 <-> 32767
 */
int CJoyStickEV::calc1pm32767(int val, const struct input_absinfo& ai)
{
    int a = ai.maximum - ai.minimum;
    double b = (double)val / (double)a;
    int c = ((int) (b * 65534)) - 32767;
    return c;
}

/**
 * @brief calc value case 2
 *        change to 0 <-> 65534
 */
int CJoyStickEV::calc2p65535(int val, const struct input_absinfo& ai)
{
    int a = ai.maximum - ai.minimum;
    double b = (double)val / (double)a;
    int c = (int) (b * 65534);
    return c;
}

/**
 * @brief calc value case 3
 *        change to 0 <-> 32767
 */
int CJoyStickEV::calc3p32767(int val, const struct input_absinfo& ai)
{
    int a = ai.maximum - ai.minimum;
    double b = (double)val / (double)a;
    int c = (int) (b * 32767);
    return c;
}

/**
 * @brief calc value case 3
 *        change to 32767 <-> 0
 */
int CJoyStickEV::calc3p32767Reverse(int val, const struct input_absinfo& ai)
{
    int a = ai.maximum - ai.minimum;
    double b = (double)val / (double)a;
    int c = abs(((int) (b * 32767)) - 32767);
    return c;
}

/**
 * get device name
 */
bool CJoyStickEV::getDeviceName(int fd, char* devNM, size_t sz)
{
    if (0 > fd) {
        return false;
    }
    if (0 > ioctl(fd, EVIOCGNAME(sz), devNM)) {
        return false;
    }
    return true;
}

/**
 * End of File.(CJoyStickEV.cpp)
 */
