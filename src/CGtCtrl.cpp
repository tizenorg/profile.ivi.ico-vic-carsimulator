/*
 * Copyright (c) 2013, TOYOTA MOTOR CORPORATION.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */
/**
 * @brief   Pseudo-driving vehicle
 *
 * @date    Jun-21-2012 create
 * @date    Apl-09-2013 RPM/Speed/Breake/GPS/direction Reform
 * @file    CGtCtrl.cpp
 */
#include <unistd.h>
#include <fstream>
#include "CJoyStick.h"
#include "CJoyStickEV.h"
#include "CGtCtrl.h"
#include "CAvgCar.h"
#include "CCalc.h"

#include <sys/time.h>
#include "ico-util/ico_log.h"

extern bool gbDevJs;

bool g_bStopFlag;
std::queue < geoData > routeList;
std::string msgQueue = "";
std::string orgmsgQueue = "";
int Daemon_MS;
int nClient = 0;
int nDelayedCallback = 0;

int ReadPriv = 0;
int WritePriv = 0;

int Debug = 0;

pthread_mutex_t mutex;

/**
 * Average ENGINE or CAR Sample Space size
 */
short g_RPM_SAMPLE_SPACE_SIZE    = 60;
short g_SPEED_SAMPLE_SPACE_SIZE  = 180;
short g_BREAKE_SAMPLE_SPACE_SIZE = 10;

/**
 * GPS Info update flag
 */
bool   g_bSentInMileage   = true;
bool   g_bSentInChgLngLat = true;
double g_SentInMileage    = 3.0;  // 3 meter
double g_StartLatitude    =  35.47945;
double g_StartLongitude   = 139.40026;
double g_GoalLatitude     =  35.49746;
double g_GoalLongitude    = 139.40504;

using namespace std;
#define _D_URI_FORM "ws://%s:%d"
#define _D_HOST_STR "127.0.0.1"
const int protocols_sz = 4;
const char* protocolsName[protocols_sz] = {
    "standarddatamessage-only",
    "standardcontrolmessage-only",
    "customdatamessage-only",
    "customcontrolmessage-only"
};


CGtCtrl::CGtCtrl()
{
    m_strConfPath = "/usr/bin/CarSimDaemon.conf";
    // TODO Auto-generated constructor stub
    signal(SIGINT, CGtCtrl::signal_handler);
    signal(SIGQUIT, CGtCtrl::signal_handler);
    signal(SIGKILL, CGtCtrl::signal_handler);
    signal(SIGTERM, CGtCtrl::signal_handler);
    signal(SIGCHLD, CGtCtrl::signal_handler);

    m_bUseGps = false;
    myJS = NULL;
}

CGtCtrl::~CGtCtrl()
{
    // TODO Auto-generated destructor stub
    if (NULL != myJS) {
        delete myJS;
        myJS = NULL;
    }
}

void CGtCtrl::signal_handler(int signo)
{
    switch (signo) {
    case SIGINT:
    case SIGKILL:
    case SIGQUIT:
    case SIGTERM:
        g_bStopFlag = false;
        printf("receive cancel command(%d).\n", signo);
        sleep(1);
        break;
    case SIGALRM:
        break;
    }
}


bool CGtCtrl::Initialize()
{
    m_nJoyStickID = -1;

    m_stVehicleInfo.fLng = g_StartLongitude;
    m_stVehicleInfo.fLat = g_StartLatitude;
    m_stVehicleInfo.nSteeringAngle = 0;
    m_stVehicleInfo.nShiftPos = 0;
    m_stVehicleInfo.bHazard = false;
    m_stVehicleInfo.bWinkR = false;
    m_stVehicleInfo.bWinkL = false;
    m_stVehicleInfo.nAirconTemp = 250;
    m_stVehicleInfo.nHeadLightPos = 1;
    m_stVehicleInfo.nVelocity = 0;
    m_stVehicleInfo.nDirection = 0;
    m_stVehicleInfo.dVelocity = 0.0;
    m_stVehicleInfo.nAccel = 0;
    m_stVehicleInfo.bHeadLight = false; // HEAD LIGHT OFF(false)

    m_bFirstOpen = true;
    if (true == gbDevJs) {
        myJS = new CJoyStick;
        int nRet = myJS->Open();
        if (nRet < 0) {
            printf("JoyStick open error\n");
            return false;
        }
    }
    else {
        int i = 0;
        do {
            switch (i) {
            case 0 :
                printf("Load class G27\n");
                myJS = new CJoyStickG27;
                m_strConfPath = g_ConfPathG27;
                break;
            case 1 :
                printf("Load class G25\n");
                myJS = new CJoyStickG25;
                m_strConfPath = g_ConfPathG25;
                break;
            case 2 :
                printf("Load class EV\n");
                myJS = new CJoyStickEV;
                m_strConfPath = g_ConfPathG27;
                break;
            default :
                break;
            }
            int nRet = myJS->Open();
            if (nRet > 0) {
                break;
            }
            delete myJS;
            myJS = NULL;
        } while ((++i) < g_JoyStickTypeNum);
        if (myJS == NULL) {
            return false;
        }
    }

    myConf.LoadConfig(m_strConfPath.c_str());
    m_stVehicleInfo.fLng = myConf.m_fLng;
    m_stVehicleInfo.fLat = myConf.m_fLat;

    m_viList.init();

    if (!LoadConfigJson(myConf.m_sAmbConfigName.c_str())) {
        printf("AMB configfile read error\n");
        return false;
    }

    m_sendMsgInfo.clear();

    char uri[128];
    for (int i = 0; i < protocols_sz; i++) {
        sprintf(uri, _D_URI_FORM, _D_HOST_STR, m_ambpicomm_port[i]);
        if (false == m_ambpicomm_client[i].start(uri, protocolsName[i])) {
            return false;   // connect fail
        }
        while(m_ambpicomm_client[i].getM_id() == NULL) {
            usleep(100*1000);
        }
        ICO_DBG("websocket: OK [%s][%s]", uri, protocolsName[i]);
    }

    const char *vi = "LOCATION";
    double location[] = { myConf.m_fLat, myConf.m_fLng, 0 };
    SendVehicleInfo(dataport_def, vi, &location[0], 3);

    if (m_bDemoRunning) {
        std::cout << "Demo Mode." << std::endl;
        LoadRouteList();
    }

    return true;
}

bool CGtCtrl::Terminate()
{
    bool b = true;

    if (myJS != NULL) {
        myJS->Close();
    }
    return b;
}

