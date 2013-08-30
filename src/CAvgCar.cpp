/*
 * Copyright (c) 2013, TOYOTA MOTOR CORPORATION.
 *
 * This program is licensed under the terms and conditions of the 
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */

/**
 * @brief   virtual car
 *          RPM / Speed / Brake Value is determined by the average
 *
 * @date    Apr-08-2013 create
 */

#include <stdlib.h>
#include <time.h>
#include <memory.h>
#include "CAvgCar.h"

/******************************************
 * average Machine
******************************************/

/**
 * @brief averageMachine
 *        Constructor
 * @param sz average sample space size
 */
averageMachine::averageMachine(const short sz)
{
    m_sz = sz;
    m_n = (double) sz;
    m_x = new double[sz];
    memset(m_x, 0, sizeof(double) * sz);
    m_posi = 0;
    m_prev = 0;
    m_t = 0.0;
    m_avg = 0.0;
}

/**
 * @brief ~averageMachine
 *        destructor
 */
averageMachine::~averageMachine()
{
    delete[] m_x;
}

/**
 * @brief setSample
 * @param x sample value
 */
void averageMachine::setSample(double x)
{
    short next = m_posi + 1;
    if (next >= m_sz) {
        next = 0;
    }
    m_x[m_posi] = x;            // store sample value
    m_t += x;                   // get sum
    m_t -= m_x[next];
    m_prev = m_posi;
    m_posi = next;              // next position
    m_avg = m_t / m_n;          // get average
}

/**
 * @brief reCalc
 */
void averageMachine::reCalc()
{
    m_t = 0.0l;
    for (int i = 0; i < m_sz; i++) {
        m_t += m_x[i];
    }
    m_avg = m_t / m_n;
}

/******************************************
 * average Machine Integer
******************************************/

/**
 * @brief iAverageMachine
 *        Constructor
 */
iAverageMachine::iAverageMachine(const short sz)
{
    m_sz = sz;
    m_n = (int) sz;
    m_x = new int[sz];
    memset(m_x, 0, sizeof(int) * sz);
    m_posi = 0;
    m_prev = 0;
    m_t = 0;
    m_avg = 0.0;
    m_iAvg = 0;
}

/**
 * @brief ~iAverageMachine
 *       destructor
 */
iAverageMachine::~iAverageMachine()
{
    delete[] m_x;
}

/**
 * @brief setSample
 * @param x sample value
 */
void iAverageMachine::setSample(int x)
{
    short next = m_posi + 1;
    if (next >= m_sz) {
        next = 0;
    }
    m_x[m_posi] = x;            // store sample value
    m_t += x;                   // get sum
    m_t -= m_x[next];
    m_prev = m_posi;
    m_posi = next;              // next position
    m_avg = (double) m_t / (double) m_n;        // get average
    m_iAvg = m_t / m_n;
}

/**
 * @brief reCalc
 */
void iAverageMachine::reCalc()
{
    m_t = 0;
    for (int i = 0; i < m_sz; i++) {
        m_t += m_x[i];
    }
    m_avg = (double) m_t / (double) m_n;        // get average
    m_iAvg = m_t / m_n;
}

/******************************************
 * Average Engne
******************************************/

/**
 * @brief CAvgEngine
 *        Constructor
 */
CAvgEngine::CAvgEngine(const short sz)
    :m_avgRPM(sz? sz: D_SAMPLE_SPACE_RPM)
{
    m_active = false;
    m_rpm = 0.0;
}

/**
 * @brief ~CAvgEngine
 */
CAvgEngine::~CAvgEngine()
{
}

/**
 * @brief startter
 */
void CAvgEngine::ignitionStart(int rpm)
{
    m_active = true;
    if (-1 != rpm) {
        m_rpm = (double) rpm;
    }
}

/**
 * @brief chgThrottle
 *       Gets the RPM from the throttle
 * @param throttle 0 - 65535
 */
void CAvgEngine::chgThrottle(int throttle)
{
    double rpm = (((double) throttle) / 65534.0l) * 100.0l;
    if (false == isActive()) {
        return;
    }
    m_avgRPM.setSample(rpm);
    m_rpm = m_avgRPM.getAvg();
    if (D_RPM_BOTTOM_BORDER > m_rpm) {
        m_rpm = D_IDLING_RPM + ((double) rand() / RAND_MAX);
    }
}

