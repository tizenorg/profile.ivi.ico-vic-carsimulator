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
 * @file    CJoyStick.h
 */
#include <iostream>
#include <string>
#include <vector>
#include <stdio.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include "CJoyStick.h"
using namespace std;

/**
 * @brief constructor
 */
CJoyStick::CJoyStick()
{
    // TODO Auto-generated constructor stub
    m_nJoyStickID = -1;
}

CJoyStick::~CJoyStick()
{
    // TODO Auto-generated destructor stub
}

/**
 * @brief open joystick
 * @retval return device id, if error occureed return nagative value
 */
int CJoyStick::Open()
{

    for (int i = 3; i >= 0; i--) {
        ::snprintf(m_strJoyStick, sizeof(m_strJoyStick), "/dev/input/js%d",
                   i);

        m_nJoyStickID = open(m_strJoyStick, O_RDONLY | O_NONBLOCK);

        if (m_nJoyStickID >= 0) {
            printf("Open joystick... ID=/dev/input/js%d\n", i);

            ioctl(m_nJoyStickID, JSIOCGAXES, &m_ucAxes);
            ioctl(m_nJoyStickID, JSIOCGBUTTONS, &m_ucButtons);
            ioctl(m_nJoyStickID, JSIOCGNAME(sizeof(m_strDevName)),
                  m_strDevName);

            printf
                ("JoyStick ID[%d], JoyStick Name[%s], Axis[%d], Buttons[%d]\n",
                 m_nJoyStickID, m_strDevName, m_ucAxes, m_ucButtons);

            if (::
                strncmp(m_strDevName, "Driving Force GT",
                        sizeof(m_strDevName)) != 0) {
                printf
                    ("/dev/input/js%d is not Driving Force GT device, close device\n",
                     i);
                this->Close();
            }
            else {
                fds.fd = m_nJoyStickID;
                break;
            }
        }
    }

    return m_nJoyStickID;
}

/**
 * @brief get axis count
 */
int CJoyStick::GetAxisCount() const
{
    return (int) m_ucAxes;
}

/**
 * @brief get button count
 */
int CJoyStick::GetButtonsCount() const
{
    return (int) m_ucButtons;
}

/**
 * @brief joystick device file close
 * @retval 0:close success
 */
int CJoyStick::Close()
{
    if (0 > m_nJoyStickID) {
        return 0;
    }
    close(m_nJoyStickID);
    printf("Close joystick... ID=%d\n", m_nJoyStickID);
    m_nJoyStickID = -1;
    return 0;
}

/**
 * @brief get input value
 * @retval  return kind of input(axis or button), if error occurred, return negative value
 * @param[in/out]   nubmer  number of button or axis
 * @param[in/out]   value   value of input
 */
int CJoyStick::Read(int *number, int *value)
{
    struct js_event jse;

    fds.events = POLLIN;

    if (poll(&fds, 1, 50) == 0) {
        return -1;
    }

    if (fds.revents & POLLIN) {
        int r = read(m_nJoyStickID, &jse, sizeof(jse));

        int type = jse.type;
        type = type & JS_EVENT_INIT;

        if ((r >= 0) && (type == 0)) {
            r = jse.type & ~JS_EVENT_INIT;
            *number = jse.number;
            *value = jse.value;
        }
        else {
            *number = -1;
            *value = -1;
        }
        return r;
    }
    else {
        return -1;
    }
}

int CJoyStick::ReadData()
{
    struct JS_DATA_TYPE js;

    int r = read(m_nJoyStickID, &js, JS_RETURN);

    printf("ret=%d, jsbutton=%d, js.x=%d, js.y=%d\n", r, js.buttons, js.x,
           js.y);
    return 0;
}

int CJoyStick::deviceOpen(const std::string& dirNM,
                          const std::string& fileParts,
                          const std::string& deviceNM,
                          std::string& getDevice)
{
    vector<string> matchingNM;
    vector<string> files;
    matchingNM.push_back(fileParts);

    getDevices(dirNM, matchingNM, files);
    size_t sz = files.size();
    if (0 == sz) {
        return -1;
    }
    int r = -1;
    for (size_t i = 0; i < sz; i++) {
        int fd = open(files[i].c_str(), O_RDONLY | O_NONBLOCK);
        if (0 > fd) { /* open error */
            fprintf(stderr,"CJoyStick::deviceOpen: Event Device.%d(%s) Open Error<%d>\n",
                    i, files[i].c_str(), errno);
            continue; /* next device */
        }
        char sDevNm[128];
        if (false == getDeviceName(fd, sDevNm, sizeof(sDevNm))){ /* get DEV NAME */
            close(fd);
            continue; /* next device */
        }
        if(0 != deviceNM.compare(sDevNm)) { /* name not match */
            close(fd);
            continue; /* next device */
        }
        r = fd;
        getDevice = files[i];
        break;  /* break of for */
    }
    return r;
}
/**
 * @brief create a list of files in the specified path
 * @param dir path directory
 * @param matching get file name parts
 * @param fileList store file list
 */
void CJoyStick::getDevices(const std::string& dir,
                const std::vector<std::string>& matching,
                std::vector<std::string>& filesList) const
{
    if (dir.empty()) {
#ifdef _DEBUG_
        std::cerr << "NO DHIRECTORY\n";
#endif
        return;
    }
    int dsz = dir.length();
    string tmpdir(dir);
    if ('/' != dir[dsz-1]) {
        tmpdir += string("/");
    }
    DIR* dp=opendir(dir.c_str());
    if (dp != NULL) {
        struct dirent* dent;
        int mssz = matching.size();
        do {
            dent = readdir(dp);
            if (dent != NULL) {
                bool bMatching = false;
                for (int i = 0; i < mssz; i++) {
                    if (strstr(dent->d_name, matching[i].c_str())) {
                        bMatching = true;
                        break;  /* break of for */
                    }
                }
                if (true == bMatching) {
                    string tmp(tmpdir);
                    tmp += string(dent->d_name);
                    filesList.push_back(tmp);
                }
            }

        } while (dent!=NULL);
        closedir(dp);
    }
}


bool CJoyStick::getDeviceName(int fd, char* devNM, size_t sz)
{
    if (0 > fd) {
        return false;
    }
    if (0 > ioctl(fd, JSIOCGNAME(sz), devNM)) {
        return false;
    }
    return true;
}


/**
 * End of File.(CJoyStick.cpp)
 */
