import QtQuick 2.13

Item{
    property var items:[]//继续，取消，重试，ok
    property int errorCode
    property int errorLevel
    property int supportVersion
    property int machineState
    property bool isRepoPlrStatus: errorCode == 20000
    property bool isMaterialStatus: errorCode == 20010

    //    onErrorCodeChanged: {
    //        console.log("errorCode = ", errorCode)
    //        console.log("errorLevel = ", errorLevel)
    //        console.log("machineState = ", machineState)
    //    }

    //    Component.onCompleted: {
    //        console.log("state22 = ", state)
    //        console.log("errorCode = ", errorCode)
    //        console.log("errorLevel = ", errorLevel)
    //        console.log("machineState = ", machineState)
    //    }

    onStateChanged: {
        console.log("state11 = ", state)
        console.log("errorCode = ", errorCode)
        console.log("errorLevel = ", errorLevel)
    }

    states: [
        State {
            when: (errorCode >= 1 && errorCode <= 200) && supportVersion == 800
            name: "old1"
            PropertyChanges { target: items[0]; visible: true }
            PropertyChanges { target: items[1]; visible: true }
            PropertyChanges { target: items[2]; visible: false }
            PropertyChanges { target: items[3]; visible: false }
        },
        State {
            when: (errorCode >= 1 && errorCode <= 200) && supportVersion == 800
            name: "old2"
            PropertyChanges { target: items[0]; visible: true }
            PropertyChanges { target: items[1]; visible: true }
            PropertyChanges { target: items[2]; visible: false }
            PropertyChanges { target: items[3]; visible: false }
        },
        State {
            when: (errorCode >= 201 && errorCode <= 500) && supportVersion == 800
            name: "old3"
            PropertyChanges { target: items[0]; visible: false }
            PropertyChanges { target: items[1]; visible: false }
            PropertyChanges { target: items[2]; visible: false }
            PropertyChanges { target: items[3]; visible: true }
        },
        State {
            when: (isRepoPlrStatus ||isMaterialStatus) && supportVersion == 800
            name: "old4"
            PropertyChanges { target: items[0]; visible: true }
            PropertyChanges { target: items[1]; visible: true }
            PropertyChanges { target: items[2]; visible: false }
            PropertyChanges { target: items[3]; visible: false }
        },
        //        State {
        //            when: errorCode >= 1 && !(isRepoPlrStatus || isMaterialStatus)
        //            name: "old1"
        //            PropertyChanges { target: items[0]; visible: false }
        //            PropertyChanges { target: items[1]; visible: false }
        //            PropertyChanges { target: items[2]; visible: false }
        //            PropertyChanges { target: items[3]; visible: true }
        //        },
        State {
            when: (machineState == 1 && errorLevel == 1) && supportVersion == 830//运行&&严重
            name: "printing_errorSerious"
            PropertyChanges { target: items[0]; visible: false }
            PropertyChanges { target: items[1]; visible: false }
            PropertyChanges { target: items[2]; visible: true }
            PropertyChanges { target: items[3]; visible: true }
        },
        State {
            when: (machineState == 1 && errorLevel == 2) && supportVersion == 830//运行&&普通
            name: "printing_errorNormal"
            PropertyChanges { target: items[0]; visible: false }
            PropertyChanges { target: items[1]; visible: false }
            PropertyChanges { target: items[2]; visible: false }
            PropertyChanges { target: items[3]; visible: true }
        },
        State {
            when: (machineState == 1 && errorCode == 115) && supportVersion == 830//运行&&115
            name: "printing_special115"
            PropertyChanges { target: items[0]; visible: true }
            PropertyChanges { target: items[1]; visible: true }
            PropertyChanges { target: items[2]; visible: false }
            PropertyChanges { target: items[3]; visible: false }
        },
        State {
            when: (machineState == 1 && errorCode == 116) && supportVersion == 830//运行&&116
            name: "printing_special116"
            PropertyChanges { target: items[0]; visible: false }
            PropertyChanges { target: items[1]; visible: false }
            PropertyChanges { target: items[2]; visible: false }
            PropertyChanges { target: items[3]; visible: true }
        },
        State {
            when: (machineState != 1 && machineState != 4 && errorLevel == 1) && supportVersion == 830
            name: "printingSpecial_errorSerious"
            PropertyChanges { target: items[0]; visible: false }
            PropertyChanges { target: items[1]; visible: false }
            PropertyChanges { target: items[2]; visible: false }
            PropertyChanges { target: items[3]; visible: true }
        },
        State {
            when: (machineState != 1 && machineState != 4 && errorLevel == 2) && supportVersion == 830
            name: "printingSpecial_errorNormal"
            PropertyChanges { target: items[0]; visible: false }
            PropertyChanges { target: items[1]; visible: false }
            PropertyChanges { target: items[2]; visible: false }
            PropertyChanges { target: items[3]; visible: true }
        },
        State {
            when: (machineState != 1 && machineState != 4 && errorCode == 115) && supportVersion == 830
            name: "printingSpecial_special115"
            PropertyChanges { target: items[0]; visible: false }
            PropertyChanges { target: items[1]; visible: false }
            PropertyChanges { target: items[2]; visible: false }
            PropertyChanges { target: items[3]; visible: false }
        },
        State {
            when: (machineState != 1 && machineState != 4 && errorCode == 116) && supportVersion == 830
            name: "printingSpecial_special116"
            PropertyChanges { target: items[0]; visible: false }
            PropertyChanges { target: items[1]; visible: false }
            PropertyChanges { target: items[2]; visible: false }
            PropertyChanges { target: items[3]; visible: false }
        },
        State {
            when: (machineState == 4 && errorLevel == 1) && supportVersion == 830
            name: "printingNo_errorSerious"
            PropertyChanges { target: items[0]; visible: false }
            PropertyChanges { target: items[1]; visible: false }
            PropertyChanges { target: items[2]; visible: false }
            PropertyChanges { target: items[3]; visible: true }
        },
        State {
            when: (machineState == 4 && errorLevel == 2) && supportVersion == 830
            name: "printingNo_errorNormal"
            PropertyChanges { target: items[0]; visible: false }
            PropertyChanges { target: items[1]; visible: false }
            PropertyChanges { target: items[2]; visible: false }
            PropertyChanges { target: items[3]; visible: true }
        },
        State {
            when: (machineState == 4 && errorCode == 115) && supportVersion == 830
            name: "printingNo_special115"
            PropertyChanges { target: items[0]; visible: false }
            PropertyChanges { target: items[1]; visible: false }
            PropertyChanges { target: items[2]; visible: false }
            PropertyChanges { target: items[3]; visible: false }
        },
        State {
            when: (machineState == 4 && errorCode == 116) && supportVersion == 830
            name: "printingNo_special116"
            PropertyChanges { target: items[0]; visible: false }
            PropertyChanges { target: items[1]; visible: false }
            PropertyChanges { target: items[2]; visible: false }
            PropertyChanges { target: items[3]; visible: false }
        },
        State {
            when: supportVersion == -1
            name: "undefined"
            PropertyChanges { target: items[0]; visible: false }
            PropertyChanges { target: items[1]; visible: false }
            PropertyChanges { target: items[2]; visible: false }
            PropertyChanges { target: items[3]; visible: true }
        }
    ]
}