#define sENGINE_SPEED   "ENGINE_SPEED"
#define sBRAKE_SIGNAL   "BRAKE_SIGNAL"
#define sBRAKE_PRESSURE "BRAKE_PRESSURE"
#define sACCPEDAL_OPEN  "ACCPEDAL_OPEN"
#define sVELOCITY       "VELOCITY"
#define sDIRECTION      "DIRECTION"
#define sLOCATION       "LOCATION"
#define sSHIFT          "SHIFT"
#define sLIGHTSTATUS    "LIGHTSTATUS"
void CGtCtrl::Run()
{
    g_bStopFlag = true;

    int type = -1;
    int number = -1;
    int value = -1;

    /**
      * INTERVAL CONTROL
      */
    struct timeval now;
    struct timeval timeout;
    long usec;
    int intervalctl = 0;
    int fcycle = 0;
    gettimeofday(&now, NULL);
    usec = now.tv_usec + D_RUNLOOP_INTERVAL_COUNT*10*1000;
    timeout.tv_usec = usec % 1000000;
    if (usec < 1000000) {
        timeout.tv_sec = now.tv_sec;
    } else {
        timeout.tv_sec = now.tv_sec + 1;
    }
    /**
      * LAST STEERING VALUE
      */
    int last_steering = 0;
    int last_angle = 0;
    /**
     * RPM/Breake/Speed calc class
     */
    int nRPM = -1;
    bool bBrakeSigSend = false;
    bool bBrakeSig = false;
    int nBrakePressure = -1;
    int nAccPedalOpen = -1;
    int nSpeed = -1;
    CAvgCar pmCar(g_RPM_SAMPLE_SPACE_SIZE, g_SPEED_SAMPLE_SPACE_SIZE,
                  g_BREAKE_SAMPLE_SPACE_SIZE);
    pmCar.chgGear(CAvgGear::E_SHIFT_PARKING);
    /**
     * DIRECTION
     */
    double dir = (double)m_stVehicleInfo.nDirection;
    /**
     * SHIFT
     */
    int nShiftPosBK = -1;
    char shiftpos = 255;

    bool nextPointFlg = false;
    geoData next;
    double dx, dy, rad, theta;

    while (g_bStopFlag) {

        type = myJS->Read(&number, &value);

        m_sendMsgInfo.clear();

        gettimeofday(&now, NULL);
        if((timeout.tv_sec == now.tv_sec && timeout.tv_usec <= now.tv_sec)
            || (timeout.tv_sec < now.tv_sec)) {
            intervalctl = (intervalctl+1)%3;
            if (intervalctl) {
                fcycle = intervalctl;
            }
            usec = now.tv_usec + D_RUNLOOP_INTERVAL_COUNT*10*1000;
            timeout.tv_usec = usec % 1000000;
            if (usec < 1000000) {
                timeout.tv_sec = now.tv_sec;
            } else {
                timeout.tv_sec = now.tv_sec + 1;
            }
        }
//      ICO_DBG("intervalctl[%d], fcycle[%d]", intervalctl, fcycle);

        switch (type) {
        case JS_EVENT_AXIS:
            if (number == myConf.m_nSteering) {
                last_steering = value;
                if (value != 0) {
                    m_stVehicleInfo.nSteeringAngle +=
                        (value * 10 / 65536 - m_stVehicleInfo.nSteeringAngle);
                }
            }
            else if (number == myConf.m_nAccel) {
                if (0 >= value) {
                    pmCar.chgThrottle(32767);
                }
                else {
                    pmCar.chgThrottle((value - 16384) * -2);
                }
            }
            else if (number == myConf.m_nBrake) {
                if (0 >= value) {
                    pmCar.chgBrake(32767);
                }
                else {
                    pmCar.chgBrake((value - 16384) * -2);
                }
            }
            break;
        case JS_EVENT_BUTTON:
            if (myConf.m_sDeviceName == D_DEV_NAME_G25) {
                /**
                 * Gear Change SHIFT UP
                 */
                if (number == myConf.m_nShiftU) {
                    //printf("Shift Up[%d]\n",value);
                    if (value != 0) {
                        pmCar.setShiftUp();
                        m_stVehicleInfo.nShiftPos = pmCar.getSelectGear();
                        shiftpos = pmCar.getValue();
                    }
                }
                /**
                 * Gear Change SHIFT DOWN
                 */
                if (number == myConf.m_nShiftD) {
                    //printf("Shift Down[%d]\n",value);
                    if (value != 0) {
                        pmCar.setShiftDown();
                        m_stVehicleInfo.nShiftPos = pmCar.getSelectGear();
                        shiftpos = pmCar.getValue();
                    }
                }
            }

            else if (myConf.m_sDeviceName == D_DEV_NAME_G27) {
                if (number == myConf.m_nShift1) {
                    //printf("Shift 1[%d]\n",value);
                    if (value != 0) {
                        pmCar.setShiftMT(CAvgGear::E_SHIFT_MT_FIRST);
                        m_stVehicleInfo.nShiftPos = pmCar.getSelectGear();
                        shiftpos = pmCar.getValue();
                    }
                    else {
                        pmCar.setShiftMT(CAvgGear::E_SHIFT_NEUTRAL);
                        m_stVehicleInfo.nShiftPos = pmCar.getSelectGear();
                        shiftpos = pmCar.getValue();
                    }
                }

                if (number == myConf.m_nShift2) {
                    //printf("Shift 2[%d]\n",value);
                    if (value != 0) {
                        pmCar.setShiftMT(CAvgGear::E_SHIFT_MT_SECOND);
                        m_stVehicleInfo.nShiftPos = pmCar.getSelectGear();
                        shiftpos = pmCar.getValue();
                    }
                    else {
                        pmCar.setShiftMT(CAvgGear::E_SHIFT_NEUTRAL);
                        m_stVehicleInfo.nShiftPos = pmCar.getSelectGear();
                        shiftpos = pmCar.getValue();
                    }
                }

                if (number == myConf.m_nShift3) {
                    //printf("Shift 3[%d]\n",value);
                    if (value != 0) {
                        pmCar.setShiftMT(CAvgGear::E_SHIFT_MT_THIRD);
                        m_stVehicleInfo.nShiftPos = pmCar.getSelectGear();
                        shiftpos = pmCar.getValue();
                    }
                    else {
                        pmCar.setShiftMT(CAvgGear::E_SHIFT_NEUTRAL);
                        m_stVehicleInfo.nShiftPos = pmCar.getSelectGear();
                        shiftpos = pmCar.getValue();
                    }
                }

                if (number == myConf.m_nShift4) {
                    //printf("Shift 4[%d]\n",value);
                    if (value != 0) {
                        pmCar.setShiftMT(CAvgGear::E_SHIFT_MT_FOURTH);
                        m_stVehicleInfo.nShiftPos = pmCar.getSelectGear();
                        shiftpos = pmCar.getValue();
                    }
                    else {
                        pmCar.setShiftMT(CAvgGear::E_SHIFT_NEUTRAL);
                        m_stVehicleInfo.nShiftPos = pmCar.getSelectGear();
                        shiftpos = pmCar.getValue();
                    }
                }

                if (number == myConf.m_nShift5) {
                    //printf("Shift 5[%d]\n",value);
                    if (value != 0) {
                        pmCar.setShiftMT(CAvgGear::E_SHIFT_MT_FIFTH);
                        m_stVehicleInfo.nShiftPos = pmCar.getSelectGear();
                        shiftpos = pmCar.getValue();
                    }
                    else {
                        pmCar.setShiftMT(CAvgGear::E_SHIFT_NEUTRAL);
                        m_stVehicleInfo.nShiftPos = pmCar.getSelectGear();
                        shiftpos = pmCar.getValue();
                    }
                }

                if (number == myConf.m_nShift6) {
                    //printf("Shift 6[%d]\n",value);
                    if (value != 0) {
                        pmCar.setShiftMT(CAvgGear::E_SHIFT_MT_SIXTH);
                        m_stVehicleInfo.nShiftPos = pmCar.getSelectGear();
                        shiftpos = pmCar.getValue();
                    }
                    else {
                        pmCar.setShiftMT(CAvgGear::E_SHIFT_NEUTRAL);
                        m_stVehicleInfo.nShiftPos = pmCar.getSelectGear();
                        shiftpos = pmCar.getValue();
                    }
                }

                if (number == myConf.m_nShiftR) {
                    //printf("Shift R[%d]\n",value);
                    if (value != 0) {
                        pmCar.setShiftMT(CAvgGear::E_SHIFT_REVERSE);
                        m_stVehicleInfo.nShiftPos = pmCar.getSelectGear();
                        shiftpos = pmCar.getValue();
                    }
                    else {
                        pmCar.setShiftMT(CAvgGear::E_SHIFT_NEUTRAL);
                        m_stVehicleInfo.nShiftPos = pmCar.getSelectGear();
                        shiftpos = pmCar.getValue();
                    }
                }
            }
            /**
             * TURN SIGNAL(WINKER) & LIGHTSTATUS
             */
            bool bLIGHTSTATUS = false;
            if (number == myConf.m_nWinkR) {
                if (value != 0) {

                    m_stVehicleInfo.bWinkR = !m_stVehicleInfo.bWinkR;
                    m_stVehicleInfo.bWinkL = false;
                    if (m_stVehicleInfo.bWinkR)
                        m_stVehicleInfo.nWinkerPos = WINKER_RIGHT;
                    else
                        m_stVehicleInfo.nWinkerPos = WINKER_OFF;

                    const char *vi = "TURN_SIGNAL";
                    int wpos =
                        m_stVehicleInfo.nWinkerPos == WINKER_RIGHT ? 1 : 0;
                    SendVehicleInfo(dataport_def, vi, wpos);
                    bLIGHTSTATUS = true;
                }
            }

            if (number == myConf.m_nWinkL) {
                if (value != 0) {
                    m_stVehicleInfo.bWinkL = !m_stVehicleInfo.bWinkL;
                    m_stVehicleInfo.bWinkR = false;
                    if (m_stVehicleInfo.bWinkL)
                        m_stVehicleInfo.nWinkerPos = WINKER_LEFT;
                    else
                        m_stVehicleInfo.nWinkerPos = WINKER_OFF;

                    const char *vi = "TURN_SIGNAL";
                    int wpos =
                        m_stVehicleInfo.nWinkerPos == WINKER_LEFT ? 2 : 0;
                    SendVehicleInfo(dataport_def, vi, wpos);
                    bLIGHTSTATUS = true;
                }
            }

            if (number == myConf.m_nHeadLight) {
                if (0 != value) {
                    if (false == m_stVehicleInfo.bHeadLight) { // HEAD LIGHT OFF(false) ?
                        m_stVehicleInfo.bHeadLight = true; // HEAD LIGHT ON(true)
                    }
                    else {
                        m_stVehicleInfo.bHeadLight = false; // HEAD LIGHT OFF(false)
                    }
                    bLIGHTSTATUS = true;
                }
            }

            if (true == bLIGHTSTATUS) {
                const size_t LSsz = 8;
                char data[LSsz];
                    // 0:LIGHT HEAD STATUS ON(1)/OFF(0)
                    // 1:LEFT WINKER STATUS ON(1)/OFF(0)
                    // 2:RIGHT WINKER STATUS ON(1)/OFF(0)
                    // 3:PARKING
                    // 4:FOG LAMP
                    // 5:HAZARD
                    // 6:BRAKE
                    // 7:HIGHBEAM
                memset(data, 0, sizeof(data));
                if (true == m_stVehicleInfo.bHeadLight) { // HEAD LIGHT ON ?
                    data[0] = 1;
                }
                if (WINKER_LEFT == m_stVehicleInfo.nWinkerPos) {
                    data[1] = 1;
                }
                else if (WINKER_RIGHT == m_stVehicleInfo.nWinkerPos) {
                    data[2] = 1;
                }
                SendVehicleInfo(dataport_def, sLIGHTSTATUS, &data[0], LSsz);
            }

            break;
        }
        pmCar.updateAvg();
        /* RealTime */
        /**
         * BRAKE SIGNAL
         */
        bool bBrakeNEW = pmCar.isOnBrake();
        m_stVehicleInfo.bBrake = bBrakeNEW;
        if ((false == bBrakeSigSend) || (bBrakeNEW != bBrakeSig)) {
            bBrakeSigSend = true;
            SendVehicleInfo(dataport_def, sBRAKE_SIGNAL, bBrakeNEW);
            bBrakeSig = bBrakeNEW; // update value
        }
        /**
         * SHIFT
         */
        if (nShiftPosBK != m_stVehicleInfo.nShiftPos) {
            const size_t ShiftSz = 3;
            int data[ShiftSz];
            int val = pmCar.getValue();
            data[0] = pmCar.getSelectGear();
            if (data[0] > 10) {
                data[0] = data[0] - 10 + 4;
                if (data[0] >= 8) {
                    data[0] = 4;
                }
            }
            data[1] = pmCar.getValue();
            data[2] = pmCar.getMode();
            SendVehicleInfo(dataport_def, sSHIFT, &data[0], ShiftSz);
            nShiftPosBK = m_stVehicleInfo.nShiftPos;
//          ICO_DBG("SHIFT: Send [%d, %d, %d]", data[0], data[1], data[2]);
        }

        /* 100ms cycle */
        if (fcycle == 2) {
            int angle = 0;
            if (last_steering < 0) {
                angle = (((double)last_steering / 32768) * 450) + 450 + 270;
                angle %= 360;
                //std:: cout << "last_steering * (450 / 32768) = " << ((double)last_steering / 32768) * 450 << "\n";
                //std:: cout << "last_steering * (450 / 32768) + 450 = " << (double)last_steering * (450 / 32768) + 450 << "\n";
                //std:: cout << "last_steering * (450 / 32768) + 450 + 270 = " << (double)last_steering * (450 / 32768) + 450 + 270 << "\n";
            }
            else {
                angle = ((double)last_steering / 32768) * 450;
                angle %= 360;
                //std:: cout << "last_steering * (450 / 32768) = " << ((double)last_steering / 32768) * 450 << "\n";
            }
            //std::cout << "Steering last_steering = " << last_steering << " -> angle = " << angle << std::endl;
            if (angle != last_angle) {
                const char *vi = "STEERING";

                /*
                SendVehicleInfo(dataport_def,
                                vi, m_stVehicleInfo.nSteeringAngle);
                */
                SendVehicleInfo(dataport_def, vi, angle);
                last_angle = angle;
            }
            /**
             * RPM check
             */
            int rpmNEW = (int)pmCar.getRPM();
            if (rpmNEW != nRPM) {
                SendVehicleInfo(dataport_def, sENGINE_SPEED, rpmNEW);
                nRPM = rpmNEW; // update value
            }
            int apoNEW = pmCar.calcAccPedalOpen();
            if (apoNEW != nAccPedalOpen) {
                SendVehicleInfo(dataport_def, sACCPEDAL_OPEN, apoNEW);
                nAccPedalOpen = apoNEW;
            }
        } /* if (fcycle == 2) */

        /* 50ms cycle */
        if (fcycle != 0) {
            /**
             * BRAKE PRESSURE
             */
            int pressureNEW = pmCar.calcPressure(pmCar.getBrakeAvg());
            m_stVehicleInfo.nBrakeHydraulicPressure = pressureNEW;
            if (pressureNEW != nBrakePressure) {
                SendVehicleInfo(dataport_def, sBRAKE_PRESSURE, pressureNEW);
                nBrakePressure = pressureNEW;
            }

            /**
             * VELOCITY (SPEED)
             */
            int speedNew = (int)pmCar.getSpeed();
            if (speedNew > 180) {
                speedNew = 180;
            }
            if (speedNew != nSpeed) {
                SendVehicleInfo(dataport_def, sVELOCITY, speedNew);
                m_stVehicleInfo.nVelocity = speedNew;
                nSpeed = speedNew;
            }

            if (m_bDemoRunning) {
                next = routeList.front();
                dy = next.lat - m_stVehicleInfo.fLat;
                dx = next.lng - m_stVehicleInfo.fLng;
                theta = (double) atan2(dy, dx) * 180.0 / (atan(1.0) * 4.0);
                if (theta < 0) {
                    theta = 360.0 + theta;
                }

                rad = (theta / 180.0) * PIE;

                m_stVehicleInfo.nDirection = 90 - theta;
                if (m_stVehicleInfo.nDirection < 0) {
                    m_stVehicleInfo.nDirection = 360 + m_stVehicleInfo.nDirection;
                }
                SendVehicleInfo(dataport_def, sDIRECTION, m_stVehicleInfo.nDirection);

                dir = static_cast<double>(m_stVehicleInfo.nDirection);
                double runMeters = pmCar.getTripmeter();
                pmCar.tripmeterReset();
                double tmpLat = m_stVehicleInfo.fLat;
                double tmpLng = m_stVehicleInfo.fLng;
                POINT pNEW = CalcDest(tmpLat, tmpLng, dir, runMeters);
                double tmpLct[] = { pNEW.lat, pNEW.lng, 0 };
                SendVehicleInfo(dataport_def, sLOCATION, &tmpLct[0], 3);
                m_stVehicleInfo.fLat = pNEW.lat;
                m_stVehicleInfo.fLng = pNEW.lng;

                if (rad == 0) {
                    if (m_stVehicleInfo.fLng >= next.lng) {
                        nextPointFlg = true;
                    }
                }
                else if (rad > 0 && rad < 0.5 * PIE) {
                    if (m_stVehicleInfo.fLng >= next.lng
                        && m_stVehicleInfo.fLat >= next.lat) {
                        nextPointFlg = true;
                    }
                }
                else if (rad == 0.5 * PIE) {
                    if (m_stVehicleInfo.fLat >= next.lat) {
                        nextPointFlg = true;
                    }
                }
                else if (rad > 0.5 * PIE && rad < PIE) {
                    if (m_stVehicleInfo.fLng <= next.lng
                        && m_stVehicleInfo.fLat >= next.lat) {
                        nextPointFlg = true;
                    }
                }
                else if (rad == PIE) {
                    if (m_stVehicleInfo.fLng <= next.lng) {
                        nextPointFlg = true;
                    }
                }
                else if (rad > PIE && rad < 1.5 * PIE) {
                    if (m_stVehicleInfo.fLng <= next.lng
                        && m_stVehicleInfo.fLat <= next.lat) {
                        nextPointFlg = true;
                    }
                }
                else if (rad == 1.5 * PIE) {
                    if (m_stVehicleInfo.fLat <= next.lat) {
                        nextPointFlg = true;
                    }
                }
                else {
                    if (m_stVehicleInfo.fLng >= next.lng
                        && m_stVehicleInfo.fLat <= next.lat) {
                        nextPointFlg = true;
                    }
                }

                if (nextPointFlg) {
                    std::cout << "routeList.size() = " << routeList.size() << std::endl;
                    routeList.pop();
                    if (routeList.empty()) {
                        LoadRouteList();
                        m_stVehicleInfo.fLng = myConf.m_fLng;
                        m_stVehicleInfo.fLat = myConf.m_fLat;
                    }
                    next = routeList.front();
                    nextPointFlg = false;
                }
            }
            else {
                /**
                  * DIRECTION (AZIMUTH)
                  * Front wheel steering angle
                  */
                double runMeters = pmCar.getTripmeter();
                pmCar.tripmeterReset();
                if (0 != runMeters) {
                    double stear = (double)m_stVehicleInfo.nSteeringAngle;
                    double dirNEW = CalcAzimuth(dir, stear, runMeters);
                    int nDir = (int)dirNEW;
                    dir = dirNEW;
                    if (nDir != m_stVehicleInfo.nDirection) {
                        SendVehicleInfo(dataport_def, sDIRECTION, nDir);
                        m_stVehicleInfo.nDirection = nDir;
                    }
                }
                /**
                  * LOCATION
                  */
                if ((!m_bUseGps) && (0 != runMeters)) {
                    double tmpLat = m_stVehicleInfo.fLat;
                    double tmpLng = m_stVehicleInfo.fLng;
                    POINT pNEW = CalcDest(tmpLat, tmpLng, dir, runMeters);
                    if ((tmpLat != pNEW.lat) || (tmpLng != pNEW.lng)){
                        double tmpLct[] = { pNEW.lat, pNEW.lng, 0 };
                        SendVehicleInfo(dataport_def, sLOCATION, &tmpLct[0], 3);
                        m_stVehicleInfo.fLat = pNEW.lat;
                        m_stVehicleInfo.fLng = pNEW.lng;
                    }
                }
            } 
        }

        fcycle = 0;
    }
}

