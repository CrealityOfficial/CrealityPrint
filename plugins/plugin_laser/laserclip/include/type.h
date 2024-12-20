#ifndef _GCORE_TYPE_H_
#define _GCORE_TYPE_H_

#include <stdint.h>
#include "pro_conf.h"

;
#pragma pack(push, 1)

/**
 * @brief ImgPoint / 像素坐标点:
 * 用于表示像素坐标系的坐标点
**/
typedef struct ImgPoint {
    int16_t x;  // unit: Pixel
    int16_t y;  // unit: Pixel
} ImgPoint;


/**
 * @brief GcoPoint / Gcode坐标点:
 * 用于表示雕刻坐标系的坐标点
 * 单位是： mm / 毫米
**/
typedef struct GcoPoint {
    int32_t x;    // unit: 0.0001mm
    int32_t y;    // unit: 0.0001mm
} GcoPoint;

typedef struct PixelData {
    ImgPoint pos;
    unsigned char grayval;
} PixelData;

typedef struct VectorData{
    PixelData* pt;
    int32_t ptNum;
    int32_t curr_idx;
}VectorData;

/**
 * @brief CarveStartPosition / 雕刻开始位置:
 * 这里指的开始位置是当我们正看图像时的四个角点，如下图所示：
 *
 *      TopLeft                        TopRight
 *             \          Top         /
 *              *-------------------*
 *              |                   |
 *              |                   |
 *              |                   |
 *        Left  |    Input Image    |  Right
 *              |                   |
 *              |                   |
 *              |                   |
 *              *-------------------*
 *             /        Bottom       \
 *   BottomLeft                        BottomRight
 *
 *   DefaultPosition = TopLeft
 *   Other Value / 其他值 将返回错误（!!!!当前未对错误值整理分类归纳，后续会完善错误信息）
 *
 * !!! 当前只支持组合:
 *      开始点是： TopLeft, 支持所有方向
 *      开始点是： BottomLeft, 支持StraightLeft, StraightRight
 *
 **/
typedef int8_t CarveStartPosition;
enum CarveStartPosition_ {
    DefaultPosition = 0,
    TopLeft = 1,
    TopRight = 2,
    BottomLeft = 3,
    BottomRight = 4
};


/**
 * @brief CarveDirection / 雕刻方向:
 * 这里的方向是相对于雕刻开始位置而言的。
 *
 * Left / Right：
 *      从图像中心正对雕刻开始位置的情况下，
 *      左手边为Left、右手边为Right。
 *
 * Straight / Diagonal：
 *      Straight 指与图像边界平行的方向进行 [行到行（Line To Line）] 且 [走Z型] 的方式来雕刻。示例如下：
 *                                     StartPoint = TopLeft        Direction = StraightRight
 *                                                \                /
 *              *-------------------*               *-------->-------->
 *              |                   |           S   <--------<--------<
 *              |                   |           P T >-------->-------->
 *              |                   |           I R <--------<--------<
 *              |    Input Image    |   ===>    N A >-------->-------->
 *              |                   |           D C <--------<--------<
 *              |                   |           L K >-------->-------->
 *              |                   |           E   <--------<--------<
 *              *-------------------*               >-------->-------->
 *
 *      Diagonal 指与图像边界成45°的方向进行 [行到行（Line To Line）] 且 [走Z型] 的方式来雕刻。
 *      此处无示例
 *
 *   DefaultDirection = StraightRight
 *   Other Value / 其他值 将返回错误（!!!!当前未对错误值整理分类归纳，后续会完善错误信息）
 *
 * !!! 当前只支持组合:
 *      开始点是： TopLeft, 支持所有方向
 *      开始点是： BottomLeft, 支持StraightLeft, StraightRight
 */
typedef int8_t CarveDirection;
enum CarveDirection_ {
    DefaultDirection = 0,
    StraightRight = 1,
    StraightLeft = 2,
    DiagonalRight = 3,
    DiagonalLeft = 4
};


