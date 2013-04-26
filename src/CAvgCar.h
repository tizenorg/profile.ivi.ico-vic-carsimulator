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


#ifndef CAVGCAR_H
#define CAVGCAR_H

/*******************************
 * AVERAGE SAMPLE SPACE SIZE
*******************************/
#define D_SAMPLE_SPACE_SPEED    40
#define D_SAMPLE_SPACE_RPM      40
#define D_SAMPLE_SPACE_BRAKE    20
#define D_SAMPLE_SPACE_ACCPEDAL 40

/******************************************
 * average Machine
******************************************/
class averageMachine
{
  public:
            averageMachine(const short sz);
            ~averageMachine();
    void    setSample(double x);
    double  getAvg() const;
    double  getPrevValue() const;
    void    reCalc();
  private:
    short   m_posi;             // Sample store position
    short   m_prev;
    short   m_sz;               // Sample space size
    double  m_n;                // Sample space size(double)
    double* m_x;                // Sample store Aarea
    double  m_t;                // Sample sum
    double  m_avg;              // average
};

/**
 * @brief getAvg
 * @return average
 */
inline double averageMachine::getAvg() const
{
    return m_avg;
}

/**
 * @brief getPrevValue
 * @return previous RPM value
 */
inline double averageMachine::getPrevValue() const
{
    return m_x[m_prev];
}

/******************************************
 * average Machine Integer
******************************************/
class iAverageMachine
{
  public:
            iAverageMachine(const short sz);
            ~iAverageMachine();
    void    setSample(int x);
    double  getAvg() const;
    int     getIAvg() const;
    int     getPrevValue() const;
    void    reCalc();
  private:
    short   m_posi;             // Sample store position
    short   m_prev;
    short   m_sz;               // Sample space size
    int     m_n;                // Sample space size(double)
    int*    m_x;                // Sample store Aarea
    int     m_t;                // Sample sum
    double  m_avg;              // average
    int     m_iAvg;             // average
};

/**
 * @brief getAvg
 *        get average
 * @return avarege value
 */
inline double iAverageMachine::getAvg() const
{
    return m_avg;
}

/**
 * @brief getIAvg
 *        get average interger
 * @return avarege value
 */
inline int iAverageMachine::getIAvg() const
{
    return m_iAvg;
}

/**
 * @breif getPrevValue
 * @return prev sample value
 */
inline int iAverageMachine::getPrevValue() const
{
    return m_x[m_prev];
}

/******************************************
 * Average Engne
******************************************/
#define D_IDLING_RPM            6.3L
#define D_RPM_BOTTOM_BORDER     2.7L
#define D_RPM_IGNITION_VALUE    4.1

class CAvgEngine
{
  public:
            CAvgEngine(const short sz = 0);
            ~CAvgEngine();
    void    ignitionStart(int rpm = -1);
    void    chgThrottle(int throttle);
    double  getRPM() const;
    double  getAvgRPM() const;
    bool    isActive() const;

  protected:
    bool    m_active;
    double  m_rpm;
    averageMachine  m_avgRPM;
};

/**
 * @brief getRPM
 * @return RPM value
 */
inline double CAvgEngine::getRPM() const
{
    return m_rpm * 100.0l;
}
/**
 * @brief getAvgRPM
 * @return RPM value
 */
inline double CAvgEngine::getAvgRPM() const
{
    return m_avgRPM.getAvg();
}

/**
 * @brief isActive
 * @return true:active / false stop
 */
inline bool CAvgEngine::isActive() const
{
    return m_active;
}

/******************************************
 * Average Brake
******************************************/
#define D_LAG_BRAKE            10240
#define D_MAX_BRAKE            65534
#define D_SOFT_BRAKE_MAX       21000
#define D_MIDDLE_BRAKE_MAX     42000
class CAvgBrake
{
  public:
            CAvgBrake(const short sz = 0);
            ~CAvgBrake();
    void    chgBrake(int brake);
    bool    isOnBrake() const;
    int     getBrakeAvg() const;
    double  getSpeed(double sourceSpeed);
    int     calcPressure(int brakeVal) const;
protected:
    void    judgeBrake();
  private:
    iAverageMachine m_brk;
    int     m_brakeVal;
    bool    m_brake;
};

