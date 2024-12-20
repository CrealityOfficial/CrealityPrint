#include <math.h>
#include <stdlib.h>
#define KL_DEFULT    0.8 //0.1
#define KC_DEFULT    0.6 //0.1
#define KH_DEFULT    0.8 //0.1
//#ifdef Q_OS_WINDOWS
#ifndef M_PI
#define M_PI 3.14159265358979323846 
#endif
//#endif
/***************************************色距算法*****************************************/
/**
 * @description: 
 * @return {*}
 * @param {double} value
 */
double linear(double value) 
{
    if (value > 0.04045) {
        return pow((value + 0.055) / 1.055, 2.4);
    } else {
        return value / 12.92;
    }
}

/**
 * @description: 将值转换为 f(t)
 * @return {*}
 * @param {double} t
 */
double f(double t) 
{
    if (t > 0.008856) {
        return pow(t, 1.0 / 3.0);
    } else {
        return 7.787 * t + 16.0 / 116.0;
    }
}

/**
 * @description: XYZ 到 CIE L*a*b* 的转换
 * @return {*}
 * @param {double} x
 * @param {double} y
 * @param {double} z
 * @param {double} *l
 * @param {double} *a
 * @param {double} *b
 */
void xyzToLab(double x, double y, double z, double *l, double *a, double *b) 
{
    *l = 116.0 * f(y) - 16.0;
    *a = 500.0 * (f(x) - f(y));
    *b = 200.0 * (f(y) - f(z));
}

/**
 * @description: Convert RGB to XYZ
 * @return {*}
 * @param {double} r
 * @param {double} g
 * @param {double} b
 * @param {double} xyz
 */
void rgb_to_xyz(double r, double g, double b, double xyz[3]) 
{
    double r_linear = linear(r);
    double g_linear = linear(g);
    double b_linear = linear(b);

    // D65 白点
    const double refX = 0.95047;
    const double refY = 1.00000;
    const double refZ = 1.08883;

    xyz[0] = r_linear * 0.412453 + g_linear * 0.357580 + b_linear * 0.180423;
    xyz[1] = r_linear * 0.212671 + g_linear * 0.715160 + b_linear * 0.072169;
    xyz[2] = r_linear * 0.019334 + g_linear * 0.119193 + b_linear * 0.950227;

    xyz[0] /= refX;
    xyz[1] /= refY;
    xyz[2] /= refZ;
}

/**
 * @description: Convert XYZ to Lab
 * @return {*}
 * @param {double} x
 * @param {double} y
 * @param {double} z
 * @param {double} lab
 */
void xyz_to_lab(double x, double y, double z, double lab[3]) {

    lab[0] = 116.0 * f(y) - 16.0;
    lab[1] = 500.0 * (f(x) - f(y));
    lab[2] = 200.0 * (f(y) - f(z));
}

/**
 * @description: Calculate Delta E
 * @return {*}
 * @param {double} lab1
 * @param {double} lab2
 */
double delta_e(double lab1[3], double lab2[3]) {
    double l_diff = lab1[0] - lab2[0];
    double a_diff = lab1[1] - lab2[1];
    double b_diff = lab1[2] - lab2[2];

    return sqrt(l_diff * l_diff + a_diff * a_diff + b_diff * b_diff);
}

/**
 * @description: 
 * @return {*}
 * @param {double} Lab1
 * @param {double} Lab2
 * @param {double} KL
 * @param {double} KC
 * @param {double} KH
 */
