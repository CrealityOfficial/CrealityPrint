import QtQuick 2.15

import "../secondqml"

Item {
  property alias loginDialog: login_dialog
  property alias userInfoDialog: user_info_dialog
  // property alias modelLibarayDialog: model_library_dialog
  property alias downloadManageDialog: download_manage_dialog
  // property alias modelGroupUploadDialog: model_group_upload_dialog
  // property alias sliceFileUploadDialog: slice_file_upload_dialog
  property alias tipDialog: tip_dialog

  readonly property var dockContext: standaloneWindow.dockContext

  // --------- temp [begin] ---------
  property var modelLibarayDialog: null
  property var modelGroupUploadDialog: null
  property var sliceFileUploadDialog: null
  readonly property var model_library_dialog: modelLibarayDialog
  readonly property var model_group_upload_dialog: modelGroupUploadDialog
  readonly property var slice_file_upload_dialog: sliceFileUploadDialog
  // --------- temp [end] ---------

  LoginDialog {
    id: login_dialog
    objectName: "LoginDlg"
    tipDialog: tip_dialog
  }

  UserInfoDialog {
    id: user_info_dialog
    context: dockContext
    tipDialog: tip_dialog
    shareDialog: file_share_dialog
    downloadManageDialog: download_manage_dialog
  }

  // ModelLibraryDialog {
  //   id: model_library_dialog
  //   tipDialog: tip_dialog
  //   shareDialog: file_share_dialog
  //   licenseDialog: license_dialog
  //   downloadManageDialog: download_manage_dialog
  // }

  DownloadManageDialog {
    id: download_manage_dialog
    modelLibraryDialog: model_library_dialog
  }

  // LicenseDescriptionDialog {
  //   id: license_dialog
  // }

  FileShareDialog {
    id: file_share_dialog
    tipDialog: tip_dialog
  }

  // ModelGroupUploadDialog {
  //   id: model_group_upload_dialog
  //   tipDialog: tip_dialog
  //   userInfoDialog: user_info_dialog
  // }

  // SliceFileUploadDialog {
  //   id: slice_file_upload_dialog
  //   tipDialog: tip_dialog
  // }

  BasicMessageDialog {
    id: tip_dialog

    messageText: ""
    property var _receiver: null

    function exec(receiver) {
      if (!receiver) {
        return
      }

      _receiver = receiver

      messageText = _receiver.message

      if (typeof _receiver.onIgnored === "function") {
        showTripleBtn()
      } else if (typeof _receiver.onCanceled === "function") {
        showDoubleBtn()
      } else if (typeof _receiver.onAccepted === "function") {
        showSingleBtn()
      } else {
        showSingleBtn()
      }

      show()
    }

    function tipToLogin(accept_callback) {
      exec({
        message: qsTr("This operation cannot be completed without logging in to Creality Cloud. Do you want to log in?"),
        onAccepted: accept_callback
      })
    }

    function tipModelDownloaded(accept_callback) {
      exec({
        message: qsTr("The Model is in the download task list. Please check the download center."),
        onAccepted: _ => {
          if (accept_callback) {
            accept_callback()
          }

          download_manage_dialog.show()
        },
      })
    }

    function tipUploadSuccessed() {
      exec({ message: qsTr("Successfully uploaded to Creality Cloud") })
    }

    function tipUploadFailed() {
      exec({ message: qsTr("Upload failed") })
    }

    function tipCopyLinkSuccessed() {
      exec({ message: qsTr("Copy link successfully") })
    }

    function tipRestartAppliction(accept_callback, cancel_callback) {
      exec({
        message: qsTr("restart_appliction_tip"),
        onAccepted: accept_callback,
        onCanceled: cancel_callback,
        onClosed: cancel_callback,
      })
    }

    onAccept: {
      if (typeof _receiver.onAccepted === "function") {
        _receiver.onAccepted()
      }
    }

    onCancel: {
      if (typeof _receiver.onCanceled === "function") {
        _receiver.onCanceled()
      }
    }

    onIgnore: {
      if (typeof _receiver.onIgnored === "function") {
        _receiver.onIgnored()
      }
    }

    onDialogClosed: {
      if (typeof _receiver.onClosed === "function") {
        _receiver.onClosed()
      }
    }
  }
}