void CGtCtrl::Run2()
{
    msgQueue = "";
    g_bStopFlag = true;
    geoData next;
    double theta = 0;
    double rad = 0;
    double dx, dy;
    bool nextPointFlg = true;
    bool routeDriveFlg = true;
    std::string tmpstr = "";

    int type = -1;
    int number = -1;
    int value = -1;

    pthread_t thread[2];

    pthread_mutex_init(&mutex, NULL);

    while (g_bStopFlag) {
        type = myJS->Read(&number, &value);

        pthread_mutex_lock(&mutex);
        if (msgQueue.size() > 0) {
            char buf[32];
            int pos = 0;
            int lastidx = 0;
            geoData tmpGeo;
            int i = 0;

            tmpGeo.lat = GEORESET;
            tmpGeo.lng = GEORESET;

            for (i = 0; i < (int) msgQueue.size(); i++) {
                if (msgQueue[i] == ',') {
                    tmpGeo.lat = atof(buf);
                    pos = 0;
                }
                else if (msgQueue[i] == '\n') {
                    tmpGeo.lng = atof(buf);
                    pos = 0;

                    routeList.push(tmpGeo);
                    lastidx = i;
                    tmpGeo.lat = GEORESET;
                    tmpGeo.lng = GEORESET;
                }
                else {
                    buf[pos++] = msgQueue[i];
                }
            }
            for (int idx = 0; msgQueue[lastidx] != '\0'; idx++, lastidx++) {
                tmpstr[idx] = msgQueue[lastidx];
            }
            msgQueue = tmpstr;

        }
        pthread_mutex_unlock(&mutex);

        switch (type) {
        case JS_EVENT_AXIS:
            if (number == myConf.m_nSteering) {
                if (value != 0)
                    m_stVehicleInfo.nSteeringAngle +=
                        (value * 10 / 65536 - m_stVehicleInfo.nSteeringAngle);

            }

            if (number == myConf.m_nAccel) {
                if (value < 0) {
                    m_stVehicleInfo.bBrake = false;
                    m_stVehicleInfo.nBrakeHydraulicPressure = 0;

                }
                else if (0 < value) {
                    m_stVehicleInfo.bBrake = true;
                    m_stVehicleInfo.nBrakeHydraulicPressure =
                        (value * 100) / MAX_SPEED;
                }
                else {
                    m_stVehicleInfo.bBrake = false;
                    m_stVehicleInfo.nBrakeHydraulicPressure = 0;
                }
                m_stVehicleInfo.nAccel = value;

                const char *vi1 = "BRAKE_SIGNAL";
                const char *vi2 = "BRAKE_PRESSURE";
                SendVehicleInfo(dataport_def, vi1, m_stVehicleInfo.bBrake);

                SendVehicleInfo(dataport_def,
                                vi2, m_stVehicleInfo.nBrakeHydraulicPressure);
            }
            break;
        case JS_EVENT_BUTTON:
            if (number == myConf.m_nShiftU) {
                if (value != 0) {
                    char shiftpos = 255;
                    if (m_stVehicleInfo.nShiftPos > PARKING) {
                        switch (m_stVehicleInfo.nShiftPos) {
                        case FIRST:
                            m_stVehicleInfo.nShiftPos = SECOND;
                            shiftpos = 2;
                            break;
                        case SECOND:
                            m_stVehicleInfo.nShiftPos = THIRD;
                            shiftpos = 3;
                            break;
                        case THIRD:
                            m_stVehicleInfo.nShiftPos = DRIVE;
                            shiftpos = 4;
                            break;
                        case DRIVE:
                            m_stVehicleInfo.nShiftPos = NEUTRAL;
                            shiftpos = 0;
                            break;
                        case NEUTRAL:
                            m_stVehicleInfo.nShiftPos = REVERSE;
                            shiftpos = 128;
                            break;
                        case REVERSE:
                            m_stVehicleInfo.nShiftPos = PARKING;
                            shiftpos = 255;
                            break;
                        }
                    }

                    char data[] = { shiftpos, shiftpos, 0 };
                    const char *vi = "SHIFT";
                    SendVehicleInfo(dataport_def, vi, &data[0]);

                }
            }

            if (number == myConf.m_nShiftD) {
                if (value != 0) {
                    char shiftpos = 1;
                    if (m_stVehicleInfo.nShiftPos < SPORTS) {
                        switch (m_stVehicleInfo.nShiftPos) {
                        case SHIFT_UNKNOWN:
                            m_stVehicleInfo.nShiftPos = PARKING;
                            shiftpos = 255;
                            break;
                        case PARKING:
                            m_stVehicleInfo.nShiftPos = REVERSE;
                            shiftpos = 128;
                            break;
                        case REVERSE:
                            m_stVehicleInfo.nShiftPos = NEUTRAL;
                            shiftpos = 0;
                            break;
                        case NEUTRAL:
                            m_stVehicleInfo.nShiftPos = DRIVE;
                            shiftpos = 4;
                            break;
                        case DRIVE:
                            m_stVehicleInfo.nShiftPos = THIRD;
                            shiftpos = 3;
                            break;
                        case THIRD:
                            m_stVehicleInfo.nShiftPos = SECOND;
                            shiftpos = 2;
                            break;
                        case SECOND:
                            m_stVehicleInfo.nShiftPos = FIRST;
                            shiftpos = 1;
                            break;
                        }
                    }

                    char data[] = { shiftpos, shiftpos, 0 };
                    const char *vi = "SHIFT";
                    SendVehicleInfo(dataport_def, vi, &data[0], 3);
                }
            }

            if (number == myConf.m_nWinkR) {
                if (value != 0) {
                    m_stVehicleInfo.bWinkR = !m_stVehicleInfo.bWinkR;
                    if (m_stVehicleInfo.bWinkR)
                        m_stVehicleInfo.nWinkerPos = WINKER_RIGHT;
                    else
                        m_stVehicleInfo.nWinkerPos = WINKER_OFF;

                    const char *vi = "TURN_SIGNAL";
                    int wpos =
                        m_stVehicleInfo.nWinkerPos == WINKER_RIGHT ? 1 : 0;
                    SendVehicleInfo(dataport_def, vi, wpos);
                }
            }

            if (number == myConf.m_nWinkL) {
                if (value != 0) {
                    m_stVehicleInfo.bWinkL = !m_stVehicleInfo.bWinkL;
                    if (m_stVehicleInfo.bWinkL)
                        m_stVehicleInfo.nWinkerPos = WINKER_LEFT;
                    else
                        m_stVehicleInfo.nWinkerPos = WINKER_OFF;

                    const char *vi = "TURN_SIGNAL";
                    int wpos =
                        m_stVehicleInfo.nWinkerPos == WINKER_LEFT ? 2 : 0;
                    SendVehicleInfo(dataport_def, vi, wpos);
                }
            }

            break;
        }

        if (m_stVehicleInfo.nAccel < 0) {
            m_stVehicleInfo.dVelocity +=
                ((abs(m_stVehicleInfo.nAccel) / 10000.0) -
                 (m_stVehicleInfo.dVelocity / 100.0));
        }
        else if (0 < m_stVehicleInfo.nAccel) {
            m_stVehicleInfo.dVelocity -=
                ((abs(m_stVehicleInfo.nAccel) / 10000.0) +
                 (m_stVehicleInfo.dVelocity / 100.0));
        }
        else {
            m_stVehicleInfo.dVelocity -= (m_stVehicleInfo.dVelocity / 1000);
        }

        if (m_stVehicleInfo.dVelocity > MAX_SPEED)
            m_stVehicleInfo.dVelocity = MAX_SPEED;
        if (m_stVehicleInfo.dVelocity < 0)
            m_stVehicleInfo.dVelocity = 0.0;

        if (m_stVehicleInfo.nVelocity != (int) m_stVehicleInfo.dVelocity) {
            m_stVehicleInfo.nVelocity = (int) m_stVehicleInfo.dVelocity;
            const char *vi = "VELOCITY";
            SendVehicleInfo(dataport_def, vi, m_stVehicleInfo.nVelocity);
        }


        if (routeList.empty()) {
            if (routeDriveFlg) {
                printf("FreeDriving\n");
            }
            routeDriveFlg = false;

            m_stVehicleInfo.nDirection += m_stVehicleInfo.nSteeringAngle;

            while (m_stVehicleInfo.nDirection > 359)
                m_stVehicleInfo.nDirection -= 360;
            while (m_stVehicleInfo.nDirection < 0)
                m_stVehicleInfo.nDirection += 360;

            {
                const char *vi = "DIRECTION";
                SendVehicleInfo(dataport_def, vi, m_stVehicleInfo.nDirection);
            }


            if (!m_bUseGps) {
                double rad = 0.0;
                if (m_stVehicleInfo.nDirection != 0)
                    rad =
                        (double) m_stVehicleInfo.nDirection / 180.0 *
                        3.14159265;

                double dx = (double) m_stVehicleInfo.nVelocity * sin(rad);
                double dy = (double) m_stVehicleInfo.nVelocity * cos(rad);

                m_stVehicleInfo.fLat += dx * 0.000003;
                m_stVehicleInfo.fLng += dy * 0.000003;

                const char *vi = "LOCATION";
                double location[] =
                    { m_stVehicleInfo.fLat, m_stVehicleInfo.fLng, 0 };
            }
        }
        else {
            if (!routeDriveFlg) {
                printf("route Driving\n");
                m_stVehicleInfo.fLat = routeList.front().lat;
                m_stVehicleInfo.fLng = routeList.front().lng;
                routeList.pop();
            }
            routeDriveFlg = true;
            next = routeList.front();
            dy = next.lat - m_stVehicleInfo.fLat;
            dx = next.lng - m_stVehicleInfo.fLng;
            theta = (double) atan2(dy, dx) * 180.0 / (atan(1.0) * 4.0);
            if (theta < 0) {
                theta = 360.0 + theta;
            }

            rad = (theta / 180.0) * PIE;

            m_stVehicleInfo.nDirection = 90 - theta;
            if (m_stVehicleInfo.nDirection < 0) {
                m_stVehicleInfo.nDirection = 360 + m_stVehicleInfo.nDirection;
            }

            {
                const char *vi = "DIRECTION";
                SendVehicleInfo(dataport_def, vi, m_stVehicleInfo.nDirection);
            }
            m_stVehicleInfo.fLat +=
                (m_stVehicleInfo.nVelocity * sin(rad) / (72 * 1000)) /
                (cos((m_stVehicleInfo.fLat / 180) * PIE) * 111.111);
            m_stVehicleInfo.fLng +=
                (m_stVehicleInfo.nVelocity * cos(rad) / (72 * 1000)) * (1 /
                                                                        111.111);

            {
                const char *vi = "LOCATION";
                double location[] =
                    { m_stVehicleInfo.fLat, m_stVehicleInfo.fLng, 0 };
            }

            if (rad == 0) {
                if (m_stVehicleInfo.fLng >= next.lng) {
                    nextPointFlg = true;
                }
            }
            else if (rad > 0 && rad < 0.5 * PIE) {
                if (m_stVehicleInfo.fLng >= next.lng
                    && m_stVehicleInfo.fLat >= next.lat) {
                    nextPointFlg = true;
                }
            }
            else if (rad == 0.5 * PIE) {
                if (m_stVehicleInfo.fLat >= next.lat) {
                    nextPointFlg = true;
                }
            }
            else if (rad > 0.5 * PIE && rad < PIE) {
                if (m_stVehicleInfo.fLng <= next.lng
                    && m_stVehicleInfo.fLat >= next.lat) {
                    nextPointFlg = true;
                }
            }
            else if (rad == PIE) {
                if (m_stVehicleInfo.fLng <= next.lng) {
                    nextPointFlg = true;
                }
            }
            else if (rad > PIE && rad < 1.5 * PIE) {
                if (m_stVehicleInfo.fLng <= next.lng
                    && m_stVehicleInfo.fLat <= next.lat) {
                    nextPointFlg = true;
                }
            }
            else if (rad == 1.5 * PIE) {
                if (m_stVehicleInfo.fLat <= next.lat) {
                    nextPointFlg = true;
                }
            }
            else {
                if (m_stVehicleInfo.fLng >= next.lng
                    && m_stVehicleInfo.fLat <= next.lat) {
                    nextPointFlg = true;
                }
            }

            if (nextPointFlg) {
                if (routeList.empty()) {
                    LoadRouteList();
                    m_stVehicleInfo.fLng = myConf.m_fLng;
                    m_stVehicleInfo.fLat = myConf.m_fLat;
                }
                next = routeList.front();
                routeList.pop();
                nextPointFlg = false;
            }
        }
    }

    pthread_join(thread[0], NULL);
    pthread_join(thread[1], NULL);
    while (!routeList.empty()) {
        routeList.pop();
    }
    pthread_mutex_destroy(&mutex);

}

