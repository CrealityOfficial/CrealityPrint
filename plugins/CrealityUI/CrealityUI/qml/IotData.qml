pragma Singleton
import QtQuick 2.10

QtObject {
	//single
	property var serDeviceNameArray: []
	property var serIotIdMap: {0: 0}
	property var serProductkeyMap: {0: 0}
	property var serNickNameMap: {0: 0}
	property var serSyDidMap: {0: 0}
	property var serSyLecenseMap: {0: 0}
	property var serTbIdMap: {0: 0}
	property var serIotType: {0: 0}

	property var iotDeviceStatusMap: {0: 0}
	property var iotPrinterStatusMap: {0: 0}
	property var iotPrinterNameMap: {0: 0}
	property var iotTFCardStatusMap: {0: 0}
	property var iotShangYunAPILicenseMap: {0: 0}
	property var iotShangYunDIDStringMap: {0: 0}
	property var iotShangYunInitStringMap: {0: 0}
	property var iotPrintStateMap:{0: 0}
	property var iotVideoStatusMap: {0: 0}
	property var iotPrintProgressMap: {0: 0}
	property var iotNozzleTempMap:{0: 0}
	property var iotBedTempMap:{0: 0}
	property var iotCurFeedratePctMap:{0: 0}
	property var iotPrintedTimesMap:{0: 0}
	property var iotTimesLeftToPrintMap:{0: 0}
	property var iotNozzleTempTargetMap:{0: 0}
	property var iotBedTempTargetMap:{0: 0}
	property var iotCurFeedratePctTargetMap:{0: 0}
	property var iotLayerPrintMap:{0: 0}
	property var iotPrintIDMap:{0: 0}
	property var iotCurPositionMap:{0: 0}
	property var iotFanSwitchMap:{0: 0}
	property var iotNetIPMap:{0: 0}	


	property var videoItemMap: {0:0}


	//group

	//device list
	property var groupDeviceModelTypeArray: []
	//key seriesId
	property var groupSeriesIdMap: {0: 0}
	property var groupSeriesNameMap: {0: 0}
	property var groupDeviceCountMap: {0: 0}
	property var groupPrintingCntMap: {0: 0}
	property var groupVideoMap: {0: 0}




	//task list
	property var groupTaskDeviceModelTypeArray: []
	//key seriesId  
	property var groupTaskSeriesIdMap: {0: 0}
	property var groupTaskSeriesNameMap: {0: 0}
	property var groupTaskDeviceNumMap: {0: 0}
	property var groupTaskPrintingCntMap: {0: 0}
	property var groupTaskWaitingCntMap: {0: 0}
	property var groupTaskFinishCntMap: {0: 0}



	//child list
	property var groupDeviceModelTypeTaskMap: {0: 0}
	//key taskId
	property var groupChildSeriedIdMap: {0: 0}
	property var groupChildPrintIdMap: {0: 0}
	property var groupChildTaskPrintNameMap: {0: 0}
	property var groupChildTaskGcodeIdMap: {0: 0}
	property var groupChildTaskGcodeNameMap: {0: 0}
	property var groupChildTaskPrintPictureMap: {0: 0}
	property var groupChildTaskStatusMap: {0: 0}
	property var groupChildTaskPrintProgressMap: {0: 0}
	property var groupChildTaskPrintTimeMap: {0: 0}
	property var groupChildTaskPrintLeftTimeMap: {0: 0}
	property var groupChildDeviceNameMap: {0: 0}
	property var groupChildTaskTbIdMap: {0: 0}
	property var groupChildTaskVideoMap: {0: 0}
	property var groupChildTaskSyDidMap: {0: 0}
	property var groupChildTaskSyInitMap: {0: 0}
	property var groupChildTaskSyLicenseMap: {0: 0}

	property var taskListBtnStatus: "printing" // "waiting" ; "idling"
	property var taskListGcodeNameByDn: {0: 0}
	property var clusterDeviceWorkingCnt: 0
    property var clusterDeviceIdlingCnt: 0
    property var clusterTaskPrintingCnt: 0
    property var clusterTaskWaitingCnt: 0
	property var clusterTaskAllocatingCnt: 0

}