/******************************************
 * Average Brake
******************************************/
/**
 * @brief CAvgBrake
 *        Constructor
 * @param sz sample space size
 */
CAvgBrake::CAvgBrake(const short sz)
    :m_brk(sz? sz: D_SAMPLE_SPACE_BRAKE)
{
    m_brakeVal = 0;
    m_brake = false;
}

/**
 * @brief ~CAvgBrake
 *        destructor
 */
CAvgBrake::~CAvgBrake()
{
}

/**
 * @brief chgBrake
 *       Gets the Brake Infomation from the brake value
 * @param brake 0 - 65535
 */
void CAvgBrake::chgBrake(int brake)
{
    m_brk.setSample(brake);
    m_brakeVal = m_brk.getIAvg();
    judgeBrake();
}

/**
 * @brief judgeBreak
 */
void CAvgBrake::judgeBrake()
{
    m_brake = false;
    if (D_LAG_BRAKE < m_brakeVal) {
        m_brake = true;
    }
}

/**
 * @brief getSpeed
 * @param sourceSpeed speed
 * @return Calculate the load on the brake
 */
#define D_SOFT_SPEED_STOP   15.0l
#define D_MIDDLE_SPEED_STOP 22.0l
#define D_HARD_SPEED_STOP   30.0l

/* SOFT */
#define D_S_SPEED    30
#define D_S_SPEED_S  0.4
#define D_S_SPEED_M  0.6
#define D_S_SPEED_H  0.8

/* MIDDLE */
#define D_M_SPEED    60
#define D_M_SPEED_S  0.3
#define D_M_SPEED_M  0.5
#define D_M_SPEED_H  0.7

/* HARD */
#define D_H_SPEED_S  0.1
#define D_H_SPEED_M  0.3
#define D_H_SPEED_H  0.5

double CAvgBrake::getSpeed(double sourceSpeed)
{
    if (false == m_brake) {
        return sourceSpeed;
    }

    if (D_SOFT_BRAKE_MAX > m_brakeVal) {
        if (D_SOFT_SPEED_STOP > sourceSpeed) {
            return 0.0;
        }
    }
    else if (D_MIDDLE_BRAKE_MAX > m_brakeVal) {
        if (D_MIDDLE_SPEED_STOP > sourceSpeed) {
            return 0.0;
        }
    }
    else {
        if (D_HARD_SPEED_STOP > sourceSpeed) {
            return 0.0;
        }
    }

    double brakeX;
    if (D_S_SPEED > sourceSpeed) {
        if (D_SOFT_BRAKE_MAX > m_brakeVal) {
            brakeX = D_S_SPEED_S;
        }
        else if (D_MIDDLE_BRAKE_MAX > m_brakeVal) {
            brakeX = D_S_SPEED_M;
        }
        else {
            brakeX = D_S_SPEED_H;
        }
    }
    else if (D_M_SPEED > sourceSpeed) {
        if (D_SOFT_BRAKE_MAX > m_brakeVal) {
            brakeX = D_M_SPEED_S;
        }
        else if (D_MIDDLE_BRAKE_MAX > m_brakeVal) {
            brakeX = D_M_SPEED_M;
        }
        else {
            brakeX = D_M_SPEED_H;
        }
    }
    else {
        if (D_SOFT_BRAKE_MAX > m_brakeVal) {
            brakeX = D_H_SPEED_S;
        }
        else if (D_MIDDLE_BRAKE_MAX > m_brakeVal) {
            brakeX = D_H_SPEED_M;
        }
        else {
            brakeX = D_H_SPEED_H;
        }
    }
    return (sourceSpeed - (sourceSpeed * brakeX));
}

/******************************************
 * Somewhat Gear
******************************************/
/**
 * @brief CAvgGear
 *        Constructor
 */
CAvgGear::CAvgGear()
{
    m_transmission = E_SHIFT_PARKING;
    m_transmissionOLD = m_transmission;
    m_transmissionValue = D_SHIFT_VALUE_PARKING;
}

/**
 * @brief ~CAvgGear
 *        Destructor
 */
CAvgGear::~CAvgGear()
{

}

/**
 * @brief getGearRatio
 * @param speed
 * @param bDown true:speed down
 * @return Gear Ratio
 */