void *Comm(void *s)
{
    struct KeyDataMsg_t ret;
    int *recvid = (int *) s;

    while (g_bStopFlag) {
        if (msgrcv(*recvid, &ret, sizeof(struct KeyDataMsg_t), 31, MSG_EXCEPT)
            == -1) {
            return (void *) NULL;
        }
    }
    return 0;
}


/*--------------------------------------------------------------------------*/
/**
 * @brief   function of vehicle information access
 *
 * @param[in]   SigNo       signal number
 * @return      none
 */
/*--------------------------------------------------------------------------*/
void DaemonTerminate(int SigNo)
{

    if ((SigNo > 0) &&
        (SigNo != SIGHUP) && (SigNo != SIGINT) && (SigNo != SIGTERM)) {
        (void) signal(SigNo, DaemonTerminate);
        return;
    }

    exit(0);
}

/*--------------------------------------------------------------------------*/
/**
 * @brief   function of vehicle information set to AMB
 *
 * @param[in]   sendque_id  que id for sending
 * @param[in]   recvque_id  que id for receiving
 * @param[in]   mtype       priority
 * @param[in]   key         name of vehicle information
 * @param[in]   comstat     common_status
 * @param[in]   data        value of vehicle information
 * @param[in]   len         length of vehicle information
 * @param[out]  ret         struct from AMB
 * @return  bool    true:success,false:fail
 */
