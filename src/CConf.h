/*
 * Copyright (c) 2013, TOYOTA MOTOR CORPORATION.
 *
 * This program is licensed under the terms and conditions of the 
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */
/**
 * @brief   Read configuration file
 * @file    CConf.h
 */

#ifndef CCONF_H_
#define CCONF_H_

#include <string>
#include <string.h>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>

#define STR_BUF_SIZE    2048

class CConf
{
  public:
            CConf();
    virtual ~CConf();

    char m_strConfPath[260];

    static int GetConfig(const char *strPath, const char *strSection,
                         const char *strKey, int nDefault);
    static double GetConfig(const char *strPath, const char *strSection,
                            const char *strKey, double fDefault);
    static bool GetConfig(const char *strPath, const char *strSection,
                          const char *strKey, const char *strDefault,
                          char *buf, int bufsize);

    static bool SetConfig(const char *strPath, const char *strSection,
                          const char *strKey, int nValue);
    static bool SetConfig(const char *strPath, const char *strSection,
                          const char *strKey, double fValue);
    static bool SetConfig(const char *strPath, const char *strSection,
                          const char *strKey, const char strValue);

    static void GetModulePath(char *buf, int bufsize);

    void LoadConfig(const char*filePath);


    int m_nHazard;
    int m_nWinkR;
    int m_nWinkL;
    int m_nAircon;
    int m_nShiftU;
    int m_nShiftD;
    int m_nHeadLight;
    int m_nSteering;
    int m_nAccel;
    int m_nBrake;
    int m_nShift1;
    int m_nShift2;
    int m_nShift3;
    int m_nShift4;
    int m_nShift5;
    int m_nShift6;
    int m_nShiftR;
    double m_fLng;
    double m_fLat;
    std::string m_sDeviceName;
    std::string m_sAmbConfigName;

};

#endif /* CCONF_H_ */
/**
 * End of File.(CConf.h)
 */