const double gr1 = 1.0 / 4.1;
const double gr2 = 1.0 / 2.8;
const double gr3 = 1.0 / 1.8;
const double grD1 = 1.0 / 2.3;
const double grD2 = 1.0 / 1.4;
const double grD3 = 1.0 / 1.0;
const double grD4 = 1.0 / 0.8;
const double grD5 = 1.0 / 0.46;
const double grR = 1.0 / 3.1;

const double atd1 = 9.0l;
const double atd2 = 15.0l;
const double atd3 = 23.0l;
const double atd4 = 32.0l;
const double atd5 = 42.0l;

double CAvgGear::getGearRatio(double speed, bool bDown) const
{
    double gr;
    switch (m_transmission) {
    case E_SHIFT_FIRST:
        gr = gr1;
        break;
    case E_SHIFT_SECOND:
        gr = gr2;
        break;
    case E_SHIFT_THIRD:
        gr = gr3;
    case E_SHIFT_DRIVE:
        // Speed by changing the gear ratio
        if (atd1 > speed) {
            gr = grD1;
        }
        else if (atd2 > speed) {
            gr = grD2;
            if (true == bDown) {
                gr = gr1;
            }
        }
        else if (atd3 > speed) {
            gr = grD3;
            if (true == bDown) {
                gr = gr2;
            }
        }
        else if (atd4 > speed) {
            gr = grD4;
            if (true == bDown) {
                gr = gr3;
            }
        }
        else {
            gr = grD5;
            if (true == bDown) {
                gr = grD3;
            }
        }
        break;
    case E_SHIFT_REVERSE:
        gr = grR;
        break;
    case E_SHIFT_PARKING:
    case E_SHIFT_NEUTRAL:
        gr = 0.0l;
        break;
    case E_SHIFT_MT_FIRST:
        gr = gr1;
        break;
    case E_SHIFT_MT_SECOND:
        gr = grD1;
        break;
    case E_SHIFT_MT_THIRD:
        gr = grD2;
        break;
    case E_SHIFT_MT_FOURTH:
        gr = grD3;
        break;
    case E_SHIFT_MT_FIFTH:
        gr = grD4;
        break;
    case E_SHIFT_MT_SIXTH:
        gr = grD5;
        break;
    default:
        gr = 0.0l;
        break;
    }
    return gr;
}

/**
 * @brief setShiftUp
 */
void CAvgGear::setShiftDown()
{
    switch (m_transmission) {
    case E_SHIFT_PARKING:
        m_transmissionOLD = m_transmission;
        m_transmission = E_SHIFT_REVERSE; // PARKING -> REVERSE
        m_transmissionValue = D_SHIFT_VALUE_REVERSE;
        break;
    case E_SHIFT_REVERSE:
        m_transmissionOLD = m_transmission;
        m_transmission = E_SHIFT_NEUTRAL; // REVERSE -> NEUTRAL
        m_transmissionValue = D_SHIFT_VALUE_NEUTRAL;
        break;
    case E_SHIFT_NEUTRAL:
        m_transmissionOLD = m_transmission;
        m_transmission = E_SHIFT_DRIVE;   // NEUTRAL -> DRIVE
        m_transmissionValue = D_SHIFT_VALUE_DRIVE;
        break;
    case E_SHIFT_DRIVE:
        m_transmissionOLD = m_transmission;
        m_transmission = E_SHIFT_THIRD;   // DRIVE   -> THIRD
        m_transmissionValue = D_SHIFT_VALUE_THIRD;
        break;
    case E_SHIFT_THIRD:
        m_transmissionOLD = m_transmission;
        m_transmission = E_SHIFT_SECOND;  // THIRD   -> SECOND
        m_transmissionValue = D_SHIFT_VALUE_SECOND;
        break;
    case E_SHIFT_SECOND:
        m_transmissionOLD = m_transmission;
        m_transmission = E_SHIFT_FIRST;   // SECOND  -> FIRST
        m_transmissionValue = D_SHIFT_VALUE_FIRST;
        break;
    case E_SHIFT_FIRST:
        m_transmissionOLD = m_transmission;
        m_transmission = E_SHIFT_FIRST;   // NO CHANGE
        m_transmissionValue = D_SHIFT_VALUE_FIRST;
        break;
    }
}

/**
 * @brief setShiftUp
 */