/*--------------------------------------------------------------------------*/
bool CGtCtrl::SendVehicleInfo( /*int & send_id, long mtype, */
                              ProtocolType type, const char *key, bool data)
{
    return sendVehicleInfo( /*send_id, mtype, */ type, key, (void *) &data,
                           sizeof(bool), 1);
}

bool CGtCtrl::SendVehicleInfo( /*int & send_id, long mtype, */
                              ProtocolType type, const char *key, int data)
{
    return sendVehicleInfo( /*send_id, mtype, */ type, key, (void *) &data,
                           sizeof(int), 1);
}

bool CGtCtrl::SendVehicleInfo( /*int & send_id, long mtype, */
                              ProtocolType type, const char *key, int data[],
                              int len)
{
    return sendVehicleInfo( /*send_id, mtype, */ type, key, (void *) data,
                           sizeof(int), len);
}

bool CGtCtrl::SendVehicleInfo(/*int & send_id, long mtype */
                              ProtocolType type, const char *key,
                              double data[], int len)
{
    return sendVehicleInfo( /*send_id, mtype, */ type, key, (void *) data,
                           sizeof(double), len);
}

bool CGtCtrl::SendVehicleInfo( /*int & send_id, long mtype, */
                              ProtocolType type, const char *key, char data[],
                              int len)
{
    return sendVehicleInfo( /*send_id, mtype, */ type, key, (void *) data,
                           sizeof(char), len);
}

