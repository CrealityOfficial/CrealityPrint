import QtQuick 2.0
import QtQuick 2.10
import QtQuick.Controls 2.0
import QtQuick.Dialogs 1.3
Dialog
{
    width:500
    height:400
    title:"111";
    Component.onCompleted:
    {
        console.log("dialog finished!!!")
    }
}
