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

#ifndef CJOYSTICK_H_
#define CJOYSTICK_H_

#include <fcntl.h>
#include <linux/joystick.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>
#include <string>
#include <vector>

#define D_DEV_DIR_PATH      "/dev/input/"
#define D_DEV_NAME_PARTS_JS "js"
#define D_DEV_NAME_G25      "Driving Force GT"
#define D_DEV_NAME_G27      "G27 Racing Wheel"

class CJoyStick
{
  public:
            CJoyStick();
    virtual ~CJoyStick();

    virtual int Open();
    virtual int Close();
    virtual int Read(int *number, int *value);
    virtual int ReadData();

    int GetAxisCount() const;
    int GetButtonsCount() const;

    enum TYPE
    {
        AXIS = 0,
        BUTTONS
    };
    int  deviceOpen(const std::string& dirNM, const std::string& fileParts,
                    const std::string& deviceNM, std::string& getDevice);
    void getDevices(const std::string& dir,
                    const std::vector<std::string>& matching,
                    std::vector<std::string>& filesList) const;
    virtual bool getDeviceName(int fd, char* devNM, size_t sz);

protected:
    char m_strJoyStick[64];
    int m_nJoyStickID;

    unsigned char m_ucAxes;
    unsigned char m_ucButtons;
    char m_strDevName[80];

    fd_set m_stSet;
    struct pollfd fds;
};

#endif /* CJOYSTICK_H_ */
/**
 * End of File.(CJoyStick.h)
 */