/*--------------------------------------------------------------------------*/
/**
 * @brief   vehicle information struct for AMB
 *
 * @param[in]   buf     buffer for vehicle information struct
 * @param[in]   bufsize size of buffer
 * @param[in]   mtype   priority
 * @param[in]   key     name of vehicle information
 * @param[in]   tm      time
 * @param[in]   nstat   common_status
 * @param[in]   status  value of vehicle information
 * @return  none
 */
/*--------------------------------------------------------------------------*/
void CGtCtrl::SetMQKeyData(char *buf, unsigned int bufsize, long &mtype,
                           const char *key, char status[], unsigned int size)
{
    KeyDataMsg_t *tmp_t = (KeyDataMsg_t *) buf;

    memset(buf, 0x00, bufsize);
    strcpy(tmp_t->KeyEventType, key);
    gettimeofday(&tmp_t->recordtime, NULL);
    tmp_t->data.common_status = 0;
    memcpy(&tmp_t->data.status[0], &status[0], size);
}

/*--------------------------------------------------------------------------*/
/**
 * @brief   set vehicle information for AMB
 *
 * @param[in]   sendque_id  queue ID
 * @param[in]   priority    priority of queue
 * @param[in]   key         KeyEventType
 * @param[in]   data        value of vehicle info
 * @param[in]   unit_size   size of send data unit
 * @param[in]   unit_cnt    number of send data unit
 * @return  bool    true:success,false:failure
 */
/*--------------------------------------------------------------------------*/
bool CGtCtrl::sendVehicleInfo( /*int & send_id, long priority, */
                              ProtocolType type, const char *key, void *data,
                              unsigned int unit_size, int unit_cnt)
{
    long priority = 1;

    if (unit_size == 0 || unit_cnt <= 0 || data == NULL)
        return false;

    char adata[unit_size * unit_cnt];
    char mqMsg[sizeof(KeyDataMsg_t) + sizeof(adata)];

    for (int i = 0; i < unit_cnt; i++) {
        int pos = i * unit_size;
        memcpy(&adata[pos], (char *) data + pos, unit_size);
    }

    SetMQKeyData(&mqMsg[0], sizeof(mqMsg), priority, key, adata,
                 sizeof(adata));

    if (!m_ambpicomm_client[type].
        send(reinterpret_cast < char *>(mqMsg), sizeof(mqMsg)))
    {
        std::cerr << "Failed to send data(" << errno << ")." << std::endl;
        return false;
    }

    m_sendMsgInfo.push_back(string(key));

    return true;
}


/*--------------------------------------------------------------------------*/
/**
 * @brief   JOSN parser
 *
 * @param[in]   fname   file name of configuration file(full path)
 * @return  bool    false:failure
 */
/*--------------------------------------------------------------------------*/
bool CGtCtrl::LoadConfigJson(const char *fname)
{
    char confpath[1024];
    JsonParser *parser = NULL;
    JsonNode *node = NULL;
    JsonReader *reader = NULL;

    if (!LoadConfigAMBJson(fname, &confpath[0], sizeof(confpath))) {
        return false;
    }

    //g_type_init();
    printf("conf=%s\nvehicleinfo conf=%s\n", fname, confpath);

    parser = json_parser_new();
    GError *error = NULL;

    json_parser_load_from_file(parser, confpath, &error);
    if (error) {
        printf("Failed to load config: %s\n", error->message);
        DelJsonObj(parser, reader);
        return false;
    }

    node = json_parser_get_root(parser);
    if (node == NULL) {
        printf("Unable to get JSON root object.\n");
        DelJsonObj(parser, reader);
        return false;
    }

    reader = json_reader_new(node);
    if (reader == NULL) {
        printf("Unable to create JSON reader\n");
        DelJsonObj(parser, reader);
        return false;
    }

    if (!LoadConfigJsonCarSim(reader)) {
        DelJsonObj(parser, reader);
        return false;
    }

    json_reader_set_root(reader, node);
    if (!LoadConfigJsonCommon(reader)) {
        DelJsonObj(parser, reader);
        return false;
    }

    return true;
}

