import QtQuick 2.13
import QtQuick.Controls 2.5

import "../qml"
import "../secondqml"
import "../cxcloud"

Button {
  id: root
  width: 40 * screenScaleFactor
  height: 40 * screenScaleFactor

  readonly property var cloudContext: standaloneWindow.cloudContext

  property bool logined: cxkernel_cxcloud.accountService.logined
  property string btnImgUrl: Constants.currentTheme ? "qrc:/UI/photo/menuImg/userImg_light.svg" : "qrc:/UI/photo/menuImg/userImg_dark.svg"

  function showLoginPanel() {
    cloudContext.loginDialog.show()
    cloudContext.loginDialog.raise()
  }

  function setCurrentServer(serverIndex) {
    cloudContext.loginDialog.setServerType(serverIndex)
  }

  onClicked: {
    if (logined) {
      cloudContext.userInfoDialog.show()
    } else {
      cloudContext.loginDialog.show()
    }
  }

  onLoginedChanged: {
    if (logined && cloudContext.loginDialog.visible) {
      if(cloudContext.loginDialog.callback) {
        cloudContext.loginDialog.callback()
        cloudContext.loginDialog.callback = null
      }
      cloudContext.loginDialog.visible = false
    }
  }

  background: Rectangle {
    color:  "transparent"
    border.color: Constants.themeGreenColor
    border.width: parent.hovered ? 1 : 0
    radius: 5* screenScaleFactor

    BaseCircularImage {
      anchors.centerIn: parent
      width: (logined ? 24 : 14) * screenScaleFactor
      height: (logined ? 24 : 16) * screenScaleFactor
      img_src: logined ? cxkernel_cxcloud.accountService.avatar : btnImgUrl
    }
  }
}