/**
 * @brief MachineModel / 机型
 * 关于机型，后期会导入各个机型的配置。做适配，当前并未使用，
 * 1. 十字架系列， 分配： 1 - 19
 * 2. 振镜系列，分配： 20 - 39
 * 3. 龙门架系列，分配： 40 - 59
 * 4. 智能模组系列，分配： 60 - 79
 * 5. 3D打印机二合一系列, 分配： 100 以上
 * 6. UndefinedModuel 应该让用户可以对参数进行无限制配置（除DLL不支持的之外）
 */
typedef int8_t MachineModel;
enum MachineModel_ {
    UndefinedModel = 0,
    CV_01 = 1,
    CV_01_Pro = 2,
    CV_20 = 20,
    CV_20_Pro = 21,
    CV_30 = 40,
    CV_30_Pro = 41,
    CV_Laser_Module_Pro = 60,
    Ender3_S = 100,
    Ender3_V2_Laser = 101,
    CP_01 = 102,
};


/**
 * @brief GCodeStyle / 生成 GCode风格
 * DefaultStyle = GRBLStyle
 * 目前 Ender3_S 机型 是Marlin风格
 * 其他雕刻机选用 Grbl 风格
 */
typedef int8_t GCodeStyle;
enum GCodeStyle_ {
    DefaultStyle = 0,
    GRBLStyle = 1,
    MarlinStyle = 2,
    CV01Style = 3
};

typedef struct GCoreConfig {
    // 雕刻偏移位置(x, y)
    GcoPoint            offset;

    // 含义：雕刻密度(每毫米打印多少个点 * 10）：
    // 例子：density = 15 => 每毫米打印1.5个点，
    //范围10~100，默认值90（即每毫米打印9个点）
    int32_t             density;

    // 含义：雕刻功率(机器的功率比值 * 10, 改变G1的工作功率，通过控制主轴转速（S）实现）
    // 例子：假设机器最大功率S1000。则： power_rate = 505; => 最大S值为 1000 * (505 / 10)% = 1000 * 50.5% = 505
    //默认值800(即80%)，应设为截面传入(范围0 ~ 100%)
    int32_t             power_rate;


    // 含义: 雕刻速度(机器主轴移动的速度比值 * 10， 通过改变G1 主轴移动速度(F)实现）
    // 例子：假设机器最大速度F1000。则： speed_rate = 505； => 最大F值为 1000 * （505 / 10）% = 1000 * 50.5% = 505
    // 注意：
    //     1. 当work_speed被合理设置时，则不会使用speed_rate计算速度
    //     2. 当speed_rate < 0时将被弃用，改用默认值
    //默认值1000(即100%)
    int32_t             speed_rate;

    // 直接指定工作(G1)的速度, 小于或等于0或大于机型最大速率是被弃用，改为使用speed_rate或者默认值
    //默认值1800
    int32_t             work_speed;

    // 直接指定空走(G0)的速度
    //默认值3000
    int32_t             jog_speed;

    uint16_t            carving_cnt;  // 雕刻次数(1 - 2^16-1)，默认值1

    // 雕刻信息
    CarveStartPosition  start;      // 从图像的那个角点开始雕刻，默认值BottomLeft
    CarveDirection      dire;       // 雕刻路径和方式，默认值StraightLeft
    GCodeStyle          gco_style;  // 生成GCode风格，默认值MarlinStyle
    VectorData          vector_path;//矢量图片轮廓数据
    int16_t             vector_factor;//轮廓数据精度系数, 默认值10
    int16_t             platHeight; //平台高，单位pixel
    int16_t             extra_travel_distance_x_left; //X方向额外向左空走距离，单位mm*10,例子：extra_travel_distance_x_left = 50表示5mm
    int16_t             extra_travel_distance_x_right; //X方向额外向右空走距离，单位mm*10,例子：extra_travel_distance_x_right = 50表示5mm
    int16_t             cnc_work_deep; //雕刻深度，单位mm*10,例子：cnc_work_deep = 10表示1mm
    int16_t             cnc_jog_height; //雕刻深度，单位mm*10,例子：cnc_jog_height = 50表示5mm
    char*               laser_start_command;
    char*               laser_stop_command;
    int16_t             cur_command;
    int32_t             v2_offsetX;  //ender3 v2 X偏移量

    // 设备信息
    MachineModel        model;      // 机型
} GCoreConfig;

#pragma pack(pop)

#endif // GCORE_TYPE_H
