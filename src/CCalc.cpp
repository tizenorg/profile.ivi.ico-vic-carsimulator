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
 * @file    CCalc.cpp
 * @brief   utility of calc function 
 *
 */
#include <math.h>
#include <stdio.h>
#include "CCalc.h"

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
double CalcAzimuth(double azim, double delta, double dist)
{
    double tmp = delta * (M_PI / 180);

    double Rs = WHEEL_BASE / sin(tmp);
    Rs = fabs(Rs);
    double circle = 2.0 * M_PI * Rs;

    if (0.0 < dist)
        while (circle < dist)
            dist -= circle;
    else
        while (circle < fabs(dist))
            dist += circle;
    double theta = (180.0 * dist) / (M_PI * Rs);

    double retTheta = 0.0;
    if (delta < 0.0)
        retTheta = azim - theta;
    else
        retTheta = azim + theta;

    if (retTheta < 0.0)
        retTheta = retTheta + 360.0;
    if (360.0 < retTheta)
        retTheta = retTheta - 360.0;

    return retTheta;
}

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
POINT CalcDest(double lat, double lng, double azim, double dist)
{
    const double a = 6378137.0;
    const double b = 6356752.3142;
    const double f = 1 / 298.257223563;

    const double alpha1 = azim * (M_PI / 180);
    const double sinAlpha1 = sin(alpha1);
    const double cosAlpha1 = cos(alpha1);

    const double tanU1 = (1 - f) * tan(lat * (M_PI / 180));
    const double cosU1 = 1 / sqrt((1 + tanU1 * tanU1));
    const double sinU1 = tanU1 * cosU1;

    const double sigma1 = atan2(tanU1, cosAlpha1);
    const double sinAlpha = cosU1 * sinAlpha1;
    const double cosSqAlpha = 1 - sinAlpha * sinAlpha;
    const double uSq = cosSqAlpha * (a * a - b * b) / (b * b);
    const double A =
        1 + uSq / 16384 * (4096 + uSq * (-768 + uSq * (320 - 175 * uSq)));
    const double B =
        uSq / 1024 * (256 + uSq * (-128 + uSq * (74 - 47 * uSq)));

    double sigma = dist / (b * A);
    double sigmaP = 2 * M_PI;

    double cos2SigmaM = 0.0;
    double sinSigma = 0.0;
    double cosSigma = 0.0;
    double deltaSigma = 0.0;

    while (fabs(sigma - sigmaP) > 1e-12) {
        cos2SigmaM = cos(2 * sigma1 + sigma);
        sinSigma = sin(sigma);
        cosSigma = cos(sigma);
        deltaSigma =
            B * sinSigma * (cos2SigmaM + B / 4 *
                            (cosSigma *
                             (-1 + 2 * cos2SigmaM * cos2SigmaM) -
                             B / 6 * cos2SigmaM *
                             (-3 + 4 * sinSigma * sinSigma) *
                             (-3 + 4 * cos2SigmaM * cos2SigmaM)));
        sigmaP = sigma;
        sigma = dist / (b * A) + deltaSigma;
    }

    const double tmp = sinU1 * sinSigma - cosU1 * cosSigma * cosAlpha1;
    const double lat2 = atan2(sinU1 * cosSigma + cosU1 * sinSigma * cosAlpha1,
                              (1 - f) * sqrt(sinAlpha * sinAlpha +
                                             tmp * tmp));
    const double lambda = atan2(sinSigma * sinAlpha1,
                                cosU1 * cosSigma -
                                sinU1 * sinSigma * cosAlpha1);

    const double C = f / 16 * cosSqAlpha * (4 + f * (4 - 3 * cosSqAlpha));
    const double L =
        lambda - (1 - C) * f * sinAlpha *
        (sigma + C * sinSigma * (cos2SigmaM + C * cosSigma *
                                 (-1 + 2 * cos2SigmaM * cos2SigmaM)));

    double lng2 = (lng + (L * (180 / M_PI))) * (M_PI / 180);
    while (lng2 >= M_PI)
        lng2 -= (2 * M_PI);
    while (lng2 < -M_PI)
        lng2 += (2 * M_PI);

    POINT dest;
    dest.lat = lat2 * (180 / M_PI);
    dest.lng = lng2 * (180 / M_PI);

    return dest;
}


/**
 * End of File.(CCalc.cpp)
 */