/*--------------------------------------------------------------------------*/
/**
 * @brief   release JSON parser object
 *
 * @param[in]   parser  parser object
 * @param[in]   node    node object
 * @param[in]   reader  reader object
 * @return  none
 */
/*--------------------------------------------------------------------------*/
void CGtCtrl::DelJsonObj(JsonParser *parser, JsonReader *reader)
{
    if (reader != NULL)
        g_object_unref(reader);
    if (parser != NULL)
        g_object_unref(parser);
}

/*--------------------------------------------------------------------------*/
/**
 * @brief   get JSON file path form configuration
 *
 * @param[in]   fname       full path of configuration file
 * @param[out]  jsonfname   buffer of JSON file name
 * @param[in]   size        size of second argument buffer size
 * @return  bool        false:failure
 */
/*--------------------------------------------------------------------------*/
bool CGtCtrl::LoadConfigAMBJson(const char *fname, char *jsonfname, int size)
{
    JsonParser *ps = NULL;
    JsonNode *nd = NULL;
    JsonReader *rd = NULL;

    memset(jsonfname, 0x00, size);
    //g_type_init();

    ps = json_parser_new();
    GError *error = NULL;

    json_parser_load_from_file(ps, fname, &error);
    if (error) {
        printf("Failed to load AMBconfig: %s\n", error->message);
        DelJsonObj(ps, rd);
        return false;
    }

    nd = json_parser_get_root(ps);
    if (nd == NULL) {
        printf("Unable to get AMB-JSON root object.\n");
        DelJsonObj(ps, rd);
        return false;
    }

    rd = json_reader_new(nd);
    if (rd == NULL) {
        printf("Unable to create AMB-JSON reader\n");
        DelJsonObj(ps, rd);
        return false;
    }

    json_reader_read_member(rd, "sources");
    error = (GError *) json_reader_get_error(rd);
    if (error != NULL) {
        printf("Error getting AMB sources member. %s\n", error->message);
        DelJsonObj(ps, rd);
        return false;
    }
    g_assert(json_reader_is_array(rd));
    for (int i = 0; i < json_reader_count_elements(rd); i++) {
        json_reader_read_element(rd, i);
        json_reader_read_member(rd, "name");
        std::string section = json_reader_get_string_value(rd);
        json_reader_end_member(rd);
        if ("VehicleSource" == section) {
            char val[size];
            if (GetConfigValue(rd, "configfile", &val[0], size)) {
                strcpy(jsonfname, val);
            }
            break;
        }
        json_reader_end_element(rd);
    }
    json_reader_end_member(rd);
    DelJsonObj(ps, rd);
    return true;
}

/*--------------------------------------------------------------------------*/
/**
 * @brief   get JSON value(CarSim)
 *
 * @param[in]   reader  JSON reader object
 * @return  bool    false:failure
 */
/*--------------------------------------------------------------------------*/
bool CGtCtrl::LoadConfigJsonCarSim(JsonReader *reader)
{
    json_reader_read_member(reader, "Config");
    const GError *confreaderror = json_reader_get_error(reader);
    if (confreaderror != NULL) {
        printf("Error getting sources member. %s\n", confreaderror->message);
        return false;
    }
    g_assert(json_reader_is_array(reader));

    int elementnum = json_reader_count_elements(reader);
    for (int i = 0; i < elementnum; i++) {
        json_reader_read_element(reader, i);
        json_reader_read_member(reader, "Section");
        std::string section = json_reader_get_string_value(reader);
        json_reader_end_member(reader);

        if ("CarSim" == section) {
            const char *key1 = "DefaultInfoMQKey";
            const char *key2 = "CustomizeInfoMQKey";
            const char *key3 = "VehicleInfoList";
            char val[9];

            if (!json_reader_read_member(reader, key3)) {
                confreaderror = json_reader_get_error(reader);
                printf("Error getting %s. %s\n", key3,
                       confreaderror->message);
                return false;
            }
            else {
                confreaderror = json_reader_get_error(reader);
                if (confreaderror != NULL) {
                    confreaderror = json_reader_get_error(reader);
                    printf("Error getting %s member. %s\n", key3,
                           confreaderror->message);
                    return false;
                }
                g_assert(json_reader_is_array(reader));
                m_viList.length = json_reader_count_elements(reader);
                for (int j = 0; j < m_viList.length; j++) {
                    json_reader_read_element(reader, j);
                    std::string str = json_reader_get_string_value(reader);
                    strcpy(m_viList.name[j], str.c_str());
                    json_reader_end_element(reader);
                }
                json_reader_end_member(reader);
            }
        }
        json_reader_end_element(reader);
    }
    json_reader_end_member(reader);

    return true;
}

/*--------------------------------------------------------------------------*/
/**
 * @brief   get JSON value(Common)
 *
 * @param[in]   reader  JSON reader object
 * @return  bool    false:failure
 */
/*--------------------------------------------------------------------------*/
bool CGtCtrl::LoadConfigJsonCommon(JsonReader *reader)
{
    json_reader_read_member(reader, "Config");
    const GError *confreaderror = json_reader_get_error(reader);
    if (confreaderror != NULL) {
        printf("Error getting sources member. %s\n", confreaderror->message);
        return false;
    }
    g_assert(json_reader_is_array(reader));

    int elementnum = json_reader_count_elements(reader);
    int i;
    for (i = 0; i < elementnum; i++) {
        json_reader_read_element(reader, i);
        if (!json_reader_read_member(reader, "Section")) {
            confreaderror = json_reader_get_error(reader);
            printf("Error getting Section. %s\n", confreaderror->message);
            return false;
        }
        std::string section = json_reader_get_string_value(reader);
        json_reader_end_member(reader);

        if ("Common" == section) {
            const char *key1 = "VehicleInfoDefine";

            if (!json_reader_read_member(reader, key1)) {
                confreaderror = json_reader_get_error(reader);
                printf("Error getting %s. %s\n", key1,
                       confreaderror->message);
                return false;
            }

            confreaderror = json_reader_get_error(reader);
            if (confreaderror != NULL) {
                confreaderror = json_reader_get_error(reader);
                printf("Error getting %s member. %s\n", key1,
                       confreaderror->message);
                return false;
            }
            g_assert(json_reader_is_array(reader));

            for (int j = 0; j < json_reader_count_elements(reader); j++) {
                json_reader_read_element(reader, j);
                if (!json_reader_read_member(reader, "KeyEventType")) {
                    confreaderror = json_reader_get_error(reader);
                    printf("Error getting %s. %s\n", key1,
                           confreaderror->message);
                    continue;
                }
                else {
                    std::string str = json_reader_get_string_value(reader);
                    json_reader_end_member(reader);
                    if (m_viList.isContainVehicleName(str.c_str())) {
                        int priority;
                        if (GetConfigValue(reader, "Priority", &priority, 0)) {
                            m_viList.setPriority(str.c_str(), priority);
                        }
                    }
                }
                json_reader_end_element(reader);
            }
            json_reader_end_member(reader);
            const char key2[][32] = {
                "DefaultInfoPort",
                "CustomizeInfoPort"
            };
            const char *subkey1 = "DataPort";
            const char *subkey2 = "CtrlPort";

            for (int i = 0; i < 2; i++) {
                if (!json_reader_read_member(reader, key2[i])) {
                    confreaderror = json_reader_get_error(reader);
                    printf("Error getting %s. %s\n", key2[i],
                           confreaderror->message);
                    return false;
                }
                confreaderror = json_reader_get_error(reader);
                if (confreaderror != NULL) {
                    confreaderror = json_reader_get_error(reader);
                    printf("Error getting %s member. %s\n", key2[i],
                           confreaderror->message);
                    return false;
                }

                if (!json_reader_read_member(reader, subkey1)) {
                    confreaderror = json_reader_get_error(reader);
                    printf("Error getting %s->%s. %s\n", key2[i],
                           subkey1, confreaderror->message);
                    return false;
                }
                confreaderror = json_reader_get_error(reader);
                if (confreaderror != NULL) {
                    confreaderror = json_reader_get_error(reader);
                    printf("Error getting %s->%s member. %s\n", key2[i],
                           subkey1, confreaderror->message);
                    return false;
                }
                m_ambpicomm_port[i * 2] = json_reader_get_int_value(reader);
                json_reader_end_member(reader);

                if (!json_reader_read_member(reader, subkey2)) {
                    confreaderror = json_reader_get_error(reader);
                    printf("Error getting %s->%s. %s\n", key2[i],
                           subkey2, confreaderror->message);
                    return false;
                }
                confreaderror = json_reader_get_error(reader);
                if (confreaderror != NULL) {
                    confreaderror = json_reader_get_error(reader);
                    printf("Error getting %s->%s member. %s\n", key2[i],
                           subkey2, confreaderror->message);
                    return false;
                }
                m_ambpicomm_port[i * 2 + 1] =
                    json_reader_get_int_value(reader);
                json_reader_end_member(reader);
                json_reader_end_member(reader);
            }
        }
        json_reader_end_element(reader);
    }
    json_reader_end_member(reader);

    return true;
}

