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
        m_absInf[E_ABSY].minimum = 0;
        m_absInf[E_ABSY].maximum = 255;
    }
    if (0 > ioctl(rfd, EVIOCGABS(ABS_Z), &m_absInf[E_ABSZ])) {
        std::cerr << "ioctl(EVIOCGABS(ABS_Z)) get error" << std::endl;
        m_absInf[E_ABSZ].minimum = 0;
        m_absInf[E_ABSZ].maximum = 255;
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