/**
 * @breif isOnBrake
 * @return true:on brake false:no brake
 */
inline bool CAvgBrake::isOnBrake() const
{
    return m_brake;
}

/**
 * @breif
 * @return
 */
inline int CAvgBrake::getBrakeAvg() const
{
    return m_brakeVal;
}


inline int CAvgBrake::calcPressure(int brakeVal) const
{
    if (D_LAG_BRAKE >= brakeVal) {
        return 0;
    }
    int a = brakeVal - D_LAG_BRAKE;
    int b = D_MAX_BRAKE - D_LAG_BRAKE;
    double x = ((double)a / (double)b) * 100.0l;
    int r = (int)x;
    return r;
}

/******************************************
 * Somewhat Gear
******************************************/
/**
 * TRANSMISSION SHIFT VALUE
 * 1=1st, 2=2nd, 3=3rd, ..., 64=neutral,
 * 128=back, 256=cvt
 */
#define D_SHIFT_VALUE_CVT      256
#define D_SHIFT_VALUE_PARKING  0
#define D_SHIFT_VALUE_REVERSE  128
#define D_SHIFT_VALUE_NEUTRAL  64
#define D_SHIFT_VALUE_DRIVE    4
#define D_SHIFT_VALUE_THIRD    3
#define D_SHIFT_VALUE_SECOND   2
#define D_SHIFT_VALUE_FIRST    1
/**
 * TRANSMISSION SHIFT MODE
 * 0=normal, 1=sports, 2=eco, 3=oem custom,
 * 4=oem custom2
 */
#define D_SHIFT_MODE_NORMAL     0
#define D_SHIFT_MODE_SPOETS     1
#define D_SHIFT_MODE_ECO        2
#define D_SHIFT_MODE_OEMCUSTOM  3
#define D_SHIFT_MODE_OEMCUSTOM2 3

class CAvgGear
{
  public:
    /**
     * TRANSMISSION SHIFT POSITION
     * 0=P, 1=R, 2=N, 4=D, 5=L, 6=2, 7=oem
     */
    enum E_AT_GEAR
    {
        E_SHIFT_PARKING = 0, // 0=P
        E_SHIFT_REVERSE,     // 1=R
        E_SHIFT_NEUTRAL,     // 2=N
        E_SHIFT_DRIVE = 4,   // 4=N
        E_SHIFT_FIRST,       // 5=L
        E_SHIFT_SECOND,      // 6=2
        E_SHIFT_THIRD        // 7=oem
    };

            CAvgGear();
            ~CAvgGear();

    void    setShiftDown();
    void    setShiftUp();
    E_AT_GEAR getSelectGear() const;
    int     getValue() const;
    int     getMode() const;
    double  getGearRatio(double speed, bool down = false) const;
    void    chgGear(E_AT_GEAR tm);
    bool    isReverse() const;
    bool    isNeutral() const;
    bool    isParking() const;
  private:
    E_AT_GEAR m_transmission;
    int       m_transmissionValue;
};

/**
 * @brief chgGear
 * @param tm
 */
inline void CAvgGear::chgGear(E_AT_GEAR tm)
{
    m_transmission = tm;
}

/**
 * @brief isReverse
 * @return true / false
 */
inline bool CAvgGear::isReverse() const
{
    if (E_SHIFT_REVERSE == m_transmission) {
        return true;
    }
    return false;
}

/**
 * @brief isNeutral
 * @return true / false
 */
inline bool CAvgGear::isNeutral() const
{
    if (E_SHIFT_NEUTRAL == m_transmission) {
        return true;
    }
    return false;
}