void CAvgGear::setShiftUp()
{
    switch (m_transmission) {
    case E_SHIFT_FIRST:
        m_transmissionOLD = m_transmission;
        m_transmission = E_SHIFT_SECOND;  // FIRST   -> SECOND
        m_transmissionValue = D_SHIFT_VALUE_SECOND;
        break;
    case E_SHIFT_SECOND:
        m_transmissionOLD = m_transmission;
        m_transmission = E_SHIFT_THIRD;   // SECOND  -> THIRD
        m_transmissionValue = D_SHIFT_VALUE_THIRD;
        break;
    case E_SHIFT_THIRD:
        m_transmissionOLD = m_transmission;
        m_transmission = E_SHIFT_DRIVE;   // THIRD -> DRIVE
        m_transmissionValue = D_SHIFT_VALUE_DRIVE;
        break;
    case E_SHIFT_DRIVE:
        m_transmissionOLD = m_transmission;
        m_transmission = E_SHIFT_NEUTRAL; // DRIVE -> NEUTRAL
        m_transmissionValue = D_SHIFT_VALUE_NEUTRAL;
        break;
    case E_SHIFT_NEUTRAL:
        m_transmissionOLD = m_transmission;
        m_transmission = E_SHIFT_REVERSE; // NEUTRAL -> REVERSE
        m_transmissionValue = D_SHIFT_VALUE_REVERSE;
        break;
    case E_SHIFT_REVERSE:
        m_transmissionOLD = m_transmission;
        m_transmission = E_SHIFT_PARKING; // REVERSE  -> PARKING
        m_transmissionValue = D_SHIFT_VALUE_PARKING;
        break;
    case E_SHIFT_PARKING:
        m_transmissionOLD = m_transmission;
        m_transmission = E_SHIFT_PARKING; // NO CHANGE
        m_transmissionValue = D_SHIFT_VALUE_PARKING;
        break;
    }
}

void CAvgGear::setShiftMT(E_AT_GEAR tm) {
    switch(tm) {
    case E_SHIFT_MT_FIRST :
        m_transmissionOLD = m_transmission;
        m_transmission = E_SHIFT_MT_FIRST;
        m_transmissionValue = D_SHIFT_VALUE_FIRST;
        break;
    case E_SHIFT_MT_SECOND :
        m_transmissionOLD = m_transmission;
        m_transmission = E_SHIFT_MT_SECOND;
        m_transmissionValue = D_SHIFT_VALUE_SECOND;
        break;
    case E_SHIFT_MT_THIRD :
        m_transmissionOLD = m_transmission;
        m_transmission = E_SHIFT_MT_THIRD;
        m_transmissionValue = D_SHIFT_VALUE_THIRD;
        break;
    case E_SHIFT_MT_FOURTH :
        m_transmissionOLD = m_transmission;
        m_transmission = E_SHIFT_MT_FOURTH;
        m_transmissionValue = D_SHIFT_VALUE_DRIVE;
        break;
    case E_SHIFT_MT_FIFTH :
        m_transmissionOLD = m_transmission;
        m_transmission = E_SHIFT_MT_FIFTH;
        m_transmissionValue = D_SHIFT_VALUE_DRIVE;
        break;
    case E_SHIFT_MT_SIXTH :
        m_transmissionOLD = m_transmission;
        m_transmission = E_SHIFT_MT_SIXTH;
        m_transmissionValue = D_SHIFT_VALUE_DRIVE;
        break;
    case E_SHIFT_REVERSE :
        m_transmissionOLD = m_transmission;
        m_transmission = E_SHIFT_REVERSE;
        m_transmissionValue = D_SHIFT_VALUE_REVERSE;
        break;
    case E_SHIFT_NEUTRAL :
        m_transmissionOLD = m_transmission;
        m_transmission = E_SHIFT_NEUTRAL;
        m_transmissionValue = D_SHIFT_VALUE_NEUTRAL;
        break;
    default :
        break;
    }
}

/******************************************
 * Average Car
******************************************/
/**
  * @brief CAvgCar
  *   constractor
  */
