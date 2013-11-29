/*
 * Copyright (c) 2013, TOYOTA MOTOR CORPORATION.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */
/**
 * @file    CarSim_Daemon.cpp
 * @brief   main entry point
 */

#include <sys/types.h>
#include <iostream>
#include <unistd.h>
#include <getopt.h>
#include <string>
#include "CGtCtrl.h"
using namespace std;

#define VERSION "0.1.2"

bool gbDevJs = false;

int main(int argc, char **argv)
{
    // parse cmd line
    int result = 0;
    bool bUseGps = false;
    bool bTestMode = false;
    bool b;
    bool comFlg = false;
    bool bDemoRunning = false;

    // parse command line
    while ((result = getopt(argc, argv, "jhvgctr")) != -1) {
        switch (result) {
        case 'h':
            printf("Usage: CarSim_Daemon [-g]\n");
            printf("  -g\t Get GPS form smartphone\n");
            return 0;
            break;
        case 'v':
            printf("CarSim_Daemon version:%s\n", VERSION);
            return 0;
            break;
        case 'g':
            bUseGps = true;
            break;
        case 'c':
            printf("Using amb plug-in I/F\n");
            comFlg = true;
            break;
        case 't':
            bTestMode = true;
            break;
        case 'j':
            gbDevJs = true;
            break;
        case 'r':
            bDemoRunning = true;
            break;
        }
    }

    if (comFlg) {
        CGtCtrl myGtCtrl;

        b = myGtCtrl.Initialize();

        if (b) {
            myGtCtrl.m_bUseGps = bUseGps;

            myGtCtrl.Run2();

            myGtCtrl.Terminate();
        }

        return 0;
    }
    if (bTestMode) {
        // test mode
        CJoyStick cjs;
        b = cjs.Open();
        if (b) {
            printf
                ("Carsim_Daemon Test mode...\n Press Ctrl+C to close program...\n");
            int type;
            int number;
            int value;

            while (true) {
                type = cjs.Read(&number, &value);

                if (type >= 0)
                    printf("type=%d, number=%d, value=%d\n", type, number,
                           value);

                usleep(50000);
            }
            cjs.Close();
        }
        else {
            printf("joystick open error(test mode)\n");
        }
    }
    else {
        // change to super user
        if (setuid(0) < 0)  {
            printf("can not set super user\n");
        }

        CGtCtrl myGtCtrl;
        myGtCtrl.m_bUseGps = bUseGps;
        myGtCtrl.m_bDemoRunning = bDemoRunning;
        b = myGtCtrl.Initialize();

        if (b) {
            myGtCtrl.Run();
        }
        myGtCtrl.Terminate();
    }
    return 0;
}

/**
 * End of File.(CarSim_Daemon)
 */