/*--------------------------------------------------------------------------*/
/**
 * @brief   get value form configuration file(text data)
 *
 * @param[in]   r       JsonReader object
 * @param[in]   key     key
 * @param[out]  buf     data buffer
 * @param[in]   size    size of data buffer
 * @return  bool    false:failure
 */
/*--------------------------------------------------------------------------*/
bool CGtCtrl::GetConfigValText(JsonReader *r, const char *key, char *buf,
                               int size)
{
    std::string val;
    int len;

    memset(buf, 0x00, size);
    if (!json_reader_read_member(r, key)) {
        printf("Error getting string value.\n");
        return false;
    }
    else {
        val = json_reader_get_string_value(r);
        json_reader_end_member(r);
        len =
            (int) val.length() > (size - 1) ? (size - 1) : (int) val.length();
        memcpy(buf, val.c_str(), len);
    }
    return true;
}

/*--------------------------------------------------------------------------*/
/**
 * @brief   get value form configuration file(integer data)
 *
 * @param[in]   r       JsonReader object
 * @param[in]   key     key
 * @param[out]  buf     data buffer
 * @return  bool    false:failure
 */
/*--------------------------------------------------------------------------*/
bool CGtCtrl::GetConfigValInt(JsonReader *r, const char *key, int *buf)
{

    *buf = 0;
    if (!json_reader_read_member(r, key)) {
        printf("Error getting integer value.\n");
        return false;
    }
    else {
        *buf = (int) json_reader_get_int_value(r);
        json_reader_end_member(r);
    }
    return true;
}

/*--------------------------------------------------------------------------*/
/**
 * @brief   get value form configuration file(double data)
 *
 * @param[in]   r       JsonReader object
 * @param[in]   key     key
 * @param[out]  buf     data buffer
 * @return  bool    false:failure
 */
/*--------------------------------------------------------------------------*/
bool CGtCtrl::GetConfigValDouble(JsonReader *r, const char *key, double *buf)
{

    *buf = 0.0;
    if (!json_reader_read_member(r, key)) {
        printf("Error getting floating point value.\n");
        return false;
    }
    else {
        *buf = (double) json_reader_get_double_value(r);
        json_reader_end_member(r);
    }
    return true;
}

/*--------------------------------------------------------------------------*/
/**
 * @brief   get value form configuration file(boolean data)
 *
 * @param[in]   r       JsonReader object
 * @param[in]   key     key
 * @param[out]  buf     data buffer
 * @return  bool    false:failure
 */
/*--------------------------------------------------------------------------*/
bool CGtCtrl::GetConfigValBool(JsonReader *r, const char *key, bool *buf)
{

    *buf = false;
    if (!json_reader_read_member(r, key)) {
        printf("Error getting boolean value.\n");
        return false;
    }
    else {
        *buf = (double) json_reader_get_boolean_value(r);
        json_reader_end_member(r);
    }
    return true;
}

/*--------------------------------------------------------------------------*/
/**
 * @brief   get value form configuration file
 *
 * @param[in]   r       JsonReader object
 * @param[in]   key     key
 * @param[out]  buf     data buffer
 * @param[in]   size    size of buf
 * @return  bool    false:failure
 */
/*--------------------------------------------------------------------------*/
bool CGtCtrl::GetConfigValue(JsonReader *r, const char *key, char *buf,
                             int size)
{
    return GetConfigValText(r, key, buf, size);
}

bool CGtCtrl::GetConfigValue(JsonReader *r, const char *key, int *buf,
                             int size)
{
    return GetConfigValInt(r, key, buf);
}

bool CGtCtrl::GetConfigValue(JsonReader *r, const char *key, double *buf,
                             int size)
{
    return GetConfigValDouble(r, key, buf);
}

bool CGtCtrl::GetConfigValue(JsonReader *r, const char *key, bool *buf,
                             int size)
{
    return GetConfigValBool(r, key, buf);
}


void CGtCtrl::CheckSendResult(int mqid)
{

    KeyDataMsg_t ret;

    while (1) {
        if (msgrcv(mqid, &ret, sizeof(KeyDataMsg_t), 31, IPC_NOWAIT) == -1) {
            break;
        }
        else {
            // ERROR
            list < string >::iterator pos;
            pos =
                find(m_sendMsgInfo.begin(), m_sendMsgInfo.end(),
                     string(ret.KeyEventType));
            if (pos != m_sendMsgInfo.end()) {
                printf("send error: AMB cannot receive %s\n",
                       ret.KeyEventType);
            }
        }
    }
}

void CGtCtrl::LoadRouteList() {
    std::cout << "LoadRouteList:orgmsgQueue[" << orgmsgQueue << "]\n";
    if (orgmsgQueue == "") {
        std::ifstream fin;
        fin.open(g_RouteListFile.c_str());
        if (!fin) {
            std::cerr << "Can't read " << g_RouteListFile << std::endl;
        }
        std::string line;
        while (fin && getline(fin, line)) {
            orgmsgQueue.append(line);
            orgmsgQueue.append("\n");
        }
        msgQueue = orgmsgQueue;
        fin.close();
    }
    else {
        msgQueue = orgmsgQueue;
    }
    if (msgQueue.size() > 0) {
        char buf[32];
        int pos = 0;
        int lastidx = 0;
        geoData tmpGeo;
        int i = 0;

        tmpGeo.lat = GEORESET;
        tmpGeo.lng = GEORESET;

        for (i = 0; i < (int) msgQueue.size(); i++) {
            if (msgQueue[i] == ',') {
                tmpGeo.lat = atof(buf);
                pos = 0;
            }
            else if (msgQueue[i] == '\n') {
                tmpGeo.lng = atof(buf);
                pos = 0;

                routeList.push(tmpGeo);
                lastidx = i;
                tmpGeo.lat = GEORESET;
                tmpGeo.lng = GEORESET;
            }
            else {
                buf[pos++] = msgQueue[i];
            }
        }
    }
}

/**
 * End of File. (CGtCtrl.cpp)
 */
