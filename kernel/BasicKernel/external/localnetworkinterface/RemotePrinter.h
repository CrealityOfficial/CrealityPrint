#ifndef REMOTE_PRINTER_H
#define REMOTE_PRINTER_H

#include <QtCore/QString>
#include <QObject>
#include "RemotePrinterConstant.h"

struct RemotePrinter
{
	QString uniqueId;//设备唯一标识，有mac地址的用mac地址，对于octoprint和集群来说是服务器地址
	RemotePrinerType type;//远程打印机的类型
	QString ipAddress;
	QString macAddress;
	QString token;//用于octoprint
	QString deviceName;//设备（盒子、树莓派）名称
	QString productKey;//设备ID
	QString printerName;//打印机名称
	QString printFileName; //打印文件名称
	QString curPosition; //当前位置 例：X50 Y25.3 Z1.2
	QString autohome; //归位状态 例： X:0 Y:0 Z:0
	int printerStatus;//打印机状态 0：离线 1：在线 2: 串口异常
	int TFCardStatus; //TF卡状态 0：离线 1：在线
	int printState;//打印状态 0：空闲 1：打印中 2：打印完成 3：打印失败 4：停止打印 5：打印暂停
	int fanSwitchState; //风扇开关状态 0：关闭 1：开启
	int caseFanSwitchState; //腔体风扇开关状态 0：关闭 1：开启
	int auxiliaryFanSwitchState; //辅助风扇开关状态 0：关闭 1：开启
	int printProgress; //打印进度
	float nozzleTemp; //喷嘴当前温度
	float nozzleTempTarget; //喷嘴目标温度
	float bedTemp; //打印平台当前温度
	float bedTempTarget; //打印平台目标温度
	int printSpeed; //打印速度
	int printJobTime; //打印时间
	int printLeftTime; //打印剩余时间
	int devConnectType; //1:自动搜索  2:手动连接
	int autoHomeState; //1:完成状态  2:执行中状态
	int errorCode;
	int errorKey;
	int video; //0: 无视频连接  1:有视频连接
	int ledState; //0: off  1: on
	int layer; //当前打印层数
	int totalLayer; //总层数
	int printMode;//0:local 1:lan 2:cloud
	int moonrakerPort;
	int fluiddPort;
	int mainsailPort;
	int printerId; //多机序号
	int materialStatus=0;//断料续打状态
	int repoPlrStatus=0;//断电续打
	int nozzleCount = 1; //喷嘴数量
	float nozzle2Temp; //喷嘴2当前温度
	float nozzle2TempTarget; //喷嘴2目标温度
	float chamberTemp; //腔体当前温度
	float chamberTempTarget; //腔体目标温度
	int fanSpeedState; // 风扇速度
	int caseFanSpeedState; // 腔体风扇速度
	int auxiliaryFanSpeedState; // 辅助风扇速度
	float machineHeight;
	float machineWidth;
	float machineDepth;


	RemotePrinter()
	{
		uniqueId = "";
		type = RemotePrinerType::REMOTE_PRINTER_TYPE_NONE;
		ipAddress = "";
		macAddress = "";
		deviceName = "";
		productKey = "";
		printerName = "";
		printFileName = "";
		curPosition = "";
		printerStatus = 0;
		TFCardStatus = 0;
		printState = 0;
		fanSwitchState = 0;
		caseFanSwitchState = 0;
		auxiliaryFanSwitchState = 0;
		printProgress = 0;
		nozzleTemp = 0;
		nozzleTempTarget = 0;
		bedTemp = 0;
		bedTempTarget = 0;
		printSpeed = 0;
		printJobTime = 0;
		printLeftTime = 0;
		devConnectType = 0;
		autoHomeState = 0;
		errorCode = 0;
		errorKey = 0;
		video = 0;
		ledState = 0;
		layer = 0;
		totalLayer = 0;
		printMode = 0;
		moonrakerPort = 7125;
		fluiddPort = 80;
		mainsailPort = 8819;
		printerId = 0;
		materialStatus = 0;
		repoPlrStatus = 0;
		nozzle2Temp = 0;
		nozzle2TempTarget = 0;
		chamberTemp = 0;
		chamberTempTarget = 0;
		fanSpeedState = 0;
		caseFanSpeedState = 0;
		auxiliaryFanSpeedState = 0;
	}
};

Q_DECLARE_METATYPE(RemotePrinter)

#endif // REMOTE_PRINTER_H


