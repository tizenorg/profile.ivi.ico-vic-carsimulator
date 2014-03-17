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
#include <strings.h>
#include <stdlib.h>
#include <errno.h>
//#include <string>
#include "CGtCtrl.h"
#include "ico-util/ico_log.h"

using namespace std;

#define VERSION "0.1.2"

bool gbDevJs = false;

int main(int argc, char **argv)
{
    // parse cmd line
    bool bUseGps = false;
    bool bTestMode = false;
    bool b;
    bool comFlg = false;
    bool bDemoRunning = false;
    int ii;

//    printf("ico-vic-carsim: userid='%s', uid=%d, euid=%d)\n", cuserid(NULL), getuid(), geteuid());
    // parse command line
    for (ii = 1; ii < argc; ii++) {
        if (strcmp( argv[ii], "-h") == 0) {
            printf("Usage: CarSim_Daemon [-g]\n");
            printf("  -g\t Get GPS form smartphone\n");
            return 0;
        }
        else if (strcmp( argv[ii], "-v") == 0) {
            printf("CarSim_Daemon version:%s\n", VERSION);
            return 0;
        }
        else if (strcmp( argv[ii], "-g") == 0) {
            bUseGps = true;
        }
        else if (strcmp( argv[ii], "-c") == 0) {
            printf("Using amb plug-in I/F\n");
            comFlg = true;
        }
        else if (strcmp( argv[ii], "-t") == 0) {
            bTestMode = true;
        }
        else if (strcmp( argv[ii], "-j") == 0) {
            gbDevJs = true;
        }
        else if (strcmp( argv[ii], "-r") == 0) {
            bDemoRunning = true;
        }
        else if (strcasecmp( argv[ii], "--user") == 0) {
            ii ++;
            if (ii < argc)  {
                if (strcmp(argv[ii], cuserid(NULL)) != 0)    {
                    printf("ico-vic-carsim: abort(cannot run in a '%s' [uid=%d, euid=%d])\n", cuserid(NULL), getuid(), geteuid());
                    return -1;
                }
            }
        }
        else if (strcasecmp( argv[ii], "--uid") == 0) {
            ii ++;
            if (ii < argc)  {
                if (strtol(argv[ii], NULL, 10) != (long)getuid()) {
                    printf("ico-vic-carsim: abort(cannot run in a '%s' [uid=%d, euid=%d])\n", cuserid(NULL), getuid(), geteuid());
                    return -1;
                }
            }
        }
    }

    ico_log_open("ico-vic-carsim");
    ICO_INF( "START_MODULE ico-vic-carsim" );

    if (comFlg) {
        CGtCtrl myGtCtrl;

        b = myGtCtrl.Initialize();

        if (b) {
            myGtCtrl.m_bUseGps = bUseGps;

            myGtCtrl.Run2();

            myGtCtrl.Terminate();
        }

        ICO_INF( "END_MODULE ico-vic-carsim" );
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
            printf("can not set super user [errno=%d]\n", errno);
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

    ICO_INF( "END_MODULE ico-vic-carsim" );
    return 0;
}

/**
 * End of File.(CarSim_Daemon)
 */