double DeltaE2000(double Lab1[3], double Lab2[3], double KL, double KC, double KH)
{
    double L1 = Lab1[0];
    double a1 = Lab1[1];
    double b1 = Lab1[2];
    double L2 = Lab2[0];
    double a2 = Lab2[1];
    double b2 = Lab2[2];
    //------------------------------------------
    // KL = 1; //控制亮度差异对总色差的影响。亮度差异指的是两个颜色在亮度上的差异。
    // KC = 2; //控制色度差异对总色差的影响。色度差异指的是两个颜色在饱和度上的差异。
    // KH = 1; //控制色相差异对总色差的影响。色相差异指的是两个颜色在色调上的差异。
    //------------------------------------------
    // delta:△  L:flag LL:L′ LL_:L'
    double aLx_mean = (L1 + L2) * 0.5;
    double aLx_mean50_2 = (aLx_mean - 50.) * (aLx_mean - 50.);
    double aDeltaLx = L2 - L1;
    double aS_L = 1.0 + 0.015 * aLx_mean50_2 / sqrt(20 + aLx_mean50_2);

    double aDL = aDeltaLx / (aS_L * KL);
    //------------------------------------------
    double aC1 = sqrt(a1 * a1 + b1 * b1);
    double aC2 = sqrt(a2 * a2 + b2 * b2);
    double aC_mean = 0.5 * (aC1 + aC2);
    // cal G
    double aC_mean_pow7 = pow(aC_mean, 7);
    double a25_pow7 = pow(25, 7);
    double aG = 0.5 * (1. - sqrt(aC_mean_pow7 / (aC_mean_pow7 + a25_pow7)));
    // cal A2 mean
    double a2x = a2 * (1. + aG); 
    double a1x = a1 * (1. + aG);
    double aC2x = sqrt(a2x * a2x + b2 * b2); // C2'
    double aC1x = sqrt(a1x * a1x + b1 * b1); // C1'
    // C- mean
    double aCx_mean = 0.5 * (aC2x + aC1x);
    double aS_C = 1.0 + 0.045 * aCx_mean;
    double aDeltaCx = aC2x - aC1x;

    double aDC = aDeltaCx / (aS_C * KC);
    //----------------------------------------
#define Epsilon 0.0001
    // 这里是换算成度，

    double ah1x = (aC1x > Epsilon ? atan2(b1, a1x) * (180. / M_PI) : 270.);
    double ah2x = (aC2x > Epsilon ? atan2(b2, a2x) * (180. / M_PI) : 270.);
    // printf("aDC1: %f %f %f %f\n",ah1x, ah2x, aC1x, aC2x);
    // ah1x = atan2(b1, a1x) * (180. / M_PI);
    // ah2x = atan2(b2, a2x) * (180. / M_PI);
    // printf("aDC2: %f %f %f  %f\n",ah1x, ah2x, aC1x, aC2x);
    if (ah1x < 0.)
        ah1x += 360; // h1' (12)
    if (ah2x < 0.)
        ah2x += 360; // h2' (13)

    double aHx_mean = 0.5 * (ah1x + ah2x);
    double aDeltahx = ah2x - ah1x; // (16)
    if (abs(aDeltahx) > 180.) //(14)
    {
        aHx_mean += (aHx_mean < 180. ? 180. : -180.);
        aDeltahx += (ah1x >= ah2x ? 360. : -360.); //(16)
    }
    else
    {
        //h2'-h1' <= 180
    }

    double aDeltaHx = 2. * sqrt (aC1x * aC2x) * sin (0.5 * aDeltahx * M_PI / 180.); //(19)
    double aT = 1. - 0.17 * cosf ((aHx_mean - 30.) * M_PI / 180.) +
                          0.24 * cosf ((2. * aHx_mean ) * M_PI / 180.) +
                          0.32 * cosf ((3. * aHx_mean +  6.) * M_PI / 180.) -
                          0.20 * cosf ((4. * aHx_mean - 63.) * M_PI / 180.);
    double aS_H = 1. + 0.015 * aCx_mean * aT; //(22)
    double aDH = aDeltaHx / (aS_H * KH); //(1)
//----------------------------------------
    double aDelta_theta = 30. * exp(-(aHx_mean - 275.) * (aHx_mean - 275.) / 625.); //(23）
    double aCx_mean_pow7 = pow(aCx_mean, 7);
    double aR_C = 2 * sqrt(aCx_mean_pow7 / (aCx_mean_pow7 + a25_pow7)); //(23)
    double aR_T = -aR_C * sin (2. * aDelta_theta * M_PI / 180.); //(25)

    double aDeltaE2000 = sqrt(aDL * aDL + aDC * aDC + aDH * aDH + aR_T * aDC * aDH);

    return aDeltaE2000;
}