CAvgCar::CAvgCar(const short RPMsz, const short SPDsz,
                             const short BRKsz)
    : m_brake(BRKsz), m_engine(RPMsz),
      m_speed(SPDsz? SPDsz: D_SAMPLE_SPACE_SPEED),
      m_accPedalOpen(D_SAMPLE_SPACE_ACCPEDAL)
{
    m_odometer = 0.0;
    m_tp.tv_sec = 0;
    m_tp.tv_nsec = 0;
    m_bChgThrottle = false;
    m_valThrottle = -1;
    m_bChgBrake = false;
    m_valBrake = -1;
}

/**
  * @brief CAvgCar
  *   destonstractor
  */
CAvgCar::~CAvgCar()
{

}

/**
 * @brief chgThrottle
 * @param throttle CAvgEngine parameter
 */
void CAvgCar::chgThrottle(int throttle)
{
    int x = abs((throttle - 32767));
    if (false == m_engine.isActive()) {
        if ((true == isNeutral()) || (true == isParking())) {
            m_engine.ignitionStart(D_RPM_IGNITION_VALUE);
        }

        for (int i; i < D_SAMPLE_SPACE_ACCPEDAL; i++) {
            m_accPedalOpen.setSample(x);
        }

        return;
    }
    else {
        m_engine.chgThrottle(x);
        m_accPedalOpen.setSample(x);
        m_bChgThrottle = true;
        m_valThrottle  = x;
    }
}

/**
  * @brief chgBrake
  * @param BrakeVal CAvgBrake parameter
  */
void CAvgCar::chgBrake(int brakeVal)
{
    int x = abs((brakeVal - 32767));
    m_brake.chgBrake(x);
    m_bChgBrake = true;
    m_valBrake = x;
}

/**
 * @brief calc
 *  speed and range calc
 */
#define D_DOUBLE_SET_SPEED_MAX atd4
void CAvgCar::calc()
{
    double oldSpd = getSpeed();
    double rpm = getRPM();
    bool bDown = false;
    if (D_RPM_BOTTOM_BORDER > m_engine.getAvgRPM()){
        bDown = true;
    }
    double gearRatio = getGearRatio(oldSpd,bDown);
    double tmpX = rpm * gearRatio / 100;

    double x = m_brake.getSpeed(tmpX);
    m_speed.setSample(x);
    /**
     * Processing for the acceleration
     */
    if ((oldSpd < x) && (atd4 > x)){
        m_speed.setSample(x);
        m_speed.setSample(x);
    }
    /**
     * Processing for the brake
     */
    if (true == m_brake.isOnBrake()) {
        if (oldSpd >= x) {
            if ((atd1 > oldSpd) && (0 == x)) {
                m_speed.setSample(x);
                m_speed.setSample(x);
                m_speed.setSample(x);
                m_speed.setSample(x);
            }
        }
    }
    double spd = m_speed.getAvg();
    struct timespec oldtp;
    oldtp.tv_sec = m_tp.tv_sec;
    oldtp.tv_nsec = m_tp.tv_nsec;
    clock_gettime(CLOCK_MONOTONIC, &m_tp);
    if (0.0 != spd) {
        long int sec  = m_tp.tv_sec  - oldtp.tv_sec; // sec
        long int nsec = m_tp.tv_nsec - oldtp.tv_nsec; // nano sec
        double r_nsec = (((double)sec * 1000000000l) + 
                          (double)nsec) / 1000000000l; // get time(nanosec)
        m_currentRunMeter = (((double)(spd / 3600)) * 1000) * r_nsec; // get ran lengths
        if (true == isReverse()) {
            double x = 0 - m_currentRunMeter;
            m_currentRunMeter = x;
        }
        else if (true == isNeutral()) {
            if (true == isReverseLastTime()) {
                double x = 0 - m_currentRunMeter;
                m_currentRunMeter = x;
            }
        }
        m_tripmeter += m_currentRunMeter;
        m_odometer += m_currentRunMeter;
    }
    else {
        reset_old();
        m_currentRunMeter = 0.0l;
    }
}
/**
 * @brief updateAvg
 */
void CAvgCar::updateAvg()
{
    if ((false == m_bChgThrottle) && (-1 != m_valThrottle)) {
        m_engine.chgThrottle(m_valThrottle);
        m_accPedalOpen.setSample(m_valThrottle);
    }
    m_bChgThrottle = false;
    if ((false == m_bChgBrake) && (-1 != m_valBrake)){
        m_brake.chgBrake(m_valBrake);
    }
    m_bChgBrake = false;
    calc();
}
