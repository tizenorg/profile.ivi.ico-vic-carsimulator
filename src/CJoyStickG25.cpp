#include <iostream>

#include "CJoyStickG25.h"

CJoyStickG25::CJoyStickG25() : CJoyStickEV() {
}

CJoyStickG25::~CJoyStickG25() {
}

int CJoyStickG25::Open() {
    m_devName = std::string(D_DEV_NAME_G25);
    int rfd = CJoyStickEV::Open();
    if (rfd < 0) {
        return rfd;
    }

    if (0 > ioctl(rfd, EVIOCGABS(ABS_X), &m_absInf[E_ABSX])) {
        std::cerr << "ioctl(EVIOCGABS(ABS_X)) get error" << std::endl;
        m_absInf[E_ABSX].minimum = 0;
        m_absInf[E_ABSX].maximum = 16384;
    }
    if (0 > ioctl(rfd, EVIOCGABS(ABS_Y), &m_absInf[E_ABSY])) {
        std::cerr << "ioctl(EVIOCGABS(ABS_Y)) get error" << std::endl;
        m_absInf[E_ABSY].minimum = -32767;
        m_absInf[E_ABSY].maximum = 32767;
    }
    if (0 > ioctl(rfd, EVIOCGABS(ABS_Z), &m_absInf[E_ABSZ])) {
        std::cerr << "ioctl(EVIOCGABS(ABS_Z)) get error" << std::endl;
        m_absInf[E_ABSZ].minimum = -32767;
        m_absInf[E_ABSZ].maximum = 32767;
    }
    if (0 > ioctl(rfd, EVIOCGABS(ABS_HAT0X), &m_absInf[E_ABSHAT0X])) {
        std::cerr << "ioctl(EVIOCGABS(ABS_HAT0X)) get error" << std::endl;
        m_absInf[E_ABSHAT0X].minimum = -1;
        m_absInf[E_ABSHAT0X].maximum = 1;
    }
    if (0 > ioctl(rfd, EVIOCGABS(ABS_HAT0Y), &m_absInf[E_ABSHAT0Y])) {
        std::cerr << "ioctl(EVIOCGABS(ABS_HAT0Y)) get error" << std::endl;
        m_absInf[E_ABSHAT0Y].minimum = -1;
        m_absInf[E_ABSHAT0Y].maximum = 1;
    }
    return rfd;
}

/**
 * @brief change input_event value to js_event value
 * Since the direction of change value is different from the G27.
 */
int CJoyStickG25::getJS_EVENT_AXIS(int& num, int& val,
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
        val = calc3p32767Reverse((int)s.value, m_absInf[E_ABSY]);
        break;
    case ABS_Z:
        r = JS_EVENT_AXIS;
        num = (int)s.code;
        val = calc3p32767((int)s.value, m_absInf[E_ABSZ]);
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
