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
 * @file    CGtCtrl.h
 */

#ifndef CGTCTRL_H_
#define CGTCTRL_H_

#include <stdio.h>
#include <signal.h>
#include <math.h>
#include "CConf.h"
#include "CJoyStick.h"
#include "CJoyStickEV.h"

#include <pthread.h>

#include <queue>
#include <string>

#include <cstddef>
#include <sstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <vector>
#include <list>
#include <algorithm>
#include <glib.h>
#include <json-glib/json-glib.h>

#include "Websocket.h"
#if 1
#define MAX_SPEED   199
#else
#define MAX_SPEED   280
#endif

const useconds_t g_sleeptime = 10000; // = 10milli sec = 0.01 sec
#define D_RUNLOOP_INTERVAL_COUNT  5
#define D_RUNLOOP_INTERVAL_COUNT2 50

#define GEORESET 1000
#define PIE 3.14159265

#ifdef  DEBUG
#define NOTRCSIZE(n)    (n | 0x01000000)
#else
#define NOTRCSIZE(n)    (n)
#endif


#define AMB_CONF        "/etc/ambd/config"

struct geoData
{
    double lat;
    double lng;
};

void DisconnectClient(int ClientID);
int ClientHandShake(int ClientID);
int ClientRequest(int ClientID);
void *Comm(void *s);

void MakeFdSet(void);
void FullMakeFdSet(void);

void DaemonTerminate(int SigNo);
void CloseAllSocket(void);


struct VehicleInfoNameList
{
    const static int maxlen = 64;

  public:
    int length;
    char name[maxlen][64];
    long priority[maxlen];

    /*--------------------------------------------------------------------------*/
    /**
     * @brief   initialize
     *
     * @param   none
     * @return  none
     */
    /*--------------------------------------------------------------------------*/
    void init()
    {
        length = 0;
        memset(&name[0], 0x00, maxlen * 64);
        memset(&priority[0], 0x00, sizeof(priority));
    }

    /*--------------------------------------------------------------------------*/
    /**
     * @brief   check to exist vehicle information name
     *
     * @param[in]   s       name of vehicle information
     * @return  bool    true:exist
     */
    /*--------------------------------------------------------------------------*/
    bool isContainVehicleName(const char *s)
    {
        bool rtn = false;

        if (s == NULL)
            return false;
        if (getIdx(s) >= 0) {
            rtn = true;
        }
        return rtn;
    }

    /*--------------------------------------------------------------------------*/
    /**
     * @brief   set priority to send vehicle information
     *
     * @param[in]   s       name of vehicle information
     * @param[in]   p       priority
     * @return  none
     */
    /*--------------------------------------------------------------------------*/
    void setPriority(const char *s, long p)
    {
        int idx = 0;

        if (s == NULL)
            return;
        if ((idx = getIdx(s)) >= 0) {
            priority[idx] = p;
        }
    }

    /*--------------------------------------------------------------------------*/
    /**
     * @brief   get priority to send vehicle information
     *
     * @param[in]   s   name of vehicle information
     * @return  int     priority. if negative value returned, failure
     */
    /*--------------------------------------------------------------------------*/
    long getPriority(const char *s)
    {
        int idx = 0;
        long rtn = 0;

        if (s == NULL)
            return 0;
        if ((idx = getIdx(s)) >= 0) {
            rtn = priority[idx];
        }

        return rtn;
    }

  private:
    /*--------------------------------------------------------------------------*/
    /**
     * @brief   get number of vehicle information list
     *
     * @param[in]   s       name of vehicle information
     * @return  int     number of element
     */
    /*--------------------------------------------------------------------------*/
    int getIdx(const char *s)
    {
        int limit = length < maxlen ? length : maxlen;
        int rtn = -1;
        for (int i = 0; i < limit; i++) {
            if (!strcmp(name[i], s)) {
                rtn = i;
                break;
            }
        }
        return rtn;
    }
};


struct VehicleInfo
{
    int nSteeringAngle;
    int nShiftPos;
    int nAirconTemp;
    int nHeadLightPos;
    int nVelocity;
    int nDirection;
    int nWinkerPos;
    bool bHazard;
    bool bWinkR;
    bool bWinkL;
    double fLng;
    double fLat;
    bool bBrake;
    int nBrakeHydraulicPressure;
    double dVelocity;
    int nAccel;
    int nBrake;
};

enum SHIFT_POS
{
    SHIFT_UNKNOWN = 0,
    PARKING,
    REVERSE,
    NEUTRAL,
    DRIVE,
    FIRST,
    SECOND,
    THIRD,
    SPORTS
};

enum WINKER_POS
{
    WINKER_UNKNOWN = 0,
    WINKER_OFF,
    WINKER_RIGHT,
    WINKER_LEFT
};

enum AIRCON_TEMP
{
    MAX_TEMP = 300,
    MIN_TEMP = 160
};

enum HEAD_LIGHT
{
    HL_UNKNOWN = 0,
    HL_OFF,
    HL_SMALL,
    HL_LOW,
    HL_HIGH
};

enum ProtocolType
{
    dataport_def,
    ctrlport_def,
    dataport_cust,
    ctrlport_cust
};

class CGtCtrl
{
  public:
            CGtCtrl();
    virtual ~CGtCtrl();

    CConf myConf;
    CJoyStick* myJS;
    bool m_bUseGps;

    bool Initialize();
    bool Terminate();

    void Run();
    void Run2();
    static void signal_handler(int signo);

  private:
    int m_nJoyStickID;

    bool m_bFirstOpen;

    VehicleInfo m_stVehicleInfo;

    int m_websocket_port[4];
    WebsocketIF m_websocket_client[4];
    KeyEventOptMsg_t m_msgOpt;
    KeyDataMsg_t m_msgDat;

    VehicleInfoNameList m_viList;

    std::list<std::string> m_sendMsgInfo;

    bool LoadConfigJson(const char *);
    bool LoadConfigAMBJson(const char *, char *, int);
    bool LoadConfigJsonCommon(JsonReader *);
    bool LoadConfigJsonCarSim(JsonReader *);
    void DelJsonObj(JsonParser *, JsonReader *);
    bool SendVehicleInfo(ProtocolType type, const char *key, bool data);
    bool SendVehicleInfo(ProtocolType type, const char *key, int data);
    bool SendVehicleInfo(ProtocolType type, const char *key, int data[],
                         int len);
    bool SendVehicleInfo(ProtocolType type, const char *key, double data[],
                         int len);
    bool SendVehicleInfo(ProtocolType type, const char *key, char data[],
                         int len);
    bool sendVehicleInfo(ProtocolType type, const char *key, void *data,
                         unsigned int unit_size, int unit_cnt);
    bool GetConfigValue(JsonReader *, const char *, char *, int);
    bool GetConfigValue(JsonReader *, const char *, int *, int);
    bool GetConfigValue(JsonReader *, const char *, double *, int);
    bool GetConfigValue(JsonReader *, const char *, bool *, int);
    bool GetConfigValText(JsonReader *, const char *, char *, int);
    bool GetConfigValInt(JsonReader *, const char *, int *);
    bool GetConfigValDouble(JsonReader *, const char *, double *);
    bool GetConfigValBool(JsonReader *, const char *, bool *);
    void SetMQKeyData(char *buf, unsigned int bufsize, long &mtype,
                      const char *key, char status[], unsigned int size);
    void CheckSendResult(int mqid);
};

#endif /* CGTCTRL_H_ */
/**
 * End of File.(CGtCtrl.h)
 */
