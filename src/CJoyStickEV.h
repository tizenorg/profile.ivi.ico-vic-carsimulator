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
 * @file    CJoyStickEV.h
 */

#ifndef CJOYSTICKEV_H_
#define CJOYSTICKEV_H_

#include <fcntl.h>
#include <linux/joystick.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>

#include "CJoyStick.h"

#define D_DEV_NAME_PARTS_EV "event"

class CJoyStickEV : public CJoyStick
{
  public:
            CJoyStickEV();
    virtual ~CJoyStickEV();

    virtual int Open();
    virtual int Close();
    virtual int Read(int *number, int *value);
    virtual int ReadData();
    virtual bool getDeviceName(int fd, char* devNM, size_t sz);
    bool deviceGrab(int fd);
    bool deviceGrabRelese(int fd);
    int getJS_EVENT_BUTTON(int& num, int& val, const struct input_event& s);
    int getJS_EVENT_AXIS(int& num, int& val, const struct input_event& s);
    int calc1pm32767(int val, const struct input_absinfo& ai);
    int calc2pm32767(int val, const struct input_absinfo& ai);
    enum {
        E_ABSX = 0, /* ABS_X */
        E_ABSY,     /* ABS_Y */
        E_ABSZ,
        E_ABSRZ,
        E_ABSHAT0X, /* ABS_HAT0X */
        E_ABSHAT0Y, /* ABS_HAT0Y */
        E_ABSMAX    /* */
    };
    struct input_absinfo m_absInf[E_ABSMAX];
    bool m_grab;
    std::string m_devName;
  private:
};

#endif /* CJOYSTICKEV_H_ */
/**
 * End of File.(CJoyStickEV.h)
 */
