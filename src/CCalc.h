/*
 * Copyright (c) 2013, TOYOTA MOTOR CORPORATION.
 *
 * This program is licensed under the terms and conditions of the 
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */
/**
 * @brief   Computation tool
 *
 * @file    CCalc.h
 */
#ifndef CCALC_H
#define CCALC_H

#include <math.h>
#include <stdio.h>

#define WHEEL_BASE 3.0

typedef struct
{
    double lat;
    double lng;
} POINT;


/*--------------------------------------------------------------------------*/
/**
 * @brief   calc direction
 *
 * @param[in]  azim         current direction
 * @param[in]  delta        angle
 * @param[in]  dist         distance
 * @return  double      new direction
 */
/*--------------------------------------------------------------------------*/
double CalcAzimuth(double azim, double delta, double dist);

/*--------------------------------------------------------------------------*/
/**
 * @brief   calc new point
 *
 * @param[in]  lat          current lat
 * @param[in]  lon          current lon
 * @param[in]  azim         current direction
 * @param[in]  dist         distance
 * @return     POINT       new lat and lon
 */
/*--------------------------------------------------------------------------*/
POINT CalcDest(double lat, double lng, double azim, double dist);

#endif // CCALC_H
