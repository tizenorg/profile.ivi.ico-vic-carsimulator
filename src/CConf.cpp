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
 * @file    CConf.cpp
 *
 */

#include <unistd.h>

#include "CConf.h"

CConf::CConf()
{
    // TODO Auto-generated constructor stub

}

CConf::~CConf()
{
    // TODO Auto-generated destructor stub
}

void CConf::LoadConfig(const char*filePath)
{
    //CConf::GetModulePath(m_strConfPath, sizeof(m_strConfPath));

    //strncat(m_strConfPath, filePath, sizeof(m_strConfPath));
    memset(m_strConfPath, 0, sizeof(m_strConfPath));
    strcpy(m_strConfPath, filePath);
    printf("ConfPath:%s\n", m_strConfPath);

    m_nWinkR = CConf::GetConfig(m_strConfPath, "WINKER_RIGHT", "NUMBER", 4);
    m_nWinkL = CConf::GetConfig(m_strConfPath, "WINKER_LEFT", "NUMBER", 5);

    m_nShiftU = CConf::GetConfig(m_strConfPath, "SHIFT_UP", "NUMBER", 11);
    m_nShiftD = CConf::GetConfig(m_strConfPath, "SHIFT_DOWN", "NUMBER", 10);
    m_nShift1 = CConf::GetConfig(m_strConfPath, "SHIFT_1", "NUMBER", 300);
    m_nShift2 = CConf::GetConfig(m_strConfPath, "SHIFT_2", "NUMBER", 301);
    m_nShift3 = CConf::GetConfig(m_strConfPath, "SHIFT_3", "NUMBER", 302);
    m_nShift4 = CConf::GetConfig(m_strConfPath, "SHIFT_4", "NUMBER", 303);
    m_nShift5 = CConf::GetConfig(m_strConfPath, "SHIFT_5", "NUMBER", 704);
    m_nShift6 = CConf::GetConfig(m_strConfPath, "SHIFT_6", "NUMBER", 705);
    m_nShiftR = CConf::GetConfig(m_strConfPath, "SHIFT_R", "NUMBER", 710);
    m_nSteering = CConf::GetConfig(m_strConfPath, "STEERING", "NUMBER", 0);
    m_nAccel = CConf::GetConfig(m_strConfPath, "ACCEL", "NUMBER", 1);
    m_nBrake = CConf::GetConfig(m_strConfPath, "BRAKE", "NUMBER", 2);

    m_nHeadLight =
        CConf::GetConfig(m_strConfPath, "HEAD_LIGHT", "NUMBER", 292);

    m_fLat =
        CConf::GetConfig(m_strConfPath, "LASTPOSITION", "LAT", 35.717931);
    m_fLng =
        CConf::GetConfig(m_strConfPath, "LASTPOSITION", "LNG", 139.736518);
    char devname[64];
    memset(devname, 0, sizeof(devname));
    CConf::GetConfig(m_strConfPath, "DEVICE", "NAME", "Driving Force GT", devname, sizeof(devname));
    m_sDeviceName = std::string(devname);
	char ambconfig[64];
	memset(ambconfig, 0, sizeof(ambconfig));
	CConf::GetConfig(m_strConfPath, "AMBCONFIG", "NAME", "/etc/ambd/config", ambconfig, sizeof(ambconfig));
    m_sAmbConfigName = std::string(ambconfig);

    printf("Configuration[%s]:\n",m_sDeviceName.c_str());
    printf("  WINKER(R) button:%d\tWINKER(L) button:%d\n", m_nWinkR,
           m_nWinkL);
    printf("  SHIFT(U) button:%d\tSHIFT(D) button:%d\n", m_nShiftU,
           m_nShiftD);
    printf("  STEERING axis:%d\tACCEL axis:%d\n", m_nSteering, m_nAccel);
}

bool CConf::GetConfig(const char *strPath, const char *strSection,
                      const char *strKey, const char *strDefault, char *buf,
                      int bufsize)
{
    char *line = NULL;
    size_t len = 0;
    std::string key = "";
    std::string sec = "";

    char retbuf[STR_BUF_SIZE];
    memset(retbuf, 0, sizeof(retbuf));

    if (strDefault == NULL) {
        strncpy(buf, "" "", bufsize);
        return false;
    }
    if ((strSection == NULL) || (strKey == NULL)) {
        strncpy(buf, strDefault, bufsize);
        return false;
    }

    FILE *fp = fopen(strPath, "r");
    if (fp == NULL) {
        strncpy(buf, strDefault, bufsize);
        return false;
    }

    key = strKey;
    if (key.rfind("=", key.length()) != key.length() - 1) {
        key += "=";
    }

    while (true) {
        int n = getline(&line, &len, fp);
        if (n < 0) {
            strncpy(buf, strDefault, bufsize);
            return false;
        }

        std::string s = line;

        if (s.find("//", 0) == 0) {
            // skip comment
            continue;
        }

        if ((s.find("[", 0) == 0)
            && (s.rfind("]", s.length()) == s.length() - 2)) {
            s = s.erase(0, 1);
            sec = s.erase(s.rfind("]"), s.length() - s.rfind("]"));
            continue;
        }

        if ((s.find(key, 0) == 0) && (sec == strSection)) {
            s.erase(0, key.length());
            s.erase(s.length() - 1, 2);

            ::memcpy(retbuf, s.c_str(), s.length());
            strncpy(buf, retbuf, bufsize);
            break;
        }
    }


    if (fp)
        fclose(fp);
    return true;
}

int CConf::GetConfig(const char *strPath, const char *strSection,
                     const char *strKey, int nDefault)
{
    int nRet = nDefault;

    if (strPath == NULL)
        return nRet;
    if (strSection == NULL)
        return nRet;
    if (strKey == NULL)
        return nRet;

    char buf[STR_BUF_SIZE];

    memset(buf, 0, sizeof(buf));
    bool b =
        CConf::GetConfig(strPath, strSection, strKey, "", buf, sizeof(buf));

    if (!b)
        return nRet;

    return atoi((char *) buf);

}

double CConf::GetConfig(const char *strPath, const char *strSection,
                        const char *strKey, double fDefault)
{
    double fRet = fDefault;

    if (strPath == NULL)
        return fRet;
    if (strSection == NULL)
        return fRet;
    if (strKey == NULL)
        return fRet;

    char buf[STR_BUF_SIZE];

    bool b =
        CConf::GetConfig(strPath, strSection, strKey, "", buf, sizeof(buf));

    if (!b)
        return fRet;

    return atof((char *) buf);
}

void CConf::GetModulePath(char *buf, int bufsize)
{
    size_t size = readlink("/proc/self/exe", buf, (size_t) bufsize);
    buf[bufsize] = '\0';

    char *p = strrchr(buf, '/');
    if (p != 0L)
        *p = '\0';

    if (buf[strlen(buf) - 1] == '/')
        buf[strlen(buf) - 1] = '\0';
}

/**
 * End of File.(CConf.cpp)
 */