/**
 * @brief isParking
 * @return true / false
 */
inline bool CAvgGear::isParking() const
{
    if (E_SHIFT_PARKING == m_transmission) {
        return true;
    }
    return false;
}

/**
 * @vrief getSelectGear
 * @return transmission position is amb transmission position
 */
inline CAvgGear::E_AT_GEAR CAvgGear::getSelectGear() const
{
    return m_transmission;
}

/**
 * @brief getValue
 * @return transmission value is amd transmission value
 */
inline int CAvgGear::getValue() const
{
    return m_transmissionValue;
}

/**
 * @brief getMode
 * @return transmission Mode is amd transmission Mode
 */
inline int CAvgGear::getMode() const
{
    return D_SHIFT_MODE_NORMAL;
}

/******************************************
 * Average Car
******************************************/
#define D_ACCPEDAL_OPEN 65534

class CAvgCar:public CAvgGear
{
public:
            CAvgCar(const short RPMsz = 0,
                    const short SPDsz = 0,
                    const short BRKsz = 0);
            ~CAvgCar();

    void    chgThrottle(int throttle);
    double  getRPM() const;
    bool    isEngineActive() const;

    void    chgBrake(int brakeVal);
    bool    isOnBrake() const;
    int     getBrakeAvg() const;
    int     calcPressure(int brakeVal) const;

    void    calc();
    double  getSpeed() const;
    double  getCurrentRun() const;
    double  getTotalRun() const;
    int     calcAccPedalOpen() const;
    double  getTripmeter() const;
    void    tripmeterReset();

    void    updateAvg();
private:

    CAvgBrake m_brake;
    CAvgEngine m_engine;

    double  m_odometer;
    double  m_currentRunMeter;
    double  m_tripmeter;

    struct timespec m_tp;
    averageMachine  m_speed;

    iAverageMachine m_accPedalOpen;
    bool    m_bChgThrottle;
    int     m_valThrottle;
    bool    m_bChgBrake;
    int     m_valBrake;
};

/**
 * @breif getRPM
 * @return RPM
 */
inline double CAvgCar::getRPM() const
{
    return m_engine.getRPM();
}

/**
 * @brief isEngineActive
 * @return true / false
 */
inline bool CAvgCar::isEngineActive() const
{
    return m_engine.isActive();
}

/**
 * @brief isOnBrake
 * @return true / false
 */
inline bool CAvgCar::isOnBrake() const
{
    return m_brake.isOnBrake();
}

/**
 * @brief getSpeed()
 * @return speed
 */
inline double CAvgCar::getSpeed() const
{
    return m_speed.getAvg();
}

/**
 * @brief getCurrntRun
 * @return meter run range
 */
inline double CAvgCar::getCurrentRun() const
{
    return m_currentRunMeter;
}

/**
 * @brief getTotalRun
 * @return odo meter value
 */
inline double CAvgCar::getTotalRun() const
{
    return m_odometer;
}

/**
 * @brief getBrakeAvg
 * @return brake avveage valeu
 */
inline int CAvgCar::getBrakeAvg() const
{
    return m_brake.getBrakeAvg();
}

/**
 * @brief calcPressure
 * @return
 */
inline int CAvgCar::calcPressure(int brakeVal) const
{
    return m_brake.calcPressure(brakeVal);
}

/**
 * @brief calcAccPedalOpen
 * @return 
 */
inline int CAvgCar::calcAccPedalOpen() const
{
    double x = (double)m_accPedalOpen.getIAvg() / (double)D_ACCPEDAL_OPEN;
    return  (int)(x*100.0);
}
/**
 * @breif get Tripmeter
 * @return trip meter value
 */
inline double CAvgCar::getTripmeter() const
{
    return m_tripmeter;
}
/**
 * @breif tripmeterReset
 *        tripmeter reset
 */
inline void CAvgCar::tripmeterReset()
{
    m_tripmeter = 0;
}

#endif // CAVGCAR_H
