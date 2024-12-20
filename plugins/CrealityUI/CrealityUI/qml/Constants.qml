pragma Singleton
import QtQuick 2.10

QtObject {
    property var screenScaleFactor: 1
    property  int rightPanelWidth: 280
    property  int rightPanelHeight: 670
    property int rightPanelMaxWidth: 280
    //lisugui 2021-1-21
    property bool bLeftToolBarEnabled: true
    property bool bRightPanelEnabled: true
    property bool bMenuBarEnabled: true
    property bool bGLViewEnabled: true

    property bool bIsWizardShowing: false

    property bool bIsLaserSizeLoced: true

    property var languageType: 0

    property int leftShowType: 0  //0 :leftbar,1: models,2 : list  //控制左边栏的 互斥显示

    readonly property FontLoader mySystemFont: FontLoader {name:"Source Han Sans CN Normal"}

    property alias fontDirectory: directoryFontLoader.fontDirectory
    property alias relativeFontDirectory: directoryFontLoader.relativeFontDirectory
    property alias fontList: directoryFontLoader.fontList

    /* Edit this comment to add your custom font */
    readonly property font font: Qt.font({
                                             family: mySystemFont.name,
                                             pointSize: Qt.application.font.pointSize
                                         })
    readonly property font largeFont: Qt.font({
                                                  family: mySystemFont.name,
                                                  pointSize: Qt.application.font.pointSize * 1.6
                                              })

    readonly property string labelFontFamily: "Source Han Sans CN Normal"
    readonly property string labelFontFamilyMedium: "Source Han Sans CN Medium"
    readonly property string labelFontFamilyBold: "Source Han Sans CN Bold"
    readonly property string panelFontFamily: "Source Han Sans CN Normal"
    readonly property int labelFontWeight: Font.Normal

    readonly property int labelFontPointSize_6: Qt.platform.os === "windows" ? 6 : (Qt.platform.os === "linux" ? 6 : 8)
    readonly property int labelFontPointSize_8: Qt.platform.os === "windows" ? 8 : (Qt.platform.os === "linux" ? 8 : 10)
    readonly property int labelFontPointSize_9: Qt.platform.os === "windows" ? 9 : (Qt.platform.os === "linux" ? 9 : 11)
    readonly property int labelFontPointSize_10: Qt.platform.os === "windows" ? 10 : (Qt.platform.os === "linux" ? 10 : 12)
    readonly property int labelFontPointSize_11: Qt.platform.os === "windows" ? 11 : (Qt.platform.os === "linux" ? 11 : 13)
    readonly property int labelFontPointSize_12: Qt.platform.os === "windows" ?12 : (Qt.platform.os === "linux" ? 12 : 14)
    readonly property int labelFontPointSize_13: Qt.platform.os === "windows" ?13 : (Qt.platform.os === "linux" ? 13 : 15)
    readonly property int labelFontPointSize_14: Qt.platform.os === "windows" ?14 : (Qt.platform.os === "linux" ? 14 : 16)
    readonly property int labelFontPointSize_16: Qt.platform.os === "windows" ?16 : (Qt.platform.os === "linux" ? 16 : 18)
    readonly property int labelFontPointSize_18: Qt.platform.os === "windows" ?18 : (Qt.platform.os === "linux" ? 18 : 20)



    readonly property color themeGreenColor: "#17CC5F"

    readonly property int imageButtomPointSize: Qt.platform.os === "windows" ? 9 : 11
    //    property color backgroundColor: "#000000"
    //切片 功能模块界面 用到的字体颜色
    readonly property color disabledTextColor: "#a0a1a2"

    //modelGroup
    property string normal:"qrc:/UI/photo/modelGroup/normal.svg"
    property string normalD:"qrc:/UI/photo/modelGroup/normal_d.svg"
    property string normalRelief:"qrc:/UI/photo/modelGroup/normalRelief.svg"
    property string normalReliefD:"qrc:/UI/photo/modelGroup/normalRelief_d.svg"
    property string negative:"qrc:/UI/photo/modelGroup/negative.svg"
    property string negativeD:"qrc:/UI/photo/modelGroup/negative_d.svg"
    property string negativeRelief:"qrc:/UI/photo/modelGroup/negativeRelief.svg"
    property string negativeReliefD:"qrc:/UI/photo/modelGroup/negativeRelief_d.svg"
    property string modifier:"qrc:/UI/photo/modelGroup/modifier.svg"
    property string modifierD:"qrc:/UI/photo/modelGroup/modifier_d.svg"
    property string modifierRelief:"qrc:/UI/photo/modelGroup/modifierRelief.svg"
    property string modifierReliefD:"qrc:/UI/photo/modelGroup/modifierRelief_d.svg"
    property string supportDefender:"qrc:/UI/photo/modelGroup/supportDefender.svg"
    property string supportDefenderD:"qrc:/UI/photo/modelGroup/supportDefender_d.svg"
    property string supportGenerator:"qrc:/UI/photo/modelGroup/supportGenerator.svg"
    property string supportGeneratorD:"qrc:/UI/photo/modelGroup/supportGenerator_d.svg"

    //backgroundColor
    property color itemBackgroundColor: "#061F3B"//"#29292c"
    property color textRectBgHoveredColor: themeGreenColor  //输入框的hovercolor

    property color warningColor: "#FF115F"
    //常规字体颜色
    property color textColor: "#E3EBEE"
    property color textBackgroundColor: "#4B4B4B"
    property color unitColor : "#BCBBBB"
    //按钮的默认颜色
    property color itemChildBackgroundColor:"#383C3E"  //框内框的背景颜色
    property color buttonColor:"#535353" //"#061F3B" //"#5C6163"
    property color hoveredColor: "#3A3A3A"//"#1b9ebb"
    property color selectionColor:  "#214076" // "#42bdd8"
    property color rectBorderColor: "#909090" //"#333333"
    property color dialogBtnHoveredColor: "#5D5D5D"
    property color secondBtnHoverdColor: "#666666"
    property color secondBtnSelectionColor: "#36B785"
    //slider color
    property color grooveColor: "#5E5E64"
    property color handleColor: "#42BDD8"
    property color handleBorderColor: "#5E5E64"

    //MAIN background
    property color mainBackgroundColor : "#363638"

    property color dropShadowColor: Qt.rgba(0, 0, 0, 0.1)
    //cmb
    property color cmbPopupColor: "#E1E1E1"
    property color cmbPopupColor_pressed : "#9CE8F4"//"#3AC2D7"

    property color cmbIndicatorRectColor : "#B0B0B0"
    property color cmbIndicatorRectColor_pressed : "#D7D7D7"

    property color cmbIndicatorRectColor_pressed_basic : "#5F5F62"
    property color cmbIndicatorRectColor_basic: "transparent"
    //tree
    property color treeBackgroundColor: "#424242"
    property color treeItemColor:"#424242"
    property color treeItemColor_pressed: "#666666"
    //lefttoolbar
    property color leftBtnBgColor_normal:  "#040405"
    property color leftBtnBgColor_hovered: "#6D6D71"
    property color leftBtnBgColor_selected: "#17CC5F"
    property color leftToolBtnColor_normal: "#6e6e73"
    property color leftToolBtnColor_hovered: "#8a8a8a"
    property color leftTextColor: "#C3C3C3"
    property color leftTextColor_d: "white"

    property color leftBarBgColor: "#2B2B2D"
    property color leftTabBgColor: "#505052"

    //topbar
    property color topBtnBgColor_normal:  "#1e9be2"
    property color topBtnBgColor_hovered: "#212122"
    property color topBtnBorderColor_normal: "transparent"
    property color topBtnBordeColor_hovered: "transparent"
    property color topBtnTextColor_normal : "#b9b9c0"
    property color topBtnTextColor_hovered : "#FFFFFF"
    property color topBtnTextColor_checked : "#FFFFFF"
    //dialog
    property color dialogTitleColor : "#373737"
    property color dialogTitleTextColor : "white"
    property color dialogItemRectBgColor : "#4B4B4B"
    property color dialogItemRectBgBorderColor : "#6E6E72"
    property color aboutDiaItemRectBgColor : "#535353"
    property color dialogContentBgColor : "#535353"
    property color dialogBorderColor : "#262626"
    //tabBtn
    property color tabButtonSelectColor: "#1e9be2"
    property color tabButtonNormalColor: "#535353"

    //menuBar title
    property color headerBackgroundColor: "#67686c"
    property color headerBackgroundColorEnd: "#67686c"
    property color topToolBarColor:"#F2F2F5"
    property color topToolBarColorEnd:"#F2F2F5"
    property color menuBarBtnColor: "#333333"
    property color menuBarBtnColor_hovered: "#3E3E3E"
    property color menuBarBtnColor_border: "#4D4D4D"
    property color menuStyleBgColor: "white"
    property color menuStyleBgColor_hovered: "#17CC5F"
    property color menuTextColor: "white"
    property color menuTextColor_hovered: "white"
    property color menuTextColor_normal: "white"

    //search btn
    property color searchBtnDisableColor : "#ECECEC"
    property color searchBtnNormalColor : "#1E9BE2"
    property color searchBtnHoveredColor : "#1EB6E2"

    property color typeBtnSelectedColor : "#1E9BE2"
    property color typeBtnHoveredColor : "#666666"
    property color splitLineColor : "#666666"
    property color splitLineColorDark : "#444444"
    property color radioCheckLabelColor: "white"
    property color radioCheckRadiusColor: "#ffffff"
    property color radioInfillBgColor: "#00a3ff"
    property color radioBorderColor: "#333333"
    property color profileBtnColor: "#6e6e73"
    property color profileBtnHoverColor: "#8a8a8a"//"#C2C2C5"
    property color tooltipBgColor: "#454545"
    property color checkBoxBgColor: "#424242"
    property color checkBoxBorderColor: "#6e6e72"
    property color menuSplitLineColor: "#0D0D0D"
    property color menuSplitLineColorleft: "#303030"
    property color mainViewSplitLineColor: "#0D0D0D"
    property color printerCombRectColor: "#D7D7D7"
    property color sliderTopColor1: "#4A4A4A"
    property color sliderTopColor2: "#767676"
    property color sliderBottomColor: "#535353"
    property color progressBarBgColor: "#303030"
    property color previewPanelRecgColor: "black"
    property color previewPanelTextgColor: "#CDD2D7"

    property color switchModelBgColor: "#181818"
    property color switchModeSelectedlBgColor: "#535353"
    property color modelAlwaysMoreBtnBgColor: "#D7D7D7"
    property color modelAlwaysItemBorderColor: "#262626"

    property color cube3DAmbientColor: "#828282"
    property color cube3DAmbientColor_Entered:"#E5E0D0"

    property color textColor_disabled:"#AFAFAF"
    property color printerCommboxBtnDefaultColor: "#B7B7B7"
    property color printerCommboxPopBgColor: "#E1E1E1"
    property color printerCommboxIndicatorBgColor: "#666666"
    property color printerCommboxIndicatorPopShowBgColor: "#E1E1E1"
    property color printerCommboxBgColor: "#181818"
    property color printerCommboxBgBorderColor: "#333333"

    property color tittleBarBtnColor: "#3E3E3E"
    property color laserFoldTittleColor: "#414143"


    property color enabledBtnShadowColor:"black"

    property color laserLineBorderColor:"#999999"

    //spinbox
    property string upBtnImgSource: "qrc:/UI/photo/upBtn.svg"
    property string upBtnImgSource_d: "qrc:/UI/photo/upBtn_d.svg"

    property string rightBtnImgSource: "qrc:/UI/photo/rightDrawer/right_btn.svg"

    property string downBtnImgSource: "qrc:/UI/photo/downBtn.svg"
    property string downBtnImgSource_d: "qrc:/UI/photo/downBtn_d.svg"


    property string clearBtnImg: "qrc:/UI/photo/clearBtn.png"
    property string clearBtnImg_d: "qrc:/UI/photo/clearBtn_d.png"

    property string searchBtnImg: "qrc:/UI/photo/search.png"
    property string searchBtnImg_d: "qrc:/UI/photo/search_d.png"

    property string downBtnImg: "qrc:/UI/photo/downBtn.svg"
    property string checkBtnImg: "qrc:/UI/images/check2.png"

    property string fold_light: "qrc:/UI/photo/rightDrawer/update/fold_light_default.svg"
    property string expand_light: "qrc:/UI/photo/rightDrawer/update/expand_light_default.svg"

    property string showPWNormalImg: "qrc:/UI/photo/showPW_dark.png"
    property string hidePWNormalImg: "qrc:/UI/photo/hidePW_dark.png"
    property string showPWHoveredImg: "qrc:/UI/photo/showPW_h_dark.png"
    property string hidePWHoveredImg: "qrc:/UI/photo/hidePW_h_dark.png"
    property string areaCodeComboboxImg: "qrc:/UI/photo/comboboxDown.png"

    property string configTabBtnImg: "qrc:/UI/photo/configTabBtn.png"
    property string configTabBtnImg_Hovered: "qrc:/UI/photo/configTabBtn_d.png"

    property string supportTabBtnImg:"qrc:/UI/photo/supportTabBtn.png"
    property string supportTabBtnImg_Hovered:"qrc:/UI/photo/supportTabBtn_d.png"

    property string homeImg: "qrc:/UI/images/home.svg"
    property string homeImg_Hovered: "qrc:/UI/images/home_s.svg"

    property string laserPickImg: "qrc:/UI/images/laser_pick.png"
    property string laserImageImg: "qrc:/UI/images/laser_img.png"
    property string laserFontImg: "qrc:/UI/images/laser_font.png"
    property string laserRectImg: "qrc:/UI/images/laser_rect.png"
    property string laserArcImg: "qrc:/UI/images/laser_arc.png"
    property string laserPickHoveredImg: "qrc:/UI/images/laser_pick2.png"
    property string laserImageHoveredImg: "qrc:/UI/images/laser_img2.png"
    property string laserFontHoveredImg: "qrc:/UI/images/laser_font2.png"
    property string laserRectHoveredImg: "qrc:/UI/images/laser_rect2.png"
    property string laserArcHoveredImg: "qrc:/UI/images/laser_arc2.png"
    property string laserPickCheckedImg: "qrc:/UI/images/laser_pick_s.png"
    property string laserImageCheckedImg: "qrc:/UI/images/laser_img_s.png"
    property string laserFontCheckedImg: "qrc:/UI/images/laser_font_s.png"
    property string laserRectCheckedImg: "qrc:/UI/images/laser_rect_s.png"
    property string laserArcCheckedImg: "qrc:/UI/images/laser_arc_s.png"

    property var switchModelFDMImg: "qrc:/UI/images/fdmMode.png"
    property var switchModelFDMImg_H: "qrc:/UI/images/fdmMode_h.png"
    property var switchModelLaserImg: "qrc:/UI/images/laserMode.png"
    property var switchModelLaserImg_H: "qrc:/UI/images/laserMode_h.png"
    property var switchModelLaserImgEn: "qrc:/UI/images/laserMode_en.png"
    property var switchModelLaserImgEn_H: "qrc:/UI/images/laserMode_h_en.png"
    property var switchModelCNCImg: "qrc:/UI/images/CNCMode.png"
    property var switchModelCNCImg_H: "qrc:/UI/images/CNCMode_h.png"

    property var printMoveAxisYUpImg: "qrc:/UI/photo/print_move_axis_y+.png"
    property var printMoveAxisYUp_HImg: "qrc:/UI/photo/print_move_axis_y+_h.png"
    property var printMoveAxisYUp_CImg: "qrc:/UI/photo/print_move_axis_y+_c.png"
    property var printMoveAxisZUpImg: "qrc:/UI/photo/print_move_axis_z+.png"
    property var printMoveAxisZUp_HImg: "qrc:/UI/photo/print_move_axis_z+_h.png"
    property var printMoveAxisZUp_CImg: "qrc:/UI/photo/print_move_axis_z+_c.png"
    property var printMoveAxisXDownImg: "qrc:/UI/photo/print_move_axis_x-.png"
    property var printMoveAxisXDown_HImg: "qrc:/UI/photo/print_move_axis_x-_h.png"
    property var printMoveAxisXDown_CImg: "qrc:/UI/photo/print_move_axis_x-_c.png"
    property var printMoveAxisZeroImg: "qrc:/UI/photo/print_move_axis_zero.png"
    property var printMoveAxisZero_HImg: "qrc:/UI/photo/print_move_axis_zero_h.png"
    property var printMoveAxisZero_CImg: "qrc:/UI/photo/print_move_axis_zero_c.png"
    property var printMoveAxisXUpImg: "qrc:/UI/photo/print_move_axis_x+.png"
    property var printMoveAxisXUp_HImg: "qrc:/UI/photo/print_move_axis_x+_h.png"
    property var printMoveAxisXUp_CImg: "qrc:/UI/photo/print_move_axis_x+_c.png"
    property var printMoveAxisYDownImg: "qrc:/UI/photo/print_move_axis_y-.png"
    property var printMoveAxisYDown_HImg: "qrc:/UI/photo/print_move_axis_y-_h.png"
    property var printMoveAxisYDown_CImg: "qrc:/UI/photo/print_move_axis_y-_c.png"
    property var printMoveAxisZDownImg: "qrc:/UI/photo/print_move_axis_z-.png"
    property var printMoveAxisZDown_HImg: "qrc:/UI/photo/print_move_axis_z-_h.png"
    property var printMoveAxisZDown_CImg: "qrc:/UI/photo/print_move_axis_z-_c.png"

    property var modelAlwaysPopBg: "qrc:/UI/photo/model_always_pop_bg.png"
    property var modelAlwaysBtnIcon: "qrc:/UI/photo/model_always_show.png"
    property var uploadModelImg: "qrc:/UI/photo/upload_model_img.png"
    property var uploadLocalModelImg: "qrc:/UI/photo/upload_model_local_img.png"
    property var deleteModelImg: "qrc:/UI/photo/delete_model_img.png"

    property var editProfileImg: "qrc:/UI/photo/editProfile.png"
    property var uploadProfileImg: "qrc:/UI/photo/uploadPro.png"
    property var deleteProfileImg: "qrc:/UI/photo/deleteProfile.png"

    property var onSrcImg: "qrc:/UI/photo/on.png"
    property var offSrcImg: "qrc:/UI/photo/off.png"
    property var checkedSrcImg: "qrc:/UI/photo/radio_1.png"
    property var uncheckSrcImg: "qrc:/UI/photo/radio_2.png"

    property var cube3DTopImg : "qrc:/UI/images/top.png"
    property var cube3DTopImg_C: "qrc:/UI/images/top_C.png"
    property var cube3DFrontImg: "qrc:/UI/images/front.png"
    property var cube3DFrontImg_C: "qrc:/UI/images/front_C.png"
    property var cube3DBottomImg: "qrc:/UI/images/bottom.png"
    property var cube3DBottomImg_C: "qrc:/UI/images/bottom_C.png"
    property var cube3DBackImg: "qrc:/UI/images/back.png"
    property var cube3DBackImg_C: "qrc:/UI/images/back_C.png"
    property var cube3DLeftkImg: "qrc:/UI/images/left.png"
    property var cube3DLeftkImg_C: "qrc:/UI/images/left_C.png"
    property var cube3DLeftkRight: "qrc:/UI/images/right.png"
    property var cube3DLeftkRight_C: "qrc:/UI/images/right_C.png"

    property var laserFoldTitleUpImg: "qrc:/UI/photo/laser_fold_item_up.png"
    property var laserFoldTitleDownImg: "qrc:/UI/photo/laser_fold_item_down.png"

    // ---------- left panel [beg] ----------
    property color left_model_list_button_default_color: "#363638"
    property color left_model_list_button_hovered_color: "#6D6D71"
    property color left_model_list_button_border_default_color: "#56565C"
    property color left_model_list_button_border_hovered_color: "#6D6D71"
    property string left_model_list_button_default_icon: "qrc:/UI/photo/modelLIstIcon.svg"
    property string left_model_list_button_hovered_icon: "qrc:/UI/photo/modelLIstIcon_h.svg"
    property color left_model_list_count_background_color: "#FFFFFF"
    property color left_model_list_count_text_color: "#363638"

    property color left_model_list_background_color: "#363638"
    property color left_model_list_border_color: "#56565C"

    property color left_model_list_title_text_color: "#FFFFFF"

    property color left_model_list_close_button_default_color: "transparent"
    property color left_model_list_close_button_hovered_color: "#FF365C"
    property string left_model_list_close_button_default_icon: "qrc:/UI/photo/closeBtn.svg"
    property string left_model_list_close_button_hovered_icon: "qrc:/UI/photo/closeBtn_d.svg"

    property color left_model_list_all_button_border_color: "#1E9BE2"
    property color left_model_list_all_button_text_color: "#FFFFFF"
    property string left_model_list_all_button_icon: "qrc:/UI/images/check2.png"

    property color left_model_list_action_button_default_color: "transparent"
    property color left_model_list_action_button_hovered_color: "#262626"
    property string left_model_list_del_button_default_icon: "qrc:/UI/photo/deleteModel_dark.png"
    property string left_model_list_del_button_hovered_icon: "qrc:/UI/photo/deleteModel_dark.png"

    property color left_model_list_item_default_color: "transparent"
    property color left_model_list_item_hovered_color: "#739AB0"
    property color left_model_list_item_text_default_color: "#CBCBCB"
    property color left_model_list_item_text_hovered_color: "#FFFFFF"
    // ---------- left panel [end] ----------

    // ---------- right panel [beg] ----------

    property color right_panel_text_default_color         : "#FFFFFF"
    property color right_panel_text_disable_color         : "#FFFFFF"
    property color right_panel_text_hovered_color         : "#FFFFFF"
    property color right_panel_text_checked_color         : "#FFFFFF"

    property color right_panel_gcode_text_color           : "#92929B"

    property color right_panel_button_default_color       : "#4B4B4D"
    property color right_panel_button_disable_color       : "#4B4B4D"
    property color right_panel_button_hovered_color       : "#414143"
    property color right_panel_button_checked_color       : "#414143"

    property color right_panel_border_default_color       : "#6C6C70"
    property color right_panel_border_disable_color       : "#6C6C70"
    property color right_panel_border_hovered_color       : "#17CC5F"
    property color right_panel_border_checked_color       : "#17CC5F"

    property color right_panel_item_default_color         : "#414143"
    property color right_panel_item_disable_color         : "#414143"
    property color right_panel_item_hovered_color         : "#5F5F5F"
    property color right_panel_item_checked_color         : "#739AB0"
    property color right_panel_item_text_default_color    : "#CBCBCB"
    property color right_panel_item_text_disable_color    : "#CBCBCB"
    property color right_panel_item_text_hovered_color    : "#CBCBCB"
    property color right_panel_item_text_checked_color    : "#FFFFFF"

    property color right_panel_combobox_background_color  : "#414143"

    property color right_panel_tab_text_default_color     : "#8D8D91"
    property color right_panel_tab_text_disable_color     : "#8D8D91"
    property color right_panel_tab_text_hovered_color     : "#8D8D91"
    property color right_panel_tab_text_checked_color     : "#FFFFFF"
    property color right_panel_tab_button_default_color   : "#1C1C1D"
    property color right_panel_tab_button_disable_color   : "#1C1C1D"
    property color right_panel_tab_button_hovered_color   : "#1C1C1D"
    property color right_panel_tab_button_checked_color   : "#1E9BE2"
    property color right_panel_tab_background_color       : "#1C1C1D"

    property color right_panel_menu_border_color          : "#1C1C1D"
    property color right_panel_menu_background_color      : "#4B4B4D"
    property color right_panel_menu_split_line_color      : "#3B3B3D"

    property color right_panel_slice_button_default_color : "#1E9BE2"
    property color right_panel_slice_button_disable_color : "#626265"
    property color right_panel_slice_button_hovered_color : "#1E9BE2"
    property color right_panel_slice_button_checked_color : "#1E9BE2"
    property color right_panel_slice_text_default_color   : "#B1B1B7"
    property color right_panel_slice_text_disable_color   : "#B1B1B7"
    property color right_panel_slice_text_hovered_color   : "#B1B1B7"
    property color right_panel_slice_text_checked_color   : "#FFFFFF"

    property color right_panel_bgColor: "#252525"

    property string right_panel_quality_custom_default_image  : "qrc:/UI/photo/config_quality_custom_default.png"
    property string right_panel_quality_custom_checked_image  : "qrc:/UI/photo/config_quality_custom_checked.png"
    property string right_panel_quality_high_default_image    : "qrc:/UI/photo/config_quality_high_default.png"
    property string right_panel_quality_high_checked_image    : "qrc:/UI/photo/config_quality_high_checked.png"
    property string right_panel_quality_middle_default_image  : "qrc:/UI/photo/config_quality_middle_default.png"
    property string right_panel_quality_middle_checked_image  : "qrc:/UI/photo/config_quality_middle_checked.png"
    property string right_panel_quality_low_default_image     : "qrc:/UI/photo/config_quality_low_default.png"
    property string right_panel_quality_low_checked_image     : "qrc:/UI/photo/config_quality_low_checked.png"
    property string right_panel_quality_verylow_default_image : "qrc:/UI/photo/config_quality_verylow_default.png"
    property string right_panel_quality_verylow_checked_image : "qrc:/UI/photo/config_quality_verylow_checked.png"

    property string right_panel_process_level_image: "qrc:/UI/photo/rightDrawer/process_level_dark.svg"
    property string right_panel_process_custom_image: "qrc:/UI/photo/rightDrawer/process_custom_dark.svg"

    property string right_panel_delete_image: "qrc:/UI/photo/rightDrawer/delete.svg"
    property string right_panel_delete_hovered_image: "qrc:/UI/photo/rightDrawer/delete_hovered.svg"
    property string right_panel_edit_image: "qrc:/UI/photo/rightDrawer/edit.svg"
    property string right_panel_edit_hovered_image: "qrc:/UI/photo/rightDrawer/edit_hovered.svg"
    property string right_panel_save_image: "qrc:/UI/photo/rightDrawer/save_dark.svg"
    property string right_panel_save_disabled_image: "qrc:/UI/photo/rightDrawer/save_dark_disabled.svg"
    property string right_panel_reset_image: "qrc:/UI/photo/rightDrawer/reset_dark.svg"
    property string right_panel_reset_disabled_image: "qrc:/UI/photo/rightDrawer/reset_dark_disabled.svg"

    // ---------- right panel [end] ----------

    //lanPrinter Panel
    property color lanPrinter_panel_border: "transparent"
    property color lanPrinter_panel_light_txt: "#7A7A82"
    property color lanPrinter_panel_weight_txt: "#FFFFFF"
    property color lanPrinter_panel_background: "#171718"
    property color lanPrinter_panel_btn_default: "#2E2E30"
    property color lanPrinter_panel_btn_hovered: "#414143"

    //new
    property color themeColor_New: "#17cc5f"

    //leftPopWin
    property color darkThemeColor_primary: "#4b4b4d"
    property color darkThemeColor_secondary: "#6e6e73"
    property color darkThemeColor_third: "#6e6e73"

    property color lightThemeColor_primary: "#ffffff"
    property color lightThemeColor_secondary: "#dddde1"
    property color lightThemeColor_third: "#999999"

    property color themeTextColor: "#ffffff"
    property color themeColor_primary: darkThemeColor_primary
    property color themeColor_secondary: darkThemeColor_secondary
    property color themeColor_third: darkThemeColor_third

    property color lpw_bgColor: themeColor_primary
    property color lpw_titleColor: themeColor_third
    property color lpw_textColor: themeTextColor

    property color lpw_spinColor: themeColor_secondary
    property color lpw_spinHoverColor: themeColor_primary
    property color lpw_spinClickedColor: themeColor_primary

    property color lpw_spinIndectorBgColor: themeColor_secondary
    property color lpw_spinIndectorBgHoverColor: themeColor_primary

    property color lpw_spinBorderColor: themeColor_primary
    property color lpw_spinBorderHoverColor: themeColor_primary

    property color lpw_BtnColor: "#8a8a8a"//themeColor_third
    property color lpw_BtnHoverColor: profileBtnHoverColor//themeColor_secondary

    property color lpw_BtnBorderColor: themeColor_secondary
    property color lpw_BtnBorderHoverColor: themeColor_secondary

    property string addModel: "qrc:/UI/photo/addModel_dark.png"
    property string delModel: "qrc:/UI/photo/deleteModel_dark.png"
    property string drawerBgImg: "qrc:/UI/photo/rightDrawer/drawerBg.svg"
    //_leftPopWin

    // ---------- dock [beg] ----------
    property color dock_context_tab_bar_color                : "#29292A"
    property color dock_context_tab_button_default_color     : "#2E2E30"
    property color dock_context_tab_button_checked_color     : "#414143"
    property color dock_context_tab_button_text_default_color: "#7A7A82"
    property color dock_context_tab_button_text_checked_color: "#FFFFFF"
    property color dock_border_color                         : "#262626"
    // ---------- dock [end] ----------

    // ---------- model library [beg] ----------
    property color model_library_border_color                      : "#67676D"

    property color model_library_type_button_default_color         : "transparent"
    property color model_library_type_button_checked_color         : "#1E9BE2"
    property color model_library_type_button_text_default_color    : "#CBCBCC"
    property color model_library_type_button_text_checked_color    : "#FFFFFF"

    property color model_library_back_button_default_color         : "transparent"
    property color model_library_back_button_checked_color         : "#1E9BE2"
    property color model_library_back_button_border_default_color  : "#67676D"
    property color model_library_back_button_border_checked_color  : "#1E9BE2"
    property color model_library_back_button_text_default_color    : "#BABABA"
    property color model_library_back_button_text_checked_color    : "#FFFFFF"
    property string model_library_back_button_default_image        : "qrc:/UI/photo/model_library_detail_back_h.png"
    property string model_library_back_button_checked_image        : "qrc:/UI/photo/model_library_detail_back.png"

    property color model_library_action_button_default_color       : "transparent"
    property color model_library_action_button_checked_color       : "#1E9BE2"
    property color model_library_action_button_border_default_color: "#67676D"
    property color model_library_action_button_border_checked_color: "#1E9BE2"
    property color model_library_action_button_text_default_color  : "#BABABA"
    property color model_library_action_button_text_checked_color  : "#FFFFFF"

    property color model_library_import_button_default_color       : "#1E9BE2"
    property color model_library_import_button_checked_color       : "#1EB6E2"

    property color model_library_special_text_color                : "#FFFFFF"
    property color model_library_general_text_color                : "#BABABA"

    property color model_library_item_default_color                : "transparent"
    property color model_library_item_hovered_color                : "#5F5F62"
    property color model_library_item_checked_color                : "#1E9BE2"
    property color model_library_item_text_default_color           : "#FFFFFF"
    property color model_library_item_text_checked_color           : "#FFFFFF"

    property color model_library_license_default_color             : "#1E9BE2"
    property color model_library_license_checked_color             : "#1EB6E2"
    property string model_library_license_default_image            : "qrc:/UI/photo/model_library_license_h.png"
    property string model_library_license_checked_image            : "qrc:/UI/photo/model_library_license_h.png"

    property string model_library_import_default_image             : "qrc:/UI/photo/model_library_import.png"
    property string model_library_import_checked_image             : "qrc:/UI/photo/model_library_import_h.png"
    property string model_library_download_default_image           : "qrc:/UI/photo/model_library_download.png"
    property string model_library_download_checked_image           : "qrc:/UI/photo/model_library_download_h.png"
    property string model_library_uncollect_default_image          : "qrc:/UI/photo/model_library_uncollect.png"
    property string model_library_uncollect_checked_image          : "qrc:/UI/photo/model_library_uncollect_h.png"
    property string model_library_shared_default_image             : "qrc:/UI/photo/model_library_share.png"
    property string model_library_shared_checked_image             : "qrc:/UI/photo/model_library_share_h.png"
    // ---------- model library [end] ----------

    // ---------- custom tabview [beg] ----------
    property color custom_tabview_border_color             : "#2B2B2B"
    property color custom_tabview_panel_color              : "#414143"
    property color custom_tabview_button_color             : "#4B4B4D"
    property color custom_tabview_button_text_default_color: "#A5A5AE"
    property color custom_tabview_button_text_checked_color: "#FFFFFF"
    property color custom_tabview_combo_item_hovered_color : "#739AB0"
    readonly property font custom_tabview_button_text_font : font
    // ---------- custom tabview [end] ----------

    // ---------- manager printer dialog [beg] ----------
    property color manager_printer_switch_default_color: "#FFFFFF"
    property color manager_printer_switch_checked_color: "#00A3FF"
    property string manager_printer_switch_default_image: "qrc:/UI/photo/printer_switch_dark_default.png"
    property string manager_printer_switch_checked_image: "qrc:/UI/photo/printer_switch_dark_checked.png"

    property color manager_printer_tabview_default_color: "#CBCBCC"
    property color manager_printer_tabview_checked_color: "#FFFFFF"

    property color manager_printer_label_color: "#CBCBCC"

    property color manager_printer_gcode_title_color: "#CBCBCC"
    property color manager_printer_gcode_text_color: "#FFFFFF"

    property color manager_printer_button_text_color: "#DBDBDC"
    property color manager_printer_button_border_color: "transparent"
    property color manager_printer_button_default_color: "#6E6E73"
    property color manager_printer_button_checked_color: "#86868A"
    // ---------- manager printer dialog [end] ----------

    // ---------- download manage dialig [beg] ----------
    property string downloadbtn_image: "qrc:/UI/photo/download_btn_dark.png"
    property string downloadbtn_image_hovered: "qrc:/UI/photo/download_btn_dark.png"
    property string downloadbtn_tip_image: "qrc:/UI/photo/download_btntip_dark.png"
    property color downloadbtn_tip_color: "#FFFFFF"

    property color downloadbtn_finished_default_color: "transparent"
    property color downloadbtn_finished_hovered_color: "#3E3E3E"
    property color downloadbtn_finished_text_hovered_color: "#B9B9C0"
    property color downloadbtn_finished_text_default_color: "#B9B9C0"
    property color downloadbtn_finished_border_default_color: "transparent"
    property color downloadbtn_finished_border_hovered_color: "#3E3E3E"

    property color downloadbtn_download_default_color: "#FFFFFF"
    property color downloadbtn_download_hovered_color: "#FFFFFF"
    property color downloadbtn_download_text_default_color: "#000000"
    property color downloadbtn_download_text_hovered_color: "#000000"
    property color downloadbtn_download_border_default_color: "#3E3E3E"
    property color downloadbtn_download_border_hovered_color: "#1E9BE2"

    property color download_manage_tab_button_color: "#4B4B4D"
    property color download_manage_title_text_color: "#CBCBCC"
    property color download_manage_model_text_color: "#FFFFFF"
    property color download_manage_prograss_left_color: "#1E9BE2"
    property color download_manage_prograss_right_color: "#3C3C3E"
    property string download_manage_group_open_image: "qrc:/UI/photo/treeView_plus_dark.png"
    property string download_manage_group_close_image: "qrc:/UI/photo/treeView_minus_dark.png"

    property string download_manage_empty_button_image: "qrc:/UI/photo/cloud_logo.svg"
    property color download_manage_empty_text_color: "#DBDBDC"
    property color download_manage_empty_button_default_color: "#6E6E73"
    property color download_manage_empty_button_checked_color: "#86868A"
    property color download_manage_empty_button_text_default_color: "#FFFFFF"
    property color download_manage_empty_button_text_checked_color: "#FFFFFF"
    // ---------- download manage dialig [end] ----------

    // ---------- left tool bar [end] ----------
    property string leftbar_rocommand_btn_icon_default: "qrc:/UI/photo/leftBar/recommand_dark.svg"
    property string leftbar_rocommand_btn_icon_hovered: "qrc:/UI/photo/leftBar/recommand_pressed.svg"
    property string leftbar_rocommand_btn_icon_pressed: "qrc:/UI/photo/leftBar/recommand_pressed.svg"
    property string leftbar_open_btn_icon_default: "qrc:/UI/photo/leftBar/open_dark.svg"
    property string leftbar_open_btn_icon_hovered: "qrc:/UI/photo/leftBar/open_pressed.svg"
    property string leftbar_open_btn_icon_pressed: "qrc:/UI/photo/leftBar/open_pressed.svg"
    property string leftbar_pick_btn_icon_default: "qrc:/UI/photo/leftBar/pick_dark.svg"
    property string leftbar_pick_btn_icon_hovered: "qrc:/UI/photo/leftBar/pick_pressed.svg"
    property string leftbar_pick_btn_icon_pressed: "qrc:/UI/photo/leftBar/pick_pressed.svg"
    property string leftbar_other_btn_icon_default: "qrc:/UI/photo/leftBar/other_dark.svg"
    property string leftbar_other_btn_icon_hovered: "qrc:/UI/photo/leftBar/other_hovered_dark.svg"
    property string leftbar_other_btn_icon_checked: "qrc:/UI/photo/leftBar/other_checked.svg"
    property string leftbar_btn_border_color: "#2B2B2D"
    // ---------- left tool bar [end] ----------

    // ---------- basic compenent new [beg] ----------
    property color switch_border_color: "#343434"
    property color switch_indicator_color: "#17CC5F"
    property color switch_background_color: "#343434"
    property color switch_indicator_text_color: "#FFFFFF"
    property color switch_background_text_color: "#C9C9C9"
    // ---------- basic compenent new [end] ----------

    // ---------- parameter [beg] ----------
    property color parameter_text_color: "#C5C5CA"
    property color parameter_text_modifyed_color: "#FFD200"
    property color parameter_group_text_color: "#FFFFFF"
    property color parameter_unit_text_color: "#A5A5AE"
    property color parameter_editer_modifyed_color: "#787565"
    property string parameter_reset_button_image: "qrc:/UI/photo/parameter/reset_dark.svg"
    // ---------- parameter [end] ----------

    property DirectoryFontLoader directoryFontLoader: DirectoryFontLoader {
        id: directoryFontLoader
    }

    property string imagePathPrefix: "file:///./photo/"

    //new color
    property color themeColor
    property color controlColor
    property color tipBackgroundColor
    property color tipBorderColor
    property color infoPanelColor
    property color infoPanelBgColor

    property color controlBorderColor
    property int currentTheme: -1
    onCurrentThemeChanged: {
        //console.log("onCurrentThemeChanged-currentTheme:",currentTheme)
        var t = themes.get(currentTheme)

        //modelGroup
        normal = t.normal
        negative = t.negative
        modifier = t.modifier
        normalRelief = t.normalRelief
        negativeRelief = t.negativeRelief
        modifierRelief = t.modifierRelief

        supportDefender = t.supportDefender
        supportGenerator = t.supportGenerator

        mainBackgroundColor = t.mainBackgroundColor
        dropShadowColor = t.dropShadowColor
        headerBackgroundColor = t.headerBackgroundColor;
        headerBackgroundColorEnd = t.headerBackgroundColorEnd;
        topToolBarColor = t.topToolBarColor;
        topToolBarColorEnd = t.topToolBarColorEnd;
        menuBarBtnColor = t.menuBarBtnColor;
        menuBarBtnColor_hovered = t.menuBarBtnColor_hovered
        menuTextColor = t.menuTextColor;
        menuTextColor_hovered = t.menuTextColor_hovered
        menuTextColor_normal = t.menuTextColor_normal
        menuStyleBgColor = t.menuStyleBgColor;
        menuStyleBgColor_hovered = t.menuStyleBgColor_hovered;
        infoPanelColor = t.infoPanelColor;
        aboutDiaItemRectBgColor = t.aboutDiaItemRectBgColor;

        //LeftToolBar
        leftBtnBgColor_normal       = t.leftBtnBgColor_normal
        leftBtnBgColor_hovered      = t.leftBtnBgColor_hovered
        leftBtnBgColor_selected     = t.leftBtnBgColor_selected
        leftToolBtnColor_normal       = t.leftToolBtnColor_normal
        leftToolBtnColor_hovered      = t.leftToolBtnColor_hovered
        leftTextColor               = t.leftTextColor
        leftTextColor_d             = t.leftTextColor_d
        leftBarBgColor              = t.leftBarBgColor
        leftTabBgColor              = t.leftTabBgColor

        topBtnBgColor_normal        = t.topBtnBgColor_normal
        topBtnBgColor_hovered       = t.topBtnBgColor_hovered
        topBtnBorderColor_normal    = t.topBtnBorderColor_normal
        topBtnBordeColor_hovered    = t.topBtnBordeColor_hovered
        topBtnTextColor_normal      = t.topBtnTextColor_normal
        topBtnTextColor_hovered     = t.topBtnTextColor_hovered
        topBtnTextColor_checked     = t.topBtnTextColor_checked

        themeColor = t.themeColor
        tipBackgroundColor = t.tipBackgroundColor
        tipBorderColor = t.tipBorderColor
        controlColor = t.controlColor
        controlBorderColor = t.controlBorderColor
        itemBackgroundColor = t.itemBackgroundColor

        buttonColor = t.buttonColor
        dialogBtnHoveredColor = t.dialogBtnHoveredColor
        secondBtnHoverdColor = t.secondBtnHoverdColor
        secondBtnSelectionColor = t.secondBtnSelectionColor
        hoveredColor = t.hoveredColor
        selectionColor = t.selectionColor
        textBackgroundColor = t.textBackgroundColor
        rectBorderColor = t.rectBorderColor
        dialogTitleColor = t.dialogTitleColor
        dialogContentBgColor = t.dialogContentBgColor
        dialogTitleTextColor = t.dialogTitleTextColor
        dialogBorderColor = t.dialogBorderColor

        searchBtnDisableColor = t.searchBtnDisableColor
        searchBtnNormalColor = t.searchBtnNormalColor
        searchBtnHoveredColor = t.searchBtnHoveredColor
        typeBtnHoveredColor = t.typeBtnHoveredColor

        textColor = t.textColor
        dialogItemRectBgColor = t.dialogItemRectBgColor
        dialogItemRectBgBorderColor = t.dialogItemRectBgBorderColor

        splitLineColor = t.splitLineColor
        splitLineColorDark = t.splitLineColorDark
        radioCheckLabelColor = t.radioCheckLabelColor
        radioCheckRadiusColor = t.radioCheckRadiusColor
        radioInfillBgColor = t.radioInfillBgColor
        radioBorderColor = t.radioBorderColor
        profileBtnColor = t.profileBtnColor
        lpw_BtnColor = t.lpw_BtnColor
        profileBtnHoverColor = t.profileBtnHoverColor
        tooltipBgColor = t.tooltipBgColor
        checkBoxBgColor = t.checkBoxBgColor
        checkBoxBorderColor = t.checkBoxBorderColor
        cmbIndicatorRectColor_basic = t.cmbIndicatorRectColor_basic
        cmbIndicatorRectColor_pressed_basic = t.cmbIndicatorRectColor_pressed_basic
        menuSplitLineColor = t.menuSplitLineColor
        menuSplitLineColorleft = t.menuSplitLineColorleft
        mainViewSplitLineColor = t.mainViewSplitLineColor
        printerCombRectColor = t.printerCombRectColor
        sliderTopColor1 = t.sliderTopColor1
        sliderTopColor2 = t.sliderTopColor2
        sliderBottomColor = t.sliderBottomColor

        previewPanelRecgColor = t.previewPanelRecgColor
        tittleBarBtnColor = t.tittleBarBtnColor
        laserFoldTittleColor = t.laserFoldTittleColor

        treeBackgroundColor = t.treeBackgroundColor
        treeItemColor = t.treeItemColor
        treeItemColor_pressed = t.treeItemColor_pressed
        tabButtonSelectColor = t.tabButtonSelectColor
        tabButtonNormalColor = t.tabButtonNormalColor
        switchModelBgColor = t.switchModelBgColor
        switchModeSelectedlBgColor = t.switchModeSelectedlBgColor
        clearBtnImg = t.clearBtnImg
        clearBtnImg_d = t.clearBtnImg_d
        searchBtnImg = t.searchBtnImg
        searchBtnImg_d = t.searchBtnImg_d
        downBtnImg = t.downBtnImg
        checkBtnImg = t.checkBtnImg
        showPWNormalImg = t.showPWNormalImg
        hidePWNormalImg = t.hidePWNormalImg
        showPWHoveredImg = t.showPWHoveredImg
        hidePWHoveredImg = t.hidePWHoveredImg
        areaCodeComboboxImg = t.areaCodeComboboxImg
        configTabBtnImg = t.configTabBtnImg
        supportTabBtnImg = t.supportTabBtnImg
        configTabBtnImg_Hovered = t.configTabBtnImg_Hovered
        supportTabBtnImg_Hovered = t.supportTabBtnImg_Hovered
        progressBarBgColor = t.progressBarBgColor
        previewPanelTextgColor = t.previewPanelTextgColor
        modelAlwaysMoreBtnBgColor = t.modelAlwaysMoreBtnBgColor
        modelAlwaysItemBorderColor = t.modelAlwaysItemBorderColor
        homeImg = t.homeImg
        homeImg_Hovered = t.homeImg_Hovered
        laserPickImg = t.laserPickImg
        laserImageImg = t.laserImageImg
        laserFontImg = t.laserFontImg
        laserRectImg = t.laserRectImg
        laserArcImg = t.laserArcImg
        laserPickHoveredImg = t.laserPickHoveredImg
        laserImageHoveredImg = t.laserImageHoveredImg
        laserFontHoveredImg = t.laserFontHoveredImg
        laserRectHoveredImg = t.laserRectHoveredImg
        laserArcHoveredImg = t.laserArcHoveredImg
        laserPickCheckedImg = t.laserPickCheckedImg
        laserImageCheckedImg = t.laserImageCheckedImg
        laserFontCheckedImg = t.laserFontCheckedImg
        laserRectCheckedImg = t.laserRectCheckedImg
        laserArcCheckedImg = t.laserArcCheckedImg
        switchModelFDMImg = t.switchModelFDMImg
        switchModelLaserImg = t.switchModelLaserImg
        switchModelFDMImg_H = t.switchModelFDMImg_H
        switchModelLaserImg_H = t.switchModelLaserImg_H
        switchModelLaserImgEn = t.switchModelLaserImgEn
        switchModelLaserImgEn_H = t.switchModelLaserImgEn_H
        switchModelCNCImg = t.switchModelCNCImg
        switchModelCNCImg_H = t.switchModelCNCImg_H

        printMoveAxisYUpImg = t.printMoveAxisYUpImg
        printMoveAxisYUp_HImg = t.printMoveAxisYUp_HImg
        printMoveAxisYUp_CImg = t.printMoveAxisYUp_CImg
        printMoveAxisZUpImg = t.printMoveAxisZUpImg
        printMoveAxisZUp_HImg = t.printMoveAxisZUp_HImg
        printMoveAxisZUp_CImg = t.printMoveAxisZUp_CImg
        printMoveAxisXDownImg = t.printMoveAxisXDownImg
        printMoveAxisXDown_HImg = t.printMoveAxisXDown_HImg
        printMoveAxisXDown_CImg = t.printMoveAxisXDown_CImg
        printMoveAxisZeroImg = t.printMoveAxisZeroImg
        printMoveAxisZero_HImg = t.printMoveAxisZero_HImg
        printMoveAxisZero_CImg = t.printMoveAxisZero_CImg
        printMoveAxisXUpImg = t.printMoveAxisXUpImg
        printMoveAxisXUp_HImg = t.printMoveAxisXUp_HImg
        printMoveAxisXUp_CImg = t.printMoveAxisXUp_CImg
        printMoveAxisYDownImg = t.printMoveAxisYDownImg
        printMoveAxisYDown_HImg = t.printMoveAxisYDown_HImg
        printMoveAxisYDown_CImg = t.printMoveAxisYDown_CImg
        printMoveAxisZDownImg = t.printMoveAxisZDownImg
        printMoveAxisZDown_HImg = t.printMoveAxisZDown_HImg
        printMoveAxisZDown_CImg = t.printMoveAxisZDown_CImg

        modelAlwaysPopBg = t.modelAlwaysPopBg
        modelAlwaysBtnIcon = t.modelAlwaysBtnIcon
        uploadModelImg = t.uploadModelImg
        uploadLocalModelImg = t.uploadLocalModelImg
        deleteModelImg = t.deleteModelImg

        editProfileImg = t.editProfileImg
        uploadProfileImg = t.uploadProfileImg
        deleteProfileImg = t.deleteProfileImg

        onSrcImg = t.onSrcImg
        offSrcImg = t.offSrcImg
        checkedSrcImg = t.checkedSrcImg
        uncheckSrcImg = t.uncheckSrcImg

        cube3DAmbientColor = t.cube3DAmbientColor
        textColor_disabled = t.textColor_disabled
        printerCommboxBtnDefaultColor = t.printerCommboxBtnDefaultColor
        printerCommboxPopBgColor = t.printerCommboxPopBgColor
        printerCommboxIndicatorBgColor = t.printerCommboxIndicatorBgColor
        printerCommboxIndicatorPopShowBgColor = t.printerCommboxIndicatorPopShowBgColor
        printerCommboxBgColor = t.printerCommboxBgColor
        printerCommboxBgBorderColor = t.printerCommboxBgBorderColor

        cube3DTopImg = t.cube3DTopImg
        cube3DTopImg_C = t.cube3DTopImg_C
        cube3DFrontImg = t.cube3DFrontImg
        cube3DFrontImg_C = t.cube3DFrontImg_C
        cube3DBottomImg = t.cube3DBottomImg
        cube3DBottomImg_C = t.cube3DBottomImg_C
        cube3DBackImg = t.cube3DBackImg
        cube3DBackImg_C = t.cube3DBackImg_C
        cube3DLeftkImg = t.cube3DLeftkImg
        cube3DLeftkImg_C = t.cube3DLeftkImg_C
        cube3DLeftkRight = t.cube3DLeftkRight
        cube3DLeftkRight_C = t.cube3DLeftkRight_C
        laserFoldTitleUpImg = t.laserFoldTitleUpImg
        laserFoldTitleDownImg = t.laserFoldTitleDownImg
        enabledBtnShadowColor = t.enabledBtnShadowColor
        laserLineBorderColor = t.laserLineBorderColor
        upBtnImgSource = t.upBtnImgSource
        upBtnImgSource_d = t.upBtnImgSource_d
        downBtnImgSource = t.downBtnImgSource
        downBtnImgSource_d = t.downBtnImgSource_d

        // ---------- left panel [beg] ----------
        left_model_list_button_default_color = t.left_model_list_button_default_color
        left_model_list_button_hovered_color = t.left_model_list_button_hovered_color
        left_model_list_button_border_default_color = t.left_model_list_button_border_default_color
        left_model_list_button_border_hovered_color = t.left_model_list_button_border_hovered_color
        left_model_list_button_default_icon = t.left_model_list_button_default_icon
        left_model_list_button_hovered_icon = t.left_model_list_button_hovered_icon
        left_model_list_count_background_color = t.left_model_list_count_background_color
        left_model_list_count_text_color = t.left_model_list_count_text_color

        left_model_list_background_color = t.left_model_list_background_color
        left_model_list_border_color = t.left_model_list_border_color

        left_model_list_title_text_color = t.left_model_list_title_text_color

        left_model_list_close_button_default_color = t.left_model_list_close_button_default_color
        left_model_list_close_button_hovered_color = t.left_model_list_close_button_hovered_color
        left_model_list_close_button_default_icon = t.left_model_list_close_button_default_icon
        left_model_list_close_button_hovered_icon = t.left_model_list_close_button_hovered_icon

        left_model_list_all_button_border_color = t.left_model_list_all_button_border_color
        left_model_list_all_button_text_color = t.left_model_list_all_button_text_color
        left_model_list_all_button_icon = t.left_model_list_all_button_icon

        left_model_list_action_button_default_color = t.left_model_list_action_button_default_color
        left_model_list_action_button_hovered_color = t.left_model_list_action_button_hovered_color
        left_model_list_del_button_default_icon = t.left_model_list_del_button_default_icon
        left_model_list_del_button_hovered_icon = t.left_model_list_del_button_hovered_icon

        left_model_list_item_default_color = t.left_model_list_item_default_color
        left_model_list_item_hovered_color = t.left_model_list_item_hovered_color
        left_model_list_item_text_default_color = t.left_model_list_item_text_default_color
        left_model_list_item_text_hovered_color = t.left_model_list_item_text_hovered_color
        // ---------- left panel [end] ----------

        // ---------- right panel [beg] ----------

        right_panel_text_default_color         = t.right_panel_text_default_color
        right_panel_text_disable_color         = t.right_panel_text_disable_color
        right_panel_text_hovered_color         = t.right_panel_text_hovered_color
        right_panel_text_checked_color         = t.right_panel_text_checked_color

        right_panel_gcode_text_color = t.right_panel_gcode_text_color

        right_panel_button_default_color       = t.right_panel_button_default_color
        right_panel_button_disable_color       = t.right_panel_button_disable_color
        right_panel_button_hovered_color       = t.right_panel_button_hovered_color
        right_panel_button_checked_color       = t.right_panel_button_checked_color

        right_panel_border_default_color       = t.right_panel_border_default_color
        right_panel_border_disable_color       = t.right_panel_border_disable_color
        right_panel_border_hovered_color       = t.right_panel_border_hovered_color
        right_panel_border_checked_color       = t.right_panel_border_checked_color

        right_panel_item_default_color         = t.right_panel_item_default_color
        right_panel_item_disable_color         = t.right_panel_item_disable_color
        right_panel_item_hovered_color         = t.right_panel_item_hovered_color
        right_panel_item_checked_color         = t.right_panel_item_checked_color
        right_panel_item_text_default_color    = t.right_panel_item_text_default_color
        right_panel_item_text_disable_color    = t.right_panel_item_text_disable_color
        right_panel_item_text_hovered_color    = t.right_panel_item_text_hovered_color
        right_panel_item_text_checked_color    = t.right_panel_item_text_checked_color

        right_panel_combobox_background_color  = t.right_panel_combobox_background_color

        right_panel_tab_text_default_color     = t.right_panel_tab_text_default_color
        right_panel_tab_text_disable_color     = t.right_panel_tab_text_disable_color
        right_panel_tab_text_hovered_color     = t.right_panel_tab_text_hovered_color
        right_panel_tab_text_checked_color     = t.right_panel_tab_text_checked_color
        right_panel_tab_button_default_color   = t.right_panel_tab_button_default_color
        right_panel_tab_button_disable_color   = t.right_panel_tab_button_disable_color
        right_panel_tab_button_hovered_color   = t.right_panel_tab_button_hovered_color
        right_panel_tab_button_checked_color   = t.right_panel_tab_button_checked_color
        right_panel_tab_background_color       = t.right_panel_tab_background_color

        right_panel_menu_border_color          = t.right_panel_menu_border_color
        right_panel_menu_background_color      = t.right_panel_menu_background_color
        right_panel_menu_split_line_color      = t.right_panel_menu_split_line_color

        right_panel_slice_button_default_color = t.right_panel_slice_button_default_color
        right_panel_slice_button_disable_color = t.right_panel_slice_button_disable_color
        right_panel_slice_button_hovered_color = t.right_panel_slice_button_hovered_color
        right_panel_slice_button_checked_color = t.right_panel_slice_button_checked_color
        right_panel_slice_text_default_color   = t.right_panel_slice_text_default_color
        right_panel_slice_text_disable_color   = t.right_panel_slice_text_disable_color
        right_panel_slice_text_hovered_color   = t.right_panel_slice_text_hovered_color
        right_panel_slice_text_checked_color   = t.right_panel_slice_text_checked_color
        right_panel_bgColor                    = t.right_panel_bgColor

        right_panel_quality_custom_default_image  = t.right_panel_quality_custom_default_image
        right_panel_quality_custom_checked_image  = t.right_panel_quality_custom_checked_image
        right_panel_quality_high_default_image    = t.right_panel_quality_high_default_image
        right_panel_quality_high_checked_image    = t.right_panel_quality_high_checked_image
        right_panel_quality_middle_default_image  = t.right_panel_quality_middle_default_image
        right_panel_quality_middle_checked_image  = t.right_panel_quality_middle_checked_image
        right_panel_quality_low_default_image     = t.right_panel_quality_low_default_image
        right_panel_quality_low_checked_image     = t.right_panel_quality_low_checked_image
        right_panel_quality_verylow_default_image = t.right_panel_quality_verylow_default_image
        right_panel_quality_verylow_checked_image = t.right_panel_quality_verylow_checked_image

        right_panel_process_level_image = t.right_panel_process_level_image
        right_panel_process_custom_image = t.right_panel_process_custom_image

        right_panel_delete_image = t.right_panel_delete_image
        right_panel_delete_hovered_image = t.right_panel_delete_hovered_image
        right_panel_edit_image = t.right_panel_edit_image
        right_panel_edit_hovered_image = t.right_panel_edit_hovered_image
        right_panel_save_image = t.right_panel_save_image
        right_panel_save_disabled_image = t.right_panel_save_disabled_image
        right_panel_reset_image = t.right_panel_reset_image
        right_panel_reset_disabled_image = t.right_panel_reset_disabled_image

        // ---------- right panel [end] ----------

        //lanPrinter Panel
        lanPrinter_panel_border = t.lanPrinter_panel_border
        lanPrinter_panel_light_txt = t.lanPrinter_panel_light_txt
        lanPrinter_panel_weight_txt = t.lanPrinter_panel_weight_txt
        lanPrinter_panel_background = t.lanPrinter_panel_background
        lanPrinter_panel_btn_default = t.lanPrinter_panel_btn_default
        lanPrinter_panel_btn_hovered = t.lanPrinter_panel_btn_hovered

        //leftPopWin
        themeTextColor = t.themeTextColor
        themeColor_primary = t.themeColor_primary
        themeColor_secondary = t.themeColor_secondary
        themeColor_third = t.themeColor_third
        addModel = t.addModel
        delModel = t.delModel
        drawerBgImg = t.drawerBgImg

        // ---------- dock [beg] ----------
        dock_context_tab_button_text_checked_color = t.dock_context_tab_button_text_checked_color
        dock_context_tab_button_text_default_color = t.dock_context_tab_button_text_default_color
        dock_context_tab_button_checked_color      = t.dock_context_tab_button_checked_color
        dock_context_tab_button_default_color      = t.dock_context_tab_button_default_color
        dock_context_tab_bar_color                 = t.dock_context_tab_bar_color
        dock_border_color                          = t.dock_border_color
        // ---------- dock [end] ----------

        // ---------- model library [beg] ----------
        model_library_border_color                       = t.model_library_border_color

        model_library_type_button_default_color          = t.model_library_type_button_default_color
        model_library_type_button_checked_color          = t.model_library_type_button_checked_color
        model_library_type_button_text_default_color     = t.model_library_type_button_text_default_color
        model_library_type_button_text_checked_color     = t.model_library_type_button_text_checked_color

        model_library_back_button_default_color          = t.model_library_back_button_default_color
        model_library_back_button_checked_color          = t.model_library_back_button_checked_color
        model_library_back_button_border_default_color   = t.model_library_back_button_border_default_color
        model_library_back_button_border_checked_color   = t.model_library_back_button_border_checked_color
        model_library_back_button_text_default_color     = t.model_library_back_button_text_default_color
        model_library_back_button_text_checked_color     = t.model_library_back_button_text_checked_color
        model_library_back_button_default_image          = t.model_library_back_button_default_image
        model_library_back_button_checked_image          = t.model_library_back_button_checked_image

        model_library_action_button_default_color        = t.model_library_action_button_default_color
        model_library_action_button_checked_color        = t.model_library_action_button_checked_color
        model_library_action_button_border_default_color = t.model_library_action_button_border_default_color
        model_library_action_button_border_checked_color = t.model_library_action_button_border_checked_color
        model_library_action_button_text_default_color   = t.model_library_action_button_text_default_color
        model_library_action_button_text_checked_color   = t.model_library_action_button_text_checked_color

        model_library_import_button_default_color        = t.model_library_import_button_default_color
        model_library_import_button_checked_color        = t.model_library_import_button_checked_color

        model_library_special_text_color                 = t.model_library_special_text_color
        model_library_general_text_color                 = t.model_library_general_text_color

        model_library_item_default_color                 = t.model_library_item_default_color
        model_library_item_hovered_color                 = t.model_library_item_hovered_color
        model_library_item_checked_color                 = t.model_library_item_checked_color
        model_library_item_text_default_color            = t.model_library_item_text_default_color
        model_library_item_text_checked_color            = t.model_library_item_text_checked_color

        model_library_license_default_color              = t.model_library_license_default_color
        model_library_license_checked_color              = t.model_library_license_checked_color
        model_library_license_default_image              = t.model_library_license_default_image
        model_library_license_checked_image              = t.model_library_license_checked_image

        model_library_import_default_image               = t.model_library_import_default_image
        model_library_import_checked_image               = t.model_library_import_checked_image
        model_library_download_default_image             = t.model_library_download_default_image
        model_library_download_checked_image             = t.model_library_download_checked_image
        model_library_uncollect_default_image            = t.model_library_uncollect_default_image
        model_library_uncollect_checked_image            = t.model_library_uncollect_checked_image
        model_library_shared_default_image               = t.model_library_shared_default_image
        model_library_shared_checked_image               = t.model_library_shared_checked_image
        // ---------- model library [end] ----------

        // ---------- custom tabview [beg] ----------
        custom_tabview_border_color              = t.custom_tabview_border_color
        custom_tabview_panel_color               = t.custom_tabview_panel_color
        custom_tabview_button_color              = t.custom_tabview_button_color
        custom_tabview_button_text_default_color = t.custom_tabview_button_text_default_color
        custom_tabview_button_text_checked_color = t.custom_tabview_button_text_checked_color
        custom_tabview_combo_item_hovered_color  = t.custom_tabview_combo_item_hovered_color
        // ---------- custom tabview [end] ----------

        // ---------- manager printer dialog [beg] ----------
        manager_printer_switch_default_color = t.manager_printer_switch_default_color
        manager_printer_switch_checked_color = t.manager_printer_switch_checked_color
        manager_printer_switch_default_image = t.manager_printer_switch_default_image
        manager_printer_switch_checked_image = t.manager_printer_switch_checked_image

        manager_printer_tabview_default_color = t.manager_printer_tabview_default_color
        manager_printer_tabview_checked_color = t.manager_printer_tabview_checked_color

        manager_printer_label_color = t.manager_printer_label_color

        manager_printer_gcode_title_color = t.manager_printer_gcode_title_color
        manager_printer_gcode_text_color = t.manager_printer_gcode_text_color

        manager_printer_button_text_color = t.manager_printer_button_text_color
        manager_printer_button_border_color = t.manager_printer_button_border_color
        manager_printer_button_default_color = t.manager_printer_button_default_color
        manager_printer_button_checked_color = t.manager_printer_button_checked_color
        // ---------- manager printer dialog [end] ----------

        // ---------- download manage dialig [beg] ----------
        downloadbtn_image = t.downloadbtn_image
        downloadbtn_image_hovered = t.downloadbtn_image_hovered
        downloadbtn_tip_image = t.downloadbtn_tip_image
        downloadbtn_tip_color = t.downloadbtn_tip_color

        downloadbtn_finished_default_color = t.downloadbtn_finished_default_color
        downloadbtn_finished_hovered_color = t.downloadbtn_finished_hovered_color
        downloadbtn_finished_text_default_color = t.downloadbtn_finished_text_default_color
        downloadbtn_finished_text_hovered_color = t.downloadbtn_finished_text_hovered_color
        downloadbtn_finished_border_default_color = t.downloadbtn_finished_border_default_color
        downloadbtn_finished_border_hovered_color = t.downloadbtn_finished_border_hovered_color

        downloadbtn_download_default_color = t.downloadbtn_download_default_color
        downloadbtn_download_hovered_color = t.downloadbtn_download_hovered_color
        downloadbtn_download_text_default_color = t.downloadbtn_download_text_default_color
        downloadbtn_download_text_hovered_color = t.downloadbtn_download_text_hovered_color
        downloadbtn_download_border_default_color = t.downloadbtn_download_border_default_color
        downloadbtn_download_border_hovered_color = t.downloadbtn_download_border_hovered_color

        download_manage_tab_button_color = t.download_manage_tab_button_color
        download_manage_title_text_color = t.download_manage_title_text_color
        download_manage_model_text_color = t.download_manage_model_text_color
        download_manage_prograss_left_color = t.download_manage_prograss_left_color
        download_manage_prograss_right_color = t.download_manage_prograss_right_color
        download_manage_group_open_image = t.download_manage_group_open_image
        download_manage_group_close_image = t.download_manage_group_close_image

        download_manage_empty_button_image = t.download_manage_empty_button_image
        download_manage_empty_text_color = t.download_manage_empty_text_color
        download_manage_empty_button_default_color = t.download_manage_empty_button_default_color
        download_manage_empty_button_checked_color = t.download_manage_empty_button_checked_color
        download_manage_empty_button_text_default_color = t.download_manage_empty_button_text_default_color
        download_manage_empty_button_text_checked_color = t.download_manage_empty_button_text_checked_color
        // ---------- download manage dialig [end] ----------

        // ---------- left tool bar [end] ----------
        leftbar_rocommand_btn_icon_default = t.leftbar_rocommand_btn_icon_default
        leftbar_rocommand_btn_icon_hovered = t.leftbar_rocommand_btn_icon_hovered
        leftbar_rocommand_btn_icon_pressed = t.leftbar_rocommand_btn_icon_pressed
        leftbar_open_btn_icon_default = t.leftbar_open_btn_icon_default
        leftbar_open_btn_icon_hovered = t.leftbar_open_btn_icon_hovered
        leftbar_open_btn_icon_pressed = t.leftbar_open_btn_icon_pressed
        leftbar_pick_btn_icon_default = t.leftbar_pick_btn_icon_default
        leftbar_pick_btn_icon_hovered = t.leftbar_pick_btn_icon_hovered
        leftbar_pick_btn_icon_pressed = t.leftbar_pick_btn_icon_pressed
        leftbar_other_btn_icon_default = t.leftbar_other_btn_icon_default
        leftbar_other_btn_icon_hovered = t.leftbar_other_btn_icon_hovered
        leftbar_other_btn_icon_checked = t.leftbar_other_btn_icon_checked
        leftbar_btn_border_color = t.leftbar_btn_border_color
        // ---------- left tool bar [end] ----------

        // ---------- basic compenent new [beg] ----------
        switch_border_color = t.switch_border_color
        switch_indicator_color = t.switch_indicator_color
        switch_background_color = t.switch_background_color
        switch_indicator_text_color = t.switch_indicator_text_color
        switch_background_text_color = t.switch_background_text_color
        // ---------- basic compenent new [end] ----------

        // ---------- parameter [beg] ----------
        parameter_text_color = t.parameter_text_color
        parameter_text_modifyed_color = t.parameter_text_modifyed_color
        parameter_group_text_color = t.parameter_group_text_color
        parameter_unit_text_color = t.parameter_unit_text_color
        parameter_editer_modifyed_color = t.parameter_editer_modifyed_color
        parameter_reset_button_image = t.parameter_reset_button_image
        // ---------- parameter [end] ----------

    }
    property ListModel themes: ListModel {
        ListElement {
            name: "Dark Theme"
            modifier:"qrc:/UI/photo/modelGroup/modifier.svg"
            negative:"qrc:/UI/photo/modelGroup/negative.svg"
            normalRelief:"qrc:/UI/photo/modelGroup/normalRelief.svg"
            modifierRelief:"qrc:/UI/photo/modelGroup/modifierRelief.svg"
            negativeRelief:"qrc:/UI/photo/modelGroup/negativeRelief.svg"
            normal:"qrc:/UI/photo/modelGroup/normal.svg"
            supportDefender:"qrc:/UI/photo/modelGroup/supportDefender.svg"
            supportGenerator:"qrc:/UI/photo/modelGroup/supportGenerator.svg"
            mainBackgroundColor : "#363638"
            dropShadowColor : "#333333"
            headerBackgroundColor:"#000000"
            headerBackgroundColorEnd: "#000000"
            topToolBarColor:"#1C1C1D"
            topToolBarColorEnd:"#363638"
            menuBarBtnColor: "transparent"
            menuBarBtnColor_hovered: "#3E3E3E"
            menuTextColor: "#000000"
            menuTextColor_hovered: "#FFFFFF"
            menuTextColor_normal: "#FFFFFF"
            menuStyleBgColor: "#FFFFFF"
            menuStyleBgColor_hovered: "#17CC5F"
            themeColor: "#4B4B4D"
            textColor: "#ffffff"
            invalidColor: "#782c2c"
            alterColor: "#383838"
            tipBackgroundColor: "#ffffff"
            tipBorderColor: "#767676"
            buttonColor : "#6E6E73"
            dialogBtnHoveredColor: "#86868A"
            secondBtnHoverdColor: "#666666"
            secondBtnSelectionColor: "#36B785"
            rectBorderColor : "#6C6C70"
            hoveredColor: "#333333"
            selectionColor : "#3A3A3A"
            textBackgroundColor : "#4B4B4B"
            textColor_disabled:"#AFAFAF"
            printerCommboxBtnDefaultColor: "#B7B7B7"
            printerCommboxPopBgColor: "#E1E1E1"
            printerCommboxIndicatorBgColor: "#666666"
            printerCommboxIndicatorPopShowBgColor: "#E1E1E1"
            printerCommboxBgColor: "#181818"
            printerCommboxBgBorderColor: "#333333"
            textColor_hovered: "#ffffff"
            textColor_pressed: "#d2d2d2"
            controlColor: "#333333"
            controlBorderColor: "#555555"
            imageColor: "#adafb2"
            imageColor_disabled: "#989898"
            scrollBarBackgroundColor: "#3f3f3f"
            scrollBarBackgroundColor_hovered: "#4a4a4a"
            aboutDiaItemRectBgColor: "#333333"

            //lsg
            //LeftToolBar Btn
            leftBtnBgColor_normal:  "#505052"
            leftBtnBgColor_hovered: "#6D6D71"
            leftBtnBgColor_selected: "#17CC5F"
            leftToolBtnColor_normal: "#6e6e73"
            leftToolBtnColor_hovered: "#8a8a8a"
            leftTextColor: "#C3C3C3"
            leftTextColor_d: "white"
            leftBarBgColor: "#2B2B2D"
            leftTabBgColor: "#505052"

            topBtnBgColor_normal:  "#1E9BE2"
            topBtnBgColor_hovered: "#212122"
            topBtnBorderColor_normal: "transparent"
            topBtnBordeColor_hovered: "transparent"
            topBtnTextColor_normal : "#d3d3dd"
            topBtnTextColor_hovered : "#d3d3dd"
            topBtnTextColor_checked : "#FFFFFF"

            itemBackgroundColor : "#535353"
            infoPanelColor: "#DBDADA"

            dialogTitleColor: "#6E6E73"
            dialogContentBgColor: "#535353"
            dialogTitleTextColor:"#ffffff"
            dialogBorderColor : "#262626"

            searchBtnDisableColor: "#999999"
            searchBtnNormalColor: "#1EB6E2"
            searchBtnHoveredColor: "#1E9BE2"
            typeBtnHoveredColor : "#1EB6E2"//"#686868"

            dialogItemRectBgColor: "#4B4B4D"
            dialogItemRectBgBorderColor : "#6E6E72"

            splitLineColor : "#68686B"
            splitLineColorDark : "#444444"
            radioCheckLabelColor: "white"
            radioCheckRadiusColor: "#ffffff"
            radioInfillBgColor: "#00a3ff"
            radioBorderColor: "#A7A7B0"
            profileBtnColor: "#6E6E73"
            lpw_BtnColor: "#6e6e73"
            profileBtnHoverColor: "#86868A"
            tooltipBgColor: "#454545"
            checkBoxBgColor: "#424242"
            checkBoxBorderColor: "#333333"
            cmbIndicatorRectColor_pressed_basic : "#5F5F62"
            cmbIndicatorRectColor_basic: "transparent"

            menuSplitLineColor: "#37373A"
            menuSplitLineColorleft: "#303030"
            mainViewSplitLineColor: "#0D0D0D"
            printerCombRectColor: "#D7D7D7"

            treeBackgroundColor: "#424242"
            treeItemColor:"#424242"
            treeItemColor_pressed: "#777777"

            progressBarBgColor: "#303030"
            previewPanelRecgColor: "black"
            previewPanelTextgColor: "#CDD2D7"
            tabButtonSelectColor: "#454545"
            tabButtonNormalColor: "#535353"

            switchModelBgColor: "#181818"
            switchModeSelectedlBgColor: "#535353"
            sliderTopColor1: "#4A4A4A"
            sliderTopColor2: "#767676"
            sliderBottomColor: "#535353"
            modelAlwaysMoreBtnBgColor: "#D7D7D7"
            modelAlwaysItemBorderColor: "#262626"

            cube3DAmbientColor: "#828282"
            cube3DAmbientColor_Entered:"#E5E0D0"

            enabledBtnShadowColor:"black"
            tittleBarBtnColor: "#3E3E3E"
            laserFoldTittleColor: "#424242"

            laserLineBorderColor:"#999999"

            upBtnImgSource: "qrc:/UI/photo/upBtn.svg"
            upBtnImgSource_d:  "qrc:/UI/photo/upBtn_green.svg"

            downBtnImgSource: "qrc:/UI/photo/downBtn.svg"
            downBtnImgSource_d:  "qrc:/UI/photo/downBtn_green.svg"

            clearBtnImg: "qrc:/UI/photo/clearBtn.png"
            clearBtnImg_d: "qrc:/UI/photo/clearBtn_d.png"

            searchBtnImg: "qrc:/UI/photo/search.png"
            searchBtnImg_d: "qrc:/UI/photo/search_d.png"
            downBtnImg: "qrc:/UI/photo/downBtn.svg"
            checkBtnImg: "qrc:/UI/images/check2.png"
            showPWNormalImg: "qrc:/UI/photo/showPW_dark.png"
            hidePWNormalImg: "qrc:/UI/photo/hidePW_dark.png"
            showPWHoveredImg: "qrc:/UI/photo/showPW_h_dark.png"
            hidePWHoveredImg: "qrc:/UI/photo/hidePW_h_dark.png"
            areaCodeComboboxImg: "qrc:/UI/photo/comboboxDown.png"
            configTabBtnImg: "qrc:/UI/photo/configTabBtn.png"
            configTabBtnImg_Hovered: "qrc:/UI/photo/configTabBtn_d.png"
            supportTabBtnImg:"qrc:/UI/photo/supportTabBtn.png"
            supportTabBtnImg_Hovered:"qrc:/UI/photo/supportTabBtn_d.png"
            homeImg: "qrc:/UI/images/home.png"
            homeImg_Hovered: "qrc:/UI/images/home_s.png"

            laserPickImg: "qrc:/UI/images/laser_pick.png"
            laserImageImg: "qrc:/UI/images/laser_img.png"
            laserFontImg: "qrc:/UI/images/laser_font.png"
            laserRectImg: "qrc:/UI/images/laser_rect.png"
            laserArcImg: "qrc:/UI/images/laser_arc.png"
            laserPickHoveredImg: "qrc:/UI/images/laser_pick_s.png"
            laserImageHoveredImg: "qrc:/UI/images/laser_img_s.png"
            laserFontHoveredImg: "qrc:/UI/images/laser_font_s.png"
            laserRectHoveredImg: "qrc:/UI/images/laser_rect_s.png"
            laserArcHoveredImg: "qrc:/UI/images/laser_arc_s.png"
            laserPickCheckedImg: "qrc:/UI/images/laser_pick_s.png"
            laserImageCheckedImg: "qrc:/UI/images/laser_img_s.png"
            laserFontCheckedImg: "qrc:/UI/images/laser_font_s.png"
            laserRectCheckedImg: "qrc:/UI/images/laser_rect_s.png"
            laserArcCheckedImg: "qrc:/UI/images/laser_arc_s.png"

            switchModelFDMImg: "qrc:/UI/images/fdmMode.png"
            switchModelLaserImg: "qrc:/UI/images/laserMode.png"
            switchModelFDMImg_H: "qrc:/UI/images/fdmMode_h.png"
            switchModelLaserImg_H: "qrc:/UI/images/laserMode_h.png"
            switchModelLaserImgEn: "qrc:/UI/images/laserMode_en.png"
            switchModelLaserImgEn_H: "qrc:/UI/images/laserMode_h_en.png"
            switchModelCNCImg: "qrc:/UI/images/CNCMode.png"
            switchModelCNCImg_H: "qrc:/UI/images/CNCMode_h.png"

            printMoveAxisYUpImg: "qrc:/UI/photo/print_move_axis_y+.png"
            printMoveAxisYUp_HImg: "qrc:/UI/photo/print_move_axis_y+_h.png"
            printMoveAxisYUp_CImg: "qrc:/UI/photo/print_move_axis_y+_c.png"
            printMoveAxisZUpImg: "qrc:/UI/photo/print_move_axis_z+.png"
            printMoveAxisZUp_HImg: "qrc:/UI/photo/print_move_axis_z+_h.png"
            printMoveAxisZUp_CImg: "qrc:/UI/photo/print_move_axis_z+_c.png"
            printMoveAxisXDownImg: "qrc:/UI/photo/print_move_axis_x-.png"
            printMoveAxisXDown_HImg: "qrc:/UI/photo/print_move_axis_x-_h.png"
            printMoveAxisXDown_CImg: "qrc:/UI/photo/print_move_axis_x-_c.png"
            printMoveAxisZeroImg: "qrc:/UI/photo/print_move_axis_zero.png"
            printMoveAxisZero_HImg: "qrc:/UI/photo/print_move_axis_zero_h.png"
            printMoveAxisZero_CImg: "qrc:/UI/photo/print_move_axis_zero_c.png"
            printMoveAxisXUpImg: "qrc:/UI/photo/print_move_axis_x+.png"
            printMoveAxisXUp_HImg: "qrc:/UI/photo/print_move_axis_x+_h.png"
            printMoveAxisXUp_CImg: "qrc:/UI/photo/print_move_axis_x+_c.png"
            printMoveAxisYDownImg: "qrc:/UI/photo/print_move_axis_y-.png"
            printMoveAxisYDown_HImg: "qrc:/UI/photo/print_move_axis_y-_h.png"
            printMoveAxisYDown_CImg: "qrc:/UI/photo/print_move_axis_y-_c.png"
            printMoveAxisZDownImg: "qrc:/UI/photo/print_move_axis_z-.png"
            printMoveAxisZDown_HImg: "qrc:/UI/photo/print_move_axis_z-_h.png"
            printMoveAxisZDown_CImg: "qrc:/UI/photo/print_move_axis_z-_c.png"

            modelAlwaysPopBg: "qrc:/UI/photo/model_always_pop_bg.png"
            modelAlwaysBtnIcon: "qrc:/UI/photo/model_always_show.png"
            uploadModelImg: "qrc:/UI/photo/upload_model_img.png"
            uploadLocalModelImg: "qrc:/UI/photo/upload_model_local_img.png"
            deleteModelImg: "qrc:/UI/photo/delete_model_img.png"

            editProfileImg: "qrc:/UI/photo/editProfile.png"
            uploadProfileImg: "qrc:/UI/photo/uploadPro.png"
            deleteProfileImg: "qrc:/UI/photo/deleteProfile.png"

            onSrcImg: "qrc:/UI/photo/on.png"
            offSrcImg: "qrc:/UI/photo/off.png"
            checkedSrcImg: "qrc:/UI/photo/radio_1.png"
            uncheckSrcImg: "qrc:/UI/photo/radio_2.png"

            cube3DTopImg: "qrc:/UI/images/top.png"
            cube3DTopImg_C: "qrc:/UI/images/top_C.png"
            cube3DFrontImg: "qrc:/UI/images/front.png"
            cube3DFrontImg_C: "qrc:/UI/images/front_C.png"
            cube3DBottomImg: "qrc:/UI/images/bottom.png"
            cube3DBottomImg_C: "qrc:/UI/images/bottom_C.png"
            cube3DBackImg: "qrc:/UI/images/back.png"
            cube3DBackImg_C: "qrc:/UI/images/back_C.png"
            cube3DLeftkImg: "qrc:/UI/images/left.png"
            cube3DLeftkImg_C: "qrc:/UI/images/left_C.png"
            cube3DLeftkRight: "qrc:/UI/images/right.png"
            cube3DLeftkRight_C: "qrc:/UI/images/right_C.png"
            laserFoldTitleUpImg: "qrc:/UI/photo/laser_fold_item_up.png"
            laserFoldTitleDownImg: "qrc:/UI/photo/laser_fold_item_down.png"

            // ---------- left panel [beg] ----------
            left_model_list_button_default_color: "#363638"
            left_model_list_button_hovered_color: "#6D6D71"
            left_model_list_button_border_default_color: "#56565C"
            left_model_list_button_border_hovered_color: "#6D6D71"
            left_model_list_button_default_icon: "qrc:/UI/photo/modelLIstIcon.svg"
            left_model_list_button_hovered_icon: "qrc:/UI/photo/modelLIstIcon_h.svg"
            left_model_list_count_background_color: "#FFFFFF"
            left_model_list_count_text_color: "#363638"

            left_model_list_background_color: "#363638"
            left_model_list_border_color: "#56565C"
            left_model_list_title_text_color: "#FFFFFF"

            left_model_list_close_button_default_color: "transparent"
            left_model_list_close_button_hovered_color: "#FF365C"
            left_model_list_close_button_default_icon: "qrc:/UI/photo/closeBtn.svg"
            left_model_list_close_button_hovered_icon: "qrc:/UI/photo/closeBtn_d.svg"

            left_model_list_all_button_border_color: "#1E9BE2"
            left_model_list_all_button_text_color: "#FFFFFF"
            left_model_list_all_button_icon: "qrc:/UI/images/check2.png"

            left_model_list_action_button_default_color: "transparent"
            left_model_list_action_button_hovered_color: "#262626"
            left_model_list_del_button_default_icon: "qrc:/UI/photo/deleteModel_dark.png"
            left_model_list_del_button_hovered_icon: "qrc:/UI/photo/deleteModel_dark.png"

            left_model_list_item_default_color: "transparent"
            left_model_list_item_hovered_color: "#739AB0"
            left_model_list_item_text_default_color: "#CBCBCB"
            left_model_list_item_text_hovered_color: "#FFFFFF"
            // ---------- left panel [end] ----------

            // ---------- right panel [beg] ----------

            right_panel_text_default_color         : "#FFFFFF"
            right_panel_text_disable_color         : "#FFFFFF"
            right_panel_text_hovered_color         : "#FFFFFF"
            right_panel_text_checked_color         : "#FFFFFF"

            right_panel_gcode_text_color           : "#92929B"

            right_panel_button_default_color       : "#4B4B4D"
            right_panel_button_disable_color       : "#4B4B4D"
            right_panel_button_hovered_color       : "#414143"
            right_panel_button_checked_color       : "#414143"

            right_panel_border_default_color       : "#6C6C70"
            right_panel_border_disable_color       : "#6C6C70"
            right_panel_border_hovered_color       : "#17CC5F"
            right_panel_border_checked_color       : "#17CC5F"

            right_panel_item_default_color         : "#414143"
            right_panel_item_disable_color         : "#414143"
            right_panel_item_hovered_color         : "#5F5F5F"
            right_panel_item_checked_color         : "#17CC5F"
            right_panel_item_text_default_color    : "#CBCBCB"
            right_panel_item_text_disable_color    : "#CBCBCB"
            right_panel_item_text_hovered_color    : "#CBCBCB"
            right_panel_item_text_checked_color    : "#FFFFFF"

            right_panel_combobox_background_color  : "#414143"

            right_panel_tab_text_default_color     : "#8D8D91"
            right_panel_tab_text_disable_color     : "#8D8D91"
            right_panel_tab_text_hovered_color     : "#8D8D91"
            right_panel_tab_text_checked_color     : "#FFFFFF"
            right_panel_tab_button_default_color   : "#1C1C1D"
            right_panel_tab_button_disable_color   : "#1C1C1D"
            right_panel_tab_button_hovered_color   : "#1C1C1D"
            right_panel_tab_button_checked_color   : "#1E9BE2"
            right_panel_tab_background_color       : "#1C1C1D"

            right_panel_menu_border_color          : "#1C1C1D"
            right_panel_menu_background_color      : "#4B4B4D"
            right_panel_menu_split_line_color      : "#3B3B3D"

            right_panel_slice_button_default_color : "#1E9BE2"
            right_panel_slice_button_disable_color : "#626265"
            right_panel_slice_button_hovered_color : "#57B4E9"
            right_panel_slice_button_checked_color : "#57B4E9"
            right_panel_slice_text_default_color   : "#FFFFFF"
            right_panel_slice_text_disable_color   : "#B1B1B7"
            right_panel_slice_text_hovered_color   : "#FFFFFF"
            right_panel_slice_text_checked_color   : "#FFFFFF"
            right_panel_bgColor                    : "#4B4B4D"

            right_panel_quality_custom_default_image  : "qrc:/UI/photo/config_quality_custom_default.svg"
            right_panel_quality_custom_checked_image  : "qrc:/UI/photo/config_quality_custom_checked.svg"
            right_panel_quality_high_default_image    : "qrc:/UI/photo/config_quality_high_default.svg"
            right_panel_quality_high_checked_image    : "qrc:/UI/photo/config_quality_high_checked.svg"
            right_panel_quality_middle_default_image  : "qrc:/UI/photo/config_quality_middle_default.svg"
            right_panel_quality_middle_checked_image  : "qrc:/UI/photo/config_quality_middle_checked.svg"
            right_panel_quality_low_default_image     : "qrc:/UI/photo/config_quality_low_default.svg"
            right_panel_quality_low_checked_image     : "qrc:/UI/photo/config_quality_low_checked.svg"
            right_panel_quality_verylow_default_image : "qrc:/UI/photo/config_quality_verylow_default.svg"
            right_panel_quality_verylow_checked_image : "qrc:/UI/photo/config_quality_verylow_checked.svg"

            right_panel_process_level_image: "qrc:/UI/photo/rightDrawer/process_level_dark.svg"
            right_panel_process_custom_image: "qrc:/UI/photo/rightDrawer/process_custom_dark.svg"

            right_panel_delete_image: "qrc:/UI/photo/rightDrawer/delete.svg"
            right_panel_delete_hovered_image: "qrc:/UI/photo/rightDrawer/delete_hovered.svg"
            right_panel_edit_image: "qrc:/UI/photo/rightDrawer/edit.svg"
            right_panel_edit_hovered_image: "qrc:/UI/photo/rightDrawer/edit_hovered.svg"
            right_panel_save_image: "qrc:/UI/photo/rightDrawer/save_dark.svg"
            right_panel_save_disabled_image: "qrc:/UI/photo/rightDrawer/save_dark_disabled.svg"
            right_panel_reset_image: "qrc:/UI/photo/rightDrawer/reset_dark.svg"
            right_panel_reset_disabled_image: "qrc:/UI/photo/rightDrawer/reset_dark_disabled.svg"

            // ---------- right panel [end] ----------

            //lanPrinter Panel
            lanPrinter_panel_border: "transparent"
            lanPrinter_panel_light_txt: "#7A7A82"
            lanPrinter_panel_weight_txt: "#FFFFFF"
            lanPrinter_panel_background: "#171718"
            lanPrinter_panel_btn_default: "#2E2E30"
            lanPrinter_panel_btn_hovered: "#414143"

            //leftPopWin
            themeTextColor: "#ffffff"
            themeColor_primary: "#4b4b4d"
            themeColor_secondary: "#6e6e73"
            themeColor_third: "#6e6e73"
            addModel: "qrc:/UI/photo/addModel_dark.png"
            delModel: "qrc:/UI/photo/deleteModel_dark.png"
            drawerBgImg: "qrc:/UI/photo/rightDrawer/drawerBg.svg"

            // ---------- dock [beg] ----------
            dock_context_tab_bar_color                : "#29292A"
            dock_context_tab_button_default_color     : "#2E2E30"
            dock_context_tab_button_checked_color     : "#414143"
            dock_context_tab_button_text_default_color: "#7A7A82"
            dock_context_tab_button_text_checked_color: "#FFFFFF"
            dock_border_color                         : "#262626"
            // ---------- dock [end] ----------

            // ---------- model library [beg] ----------
            model_library_border_color                      : "#67676D"

            model_library_type_button_default_color         : "transparent"
            model_library_type_button_checked_color         : "#1E9BE2"
            model_library_type_button_text_default_color    : "#CBCBCC"
            model_library_type_button_text_checked_color    : "#FFFFFF"

            model_library_back_button_default_color         : "transparent"
            model_library_back_button_checked_color         : "#1E9BE2"
            model_library_back_button_border_default_color  : "#67676D"
            model_library_back_button_border_checked_color  : "#1E9BE2"
            model_library_back_button_text_default_color    : "#BABABA"
            model_library_back_button_text_checked_color    : "#FFFFFF"
            model_library_back_button_default_image         : "qrc:/UI/photo/model_library_detail_back.png"
            model_library_back_button_checked_image         : "qrc:/UI/photo/model_library_detail_back_h.png"

            model_library_action_button_default_color       : "transparent"
            model_library_action_button_checked_color       : "#1E9BE2"
            model_library_action_button_border_default_color: "#67676D"
            model_library_action_button_border_checked_color: "#1E9BE2"
            model_library_action_button_text_default_color  : "#BABABA"
            model_library_action_button_text_checked_color  : "#FFFFFF"

            model_library_import_button_default_color       : "#1E9BE2"
            model_library_import_button_checked_color       : "#1EB6E2"

            model_library_special_text_color                : "#FFFFFF"
            model_library_general_text_color                : "#BABABA"

            model_library_item_default_color                : "transparent"
            model_library_item_hovered_color                : "#5F5F62"
            model_library_item_checked_color                : "#1E9BE2"
            model_library_item_text_default_color           : "#FFFFFF"
            model_library_item_text_checked_color           : "#FFFFFF"

            model_library_license_default_color             : "#1E9BE2"
            model_library_license_checked_color             : "#1EB6E2"
            model_library_license_default_image             : "qrc:/UI/photo/model_library_license_h.png"
            model_library_license_checked_image             : "qrc:/UI/photo/model_library_license_h.png"

            model_library_import_default_image              : "qrc:/UI/photo/model_library_import.png"
            model_library_import_checked_image              : "qrc:/UI/photo/model_library_import_h.png"
            model_library_download_default_image            : "qrc:/UI/photo/model_library_download.png"
            model_library_download_checked_image            : "qrc:/UI/photo/model_library_download_h.png"
            model_library_uncollect_default_image           : "qrc:/UI/photo/model_library_uncollect.png"
            model_library_uncollect_checked_image           : "qrc:/UI/photo/model_library_uncollect_h.png"
            model_library_shared_default_image              : "qrc:/UI/photo/model_library_share.png"
            model_library_shared_checked_image              : "qrc:/UI/photo/model_library_share_h.png"
            // ---------- model library [end] ----------

            // ---------- custom tabview [beg] ----------
            custom_tabview_border_color             : "#2B2B2B"
            custom_tabview_panel_color              : "#414143"
            custom_tabview_button_color             : "#4B4B4D"
            custom_tabview_button_text_default_color: "#A5A5AE"
            custom_tabview_button_text_checked_color: "#FFFFFF"
            custom_tabview_combo_item_hovered_color : "#739AB0"
            // ---------- custom tabview [end] ----------

            // ---------- manager printer dialog [beg] ----------
            manager_printer_switch_default_color: "#FFFFFF"
            manager_printer_switch_checked_color: "#00A3FF"
            manager_printer_switch_default_image: "qrc:/UI/photo/printer_switch_dark_default.png"
            manager_printer_switch_checked_image: "qrc:/UI/photo/printer_switch_dark_checked.png"

            manager_printer_tabview_default_color: "#CBCBCC"
            manager_printer_tabview_checked_color: "#FFFFFF"

            manager_printer_label_color: "#CBCBCC"

            manager_printer_gcode_title_color: "#CBCBCC"
            manager_printer_gcode_text_color: "#FFFFFF"

            manager_printer_button_text_color: "#DBDBDC"
            manager_printer_button_border_color: "transparent"
            manager_printer_button_default_color: "#6E6E73"
            manager_printer_button_checked_color: "#86868A"
            // ---------- manager printer dialog [end] ----------

            // ---------- download manage dialig [beg] ----------
            downloadbtn_image: "qrc:/UI/photo/download_btn_dark.png"
            downloadbtn_image_hovered: "qrc:/UI/photo/download_btn_dark.png"
            downloadbtn_tip_image: "qrc:/UI/photo/download_btntip_dark.png"
            downloadbtn_tip_color: "#FFFFFF"

            downloadbtn_finished_default_color: "transparent"
            downloadbtn_finished_hovered_color: "#000000"
            downloadbtn_finished_text_default_color: "#9F9FA2"
            downloadbtn_finished_text_hovered_color: "#9F9FA2"
            downloadbtn_finished_border_default_color: "#515154"
            downloadbtn_finished_border_hovered_color: "#000000"

            downloadbtn_download_default_color: "#FFFFFF"
            downloadbtn_download_hovered_color: "#FFFFFF"
            downloadbtn_download_text_default_color: "#000000"
            downloadbtn_download_text_hovered_color: "#000000"
            downloadbtn_download_border_default_color: "#3E3E3E"
            downloadbtn_download_border_hovered_color: "#1E9BE2"

            download_manage_tab_button_color: "#4B4B4D"
            download_manage_title_text_color: "#CBCBCC"
            download_manage_model_text_color: "#FFFFFF"
            download_manage_prograss_left_color: "#1E9BE2"
            download_manage_prograss_right_color: "#3C3C3E"
            download_manage_group_open_image: "qrc:/UI/photo/treeView_plus_dark.png"
            download_manage_group_close_image: "qrc:/UI/photo/treeView_minus_dark.png"

            download_manage_empty_button_image: "qrc:/UI/photo/cloud_logo.svg"
            download_manage_empty_text_color: "#DBDBDC"
            download_manage_empty_button_default_color: "#6E6E73"
            download_manage_empty_button_checked_color: "#86868A"
            download_manage_empty_button_text_default_color: "#FFFFFF"
            download_manage_empty_button_text_checked_color: "#FFFFFF"
            // ---------- download manage dialig [end] ----------

            // ---------- left tool bar [end] ----------
            leftbar_rocommand_btn_icon_default: "qrc:/UI/photo/leftBar/recommand_dark.svg"
            leftbar_rocommand_btn_icon_hovered: "qrc:/UI/photo/leftBar/recommand_pressed.svg"
            leftbar_rocommand_btn_icon_pressed: "qrc:/UI/photo/leftBar/recommand_pressed.svg"
            leftbar_open_btn_icon_default: "qrc:/UI/photo/leftBar/open_dark.svg"
            leftbar_open_btn_icon_hovered: "qrc:/UI/photo/leftBar/open_pressed.svg"
            leftbar_open_btn_icon_pressed: "qrc:/UI/photo/leftBar/open_pressed.svg"
            leftbar_pick_btn_icon_default: "qrc:/UI/photo/leftBar/pick_dark.svg"
            leftbar_pick_btn_icon_hovered: "qrc:/UI/photo/leftBar/pick_pressed.svg"
            leftbar_pick_btn_icon_pressed: "qrc:/UI/photo/leftBar/pick_pressed.svg"
            leftbar_other_btn_icon_default: "qrc:/UI/photo/leftBar/other_dark.svg"
            leftbar_other_btn_icon_hovered: "qrc:/UI/photo/leftBar/other_hovered_dark.svg"
            leftbar_other_btn_icon_checked: "qrc:/UI/photo/leftBar/other_checked.svg"
            leftbar_btn_border_color: "#2B2B2D"
            // ---------- left tool bar [end] ----------

            // ---------- basic compenent new [beg] ----------
            switch_border_color: "#343434"
            switch_indicator_color: "#17CC5F"
            switch_background_color: "#343434"
            switch_indicator_text_color: "#FFFFFF"
            switch_background_text_color: "#C9C9C9"
            // ---------- basic compenent new [end] ----------

            // ---------- parameter [beg] ----------
            parameter_text_color: "#C5C5CA"
            parameter_text_modifyed_color: "#FFD200"
            parameter_group_text_color: "#FFFFFF"
            parameter_unit_text_color: "#A5A5AE"
            parameter_editer_modifyed_color: "#787565"
            parameter_reset_button_image: "qrc:/UI/photo/parameter/reset_dark.svg"
            // ---------- parameter [end] ----------
        }
        ListElement {
            name: "Light Theme"
            modifier:"qrc:/UI/photo/modelGroup/modifier_light.svg"
            negative:"qrc:/UI/photo/modelGroup/negative_light.svg"
            normal:"qrc:/UI/photo/modelGroup/normal_light.svg"
            normalRelief:"qrc:/UI/photo/modelGroup/normalRelief_light.svg"
            modifierRelief:"qrc:/UI/photo/modelGroup/modifierRelief_light.svg"
            negativeRelief:"qrc:/UI/photo/modelGroup/negativeRelief_light.svg"
            supportDefender:"qrc:/UI/photo/modelGroup/supportDefender_light.svg"
            supportGenerator:"qrc:/UI/photo/modelGroup/supportGenerator_light.svg"
            mainBackgroundColor:"#F2F2F5"
            dropShadowColor : "#BBBBBB"
            headerBackgroundColor:"#D6D6DC"//"#333333"
            headerBackgroundColorEnd: "#D6D6DC"
            topToolBarColor:"#F2F2F5"
            topToolBarColorEnd:"#F2F2F5"
            menuBarBtnColor: "transparent"
            menuBarBtnColor_hovered: "#3E3E3E"
            menuTextColor: "#333333"
            menuTextColor_hovered: "#FFFFFF"
            menuTextColor_normal: "#FFFFFF"
            menuStyleBgColor: "#FFFFFF"
            menuStyleBgColor_hovered: "#17CC5F"
            themeColor: "#FFFFFF"//"#485359"
            textColor: "#333333"//"#373737"
            buttonColor : "#FFFFFF"
            dialogBtnHoveredColor: "#ADADAD"
            secondBtnHoverdColor: "#ECECEC"
            secondBtnSelectionColor: "#58CD9F"
            hoveredColor : "#1E9BE2"
            selectionColor : "#3A3A3A"
            rectBorderColor : "#CBCBCC"
            textBackgroundColor : "#F9F9F9"
            tipBackgroundColor: "#ffffff"
            tipBorderColor: "#767676"
            controlColor: "#f5f5f6"
            controlBorderColor: "#cbcbcb"
            aboutDiaItemRectBgColor: "#535353"

            //lsg
            //LeftToolBar Btn
            leftBtnBgColor_normal:  "#FFFFFF"
            leftBtnBgColor_hovered: "#DBF2FC"
            leftBtnBgColor_selected: "#17CC5F"
            leftToolBtnColor_normal: "#FFFFFF"
            leftToolBtnColor_hovered: "#ECECEC"
            leftTextColor: "#333333"
            leftTextColor_d: "white"
            leftBarBgColor: "#D6D6DC"
            leftTabBgColor: "#FFFFFF"
            topBtnBgColor_normal:  "#1E9BE2"
            topBtnBgColor_hovered: "#DBF2FC"
            topBtnBorderColor_normal: "#D6D6DC"
            topBtnBordeColor_hovered: "transparent"
            topBtnTextColor_normal : "#333333"
            topBtnTextColor_hovered : "#333333"
            topBtnTextColor_checked : "#FFFFFF"

            itemBackgroundColor : "#FFFFFF"//"#485359"
            infoPanelColor: "#333333"

            dialogTitleColor : "#D6D6DC"
            dialogContentBgColor: "#FFFFFF"
            dialogTitleTextColor:"#333333"
            dialogBorderColor : "transparent"

            searchBtnDisableColor: "#E5E5E9"
            searchBtnNormalColor: "#1E9BE2"
            searchBtnHoveredColor: "#1EB6E2"
            typeBtnHoveredColor : "#ECECEC"

            dialogItemRectBgColor: "#FFFFFF"
            dialogItemRectBgBorderColor: "#CBCBCC"

            splitLineColor : "#DFDFE3"
            splitLineColorDark : "#FFFFFF"
            radioCheckLabelColor: "#333333"
            radioCheckRadiusColor: "#ffffff"
            radioInfillBgColor: "#00a3ff"
            radioBorderColor: "#828790"
            profileBtnColor: "#ECECEC"
            lpw_BtnColor: "#ECECEC"
            profileBtnHoverColor: "#C2C2C5"
            tooltipBgColor: "#F9F9F9"
            checkBoxBgColor: "#F9F9F9"
            checkBoxBorderColor: "#828790"
            cmbIndicatorRectColor_pressed_basic : "#1E9BE2"
            cmbIndicatorRectColor_basic: "transparent"

            menuSplitLineColor: "#DCDCDC"
            menuSplitLineColorleft: "#E5E5E9"
            mainViewSplitLineColor: "#cacaca"
            printerCombRectColor: "#C5C5CA"

            treeBackgroundColor: "#F9F9F9"
            treeItemColor:"#F9F9F9"
            treeItemColor_pressed: "#D8D8DA"

            progressBarBgColor: "#DFDFE4"
            previewPanelRecgColor: "#959596"
            previewPanelTextgColor: "#333333"
            tabButtonSelectColor: "#F9F9F9"
            tabButtonNormalColor: "#C5C5CA"

            switchModelBgColor: "#C5C5CA"
            switchModeSelectedlBgColor: "#FFFFFF"
            sliderTopColor1: "#20ACFB"
            sliderTopColor2: "#20ACFB"
            sliderBottomColor: "#D8D8DC"
            modelAlwaysMoreBtnBgColor: "#FFFFFF"
            modelAlwaysItemBorderColor: "#727273"

            cube3DAmbientColor: "#B8B8B8"
            cube3DAmbientColor_Entered:"#A8A8A8"
            textColor_disabled:"#AFAFAF"
            printerCommboxBtnDefaultColor: "#D8D8DA"
            printerCommboxPopBgColor: "#F2F2F5"
            printerCommboxIndicatorBgColor: "#C5C5CA"
            printerCommboxIndicatorPopShowBgColor: "#C5C5CA"
            printerCommboxBgColor:"#DCDCDF"
            printerCommboxBgBorderColor: "#C5C5CA"

            enabledBtnShadowColor:"#C7C7CA"
            tittleBarBtnColor: "#ceced0"
            laserFoldTittleColor: "#D8D8DA"

            laserLineBorderColor:"#333333"

            upBtnImgSource: "qrc:/UI/photo/upBtn_white.png" // "qrc:/UI/photo/upBtn.svg"
            upBtnImgSource_d: "qrc:/UI/photo/upBtn_green.svg"
            downBtnImgSource: "qrc:/UI/photo/downBtn_white.png" //"qrc:/UI/photo/downBtn.svg"
            downBtnImgSource_d: "qrc:/UI/photo/downBtn_green.svg"

            clearBtnImg: "qrc:/UI/photo/clearBtn2.png"
            clearBtnImg_d: "qrc:/UI/photo/clearBtn2_white_d.png"
            searchBtnImg: "qrc:/UI/photo/search.png"
            searchBtnImg_d: "qrc:/UI/photo/search_white_d.png"
            downBtnImg: "qrc:/UI/photo/downBtn.svg"
            checkBtnImg: "qrc:/UI/images/check3.png"
            showPWNormalImg: "qrc:/UI/photo/showPW_light.png"
            hidePWNormalImg: "qrc:/UI/photo/hidePW_light.png"
            showPWHoveredImg: "qrc:/UI/photo/showPW_light.png"
            hidePWHoveredImg: "qrc:/UI/photo/hidePW_light.png"
            areaCodeComboboxImg: "qrc:/UI/photo/comboboxDown2.png"
            configTabBtnImg: "qrc:/UI/photo/configTabBtn2.png"
            configTabBtnImg_Hovered: "qrc:/UI/photo/configTabBtn2_d.png"
            supportTabBtnImg:"qrc:/UI/photo/supportTabBtn2.png"
            supportTabBtnImg_Hovered:"qrc:/UI/photo/supportTabBtn2_d.png"
            homeImg: "qrc:/UI/images/home2.png"
            homeImg_Hovered: "qrc:/UI/images/home2_s.png"

            laserPickImg: "qrc:/UI/images/laser_pick2.png"
            laserImageImg: "qrc:/UI/images/laser_img2.png"
            laserFontImg: "qrc:/UI/images/laser_font2.png"
            laserRectImg: "qrc:/UI/images/laser_rect2.png"
            laserArcImg: "qrc:/UI/images/laser_arc2.png"
            laserPickHoveredImg: "qrc:/UI/images/laser_pick2.png"
            laserImageHoveredImg: "qrc:/UI/images/laser_img2.png"
            laserFontHoveredImg: "qrc:/UI/images/laser_font2.png"
            laserRectHoveredImg: "qrc:/UI/images/laser_rect2.png"
            laserArcHoveredImg: "qrc:/UI/images/laser_arc2.png"
            laserPickCheckedImg: "qrc:/UI/images/laser_pick_s.png"
            laserImageCheckedImg: "qrc:/UI/images/laser_img_s.png"
            laserFontCheckedImg: "qrc:/UI/images/laser_font_s.png"
            laserRectCheckedImg: "qrc:/UI/images/laser_rect_s.png"
            laserArcCheckedImg: "qrc:/UI/images/laser_arc_s.png"

            switchModelFDMImg: "qrc:/UI/images/fdmMode2.png"
            switchModelLaserImg: "qrc:/UI/images/laserMode2.png"
            switchModelFDMImg_H: "qrc:/UI/images/fdmMode2_h.png"
            switchModelLaserImg_H: "qrc:/UI/images/laserMode2_h.png"
            switchModelLaserImgEn: "qrc:/UI/images/laserMode2_en.png"
            switchModelLaserImgEn_H: "qrc:/UI/images/laserMode2_h_en.png"
            switchModelCNCImg: "qrc:/UI/images/CNCMode2.png"
            switchModelCNCImg_H: "qrc:/UI/images/CNCMode2_h.png"

            printMoveAxisYUpImg: "qrc:/UI/photo/print_move_axis_y+2.png"
            printMoveAxisYUp_HImg: "qrc:/UI/photo/print_move_axis_y+2_h.png"
            printMoveAxisYUp_CImg: "qrc:/UI/photo/print_move_axis_y+2_c.png"
            printMoveAxisZUpImg: "qrc:/UI/photo/print_move_axis_z+2.png"
            printMoveAxisZUp_HImg: "qrc:/UI/photo/print_move_axis_z+2_h.png"
            printMoveAxisZUp_CImg: "qrc:/UI/photo/print_move_axis_z+2_c.png"
            printMoveAxisXDownImg: "qrc:/UI/photo/print_move_axis_x-2.png"
            printMoveAxisXDown_HImg: "qrc:/UI/photo/print_move_axis_x-2_h.png"
            printMoveAxisXDown_CImg: "qrc:/UI/photo/print_move_axis_x-2_c.png"
            printMoveAxisZeroImg: "qrc:/UI/photo/print_move_axis_zero2.png"
            printMoveAxisZero_HImg: "qrc:/UI/photo/print_move_axis_zero2_h.png"
            printMoveAxisZero_CImg: "qrc:/UI/photo/print_move_axis_zero2_c.png"
            printMoveAxisXUpImg: "qrc:/UI/photo/print_move_axis_x+2.png"
            printMoveAxisXUp_HImg: "qrc:/UI/photo/print_move_axis_x+2_h.png"
            printMoveAxisXUp_CImg: "qrc:/UI/photo/print_move_axis_x+2_c.png"
            printMoveAxisYDownImg: "qrc:/UI/photo/print_move_axis_y-2.png"
            printMoveAxisYDown_HImg: "qrc:/UI/photo/print_move_axis_y-2_h.png"
            printMoveAxisYDown_CImg: "qrc:/UI/photo/print_move_axis_y-2_c.png"
            printMoveAxisZDownImg: "qrc:/UI/photo/print_move_axis_z-2.png"
            printMoveAxisZDown_HImg: "qrc:/UI/photo/print_move_axis_z-2_h.png"
            printMoveAxisZDown_CImg: "qrc:/UI/photo/print_move_axis_z-2_c.png"

            modelAlwaysPopBg: "qrc:/UI/photo/model_always_pop_bg2.png"
            modelAlwaysBtnIcon: "qrc:/UI/photo/model_always_show2.png"
            uploadModelImg: "qrc:/UI/photo/upload_model_img2.png"
            uploadLocalModelImg: "qrc:/UI/photo/upload_model_local_img2.png"
            deleteModelImg: "qrc:/UI/photo/delete_model_img2.png"

            editProfileImg: "qrc:/UI/photo/editProfile2.png"
            uploadProfileImg: "qrc:/UI/photo/uploadPro.png"
            deleteProfileImg: "qrc:/UI/photo/deleteProfile.png"

            onSrcImg: "qrc:/UI/photo/on2.png"
            offSrcImg: "qrc:/UI/photo/off2.png"
            checkedSrcImg: "qrc:/UI/photo/radio2_1.png"
            uncheckSrcImg: "qrc:/UI/photo/radio2_2.png"

            cube3DTopImg: "qrc:/UI/images/top2.png"
            cube3DTopImg_C: "qrc:/UI/images/top2_C.png"
            cube3DFrontImg: "qrc:/UI/images/front2.png"
            cube3DFrontImg_C: "qrc:/UI/images/front2_C.png"
            cube3DBottomImg: "qrc:/UI/images/bottom2.png"
            cube3DBottomImg_C: "qrc:/UI/images/bottom2_C.png"
            cube3DBackImg: "qrc:/UI/images/back2.png"
            cube3DBackImg_C: "qrc:/UI/images/back2_C.png"
            cube3DLeftkImg: "qrc:/UI/images/left2.png"
            cube3DLeftkImg_C: "qrc:/UI/images/left2_C.png"
            cube3DLeftkRight: "qrc:/UI/images/right2.png"
            cube3DLeftkRight_C: "qrc:/UI/images/right2_C.png"
            laserFoldTitleUpImg: "qrc:/UI/photo/laser_fold_item_up2.png"
            laserFoldTitleDownImg: "qrc:/UI/photo/laser_fold_item_down2.png"

            // ---------- left panel [beg] ----------
            left_model_list_button_default_color: "#F2F2F5"
            left_model_list_button_hovered_color: "#DBF2FC"
            left_model_list_button_border_default_color: "#D6D6DC"
            left_model_list_button_border_hovered_color: "#D6D6DC"
            left_model_list_button_default_icon: "qrc:/UI/photo/modelListIcon_lite.svg"
            left_model_list_button_hovered_icon: "qrc:/UI/photo/modelListIcon_lite.svg"
            left_model_list_count_background_color: "#1E9BE2"
            left_model_list_count_text_color: "#F2F2F5"

            left_model_list_background_color: "#F2F2F5"
            left_model_list_border_color: "#D6D6DC"
            left_model_list_title_text_color: "#333333"

            left_model_list_close_button_default_color: "transparent"
            left_model_list_close_button_hovered_color: "#FF365C"
            left_model_list_close_button_default_icon: "qrc:/UI/photo/closeBtn.svg"
            left_model_list_close_button_hovered_icon: "qrc:/UI/photo/closeBtn_d.svg"

            left_model_list_all_button_border_color: "#1E9BE2"
            left_model_list_all_button_text_color: "#333333"
            left_model_list_all_button_icon: "qrc:/UI/images/check3.png"

            left_model_list_action_button_default_color: "transparent"
            left_model_list_action_button_hovered_color: "#ECECED"
            left_model_list_del_button_default_icon: "qrc:/UI/photo/deleteModel_light.png"
            left_model_list_del_button_hovered_icon: "qrc:/UI/photo/deleteModel_light.png"

            left_model_list_item_default_color: "transparent"
            left_model_list_item_hovered_color: "#B7E5FF"
            left_model_list_item_text_default_color: "#333333"
            left_model_list_item_text_hovered_color: "#333333"
            // ---------- left panel [end] ----------

            // ---------- right panel [beg] ----------

            right_panel_text_default_color         : "#333333"
            right_panel_text_disable_color         : "#333333"
            right_panel_text_hovered_color         : "#333333"
            right_panel_text_checked_color         : "#333333"

            right_panel_gcode_text_color           : "#999999"

            right_panel_button_default_color       : "#FFFFFF"
            right_panel_button_disable_color       : "#FFFFFF"
            right_panel_button_hovered_color       : "#DCDCE4"
            right_panel_button_checked_color       : "#DCDCE4"

            right_panel_border_default_color       : "#D6D6DC"
            right_panel_border_disable_color       : "#D6D6DC"
            right_panel_border_hovered_color       : "#17CC5F"
            right_panel_border_checked_color       : "#17CC5F"

            right_panel_item_default_color         : "#FFFFFF"
            right_panel_item_disable_color         : "#FFFFFF"
            right_panel_item_hovered_color         : "#D6D6DC"
            right_panel_item_checked_color         : "#17CC5F"
            right_panel_item_text_default_color    : "#333333"
            right_panel_item_text_disable_color    : "#333333"
            right_panel_item_text_hovered_color    : "#333333"
            right_panel_item_text_checked_color    : "#333333"

            right_panel_combobox_background_color  : "#FFFFFF"

            right_panel_tab_text_default_color     : "#8D8D90"
            right_panel_tab_text_disable_color     : "#8D8D90"
            right_panel_tab_text_hovered_color     : "#FFFFFF"
            right_panel_tab_text_checked_color     : "#FFFFFF"
            right_panel_tab_button_default_color   : "#FFFFFF"
            right_panel_tab_button_disable_color   : "#FFFFFF"
            right_panel_tab_button_hovered_color   : "#1E9BE2"
            right_panel_tab_button_checked_color   : "#1E9BE2"
            right_panel_tab_background_color       : "#FFFFFF"

            right_panel_menu_border_color          : "#D6D6DC"
            right_panel_menu_background_color      : "#FFFFFF"
            right_panel_menu_split_line_color      : "#D6D6DC"

            right_panel_slice_button_default_color : "#1E9BE2"
            right_panel_slice_button_disable_color : "#DCDCE4"
            right_panel_slice_button_hovered_color : "#57B4E9"
            right_panel_slice_button_checked_color : "#57B4E9"
            right_panel_slice_text_default_color   : "#FFFFFF"
            right_panel_slice_text_disable_color   : "#FFFFFF"
            right_panel_slice_text_hovered_color   : "#FFFFFF"
            right_panel_slice_text_checked_color   : "#FFFFFF"
            right_panel_bgColor                    : "#ffffff"

            right_panel_quality_custom_default_image  : "qrc:/UI/photo/config_quality_custom_default.svg"
            right_panel_quality_custom_checked_image  : "qrc:/UI/photo/config_quality_custom_checked.svg"
            right_panel_quality_high_default_image    : "qrc:/UI/photo/config_quality_high_default.svg"
            right_panel_quality_high_checked_image    : "qrc:/UI/photo/config_quality_high_checked.svg"
            right_panel_quality_middle_default_image  : "qrc:/UI/photo/config_quality_middle_default.svg"
            right_panel_quality_middle_checked_image  : "qrc:/UI/photo/config_quality_middle_checked.svg"
            right_panel_quality_low_default_image     : "qrc:/UI/photo/config_quality_low_default.svg"
            right_panel_quality_low_checked_image     : "qrc:/UI/photo/config_quality_low_checked.svg"
            right_panel_quality_verylow_default_image : "qrc:/UI/photo/config_quality_verylow_default.svg"
            right_panel_quality_verylow_checked_image : "qrc:/UI/photo/config_quality_verylow_checked.svg"

            right_panel_process_level_image: "qrc:/UI/photo/rightDrawer/process_level_light.svg"
            right_panel_process_custom_image: "qrc:/UI/photo/rightDrawer/process_custom_light.svg"

            right_panel_delete_image: "qrc:/UI/photo/rightDrawer/delete.svg"
            right_panel_delete_hovered_image: "qrc:/UI/photo/rightDrawer/delete_hovered.svg"
            right_panel_edit_image: "qrc:/UI/photo/rightDrawer/edit.svg"
            right_panel_edit_hovered_image: "qrc:/UI/photo/rightDrawer/edit_hovered.svg"
            right_panel_save_image: "qrc:/UI/photo/rightDrawer/save_light.svg"
            right_panel_save_disabled_image: "qrc:/UI/photo/rightDrawer/save_light_disabled.svg"
            right_panel_reset_image: "qrc:/UI/photo/rightDrawer/reset_light.svg"
            right_panel_reset_disabled_image: "qrc:/UI/photo/rightDrawer/reset_light_disabled.svg"

            // ---------- right panel [end] ----------

            //lanPrinter Panel
            lanPrinter_panel_border: "#D6D6DC"
            lanPrinter_panel_light_txt: "#666666"
            lanPrinter_panel_weight_txt: "#333333"
            lanPrinter_panel_background: "#F2F2F5"
            lanPrinter_panel_btn_default: "#E8E8ED"
            lanPrinter_panel_btn_hovered: "#FFFFFF"

            //leftPopWin
            themeTextColor: "#333333"
            themeColor_primary: "#ffffff"
            themeColor_secondary: "#dddde1"
            themeColor_third: "#ffffff"
            addModel: "qrc:/UI/photo/addModel_light.png"
            delModel: "qrc:/UI/photo/deleteModel_light.png"
            drawerBgImg: "qrc:/UI/photo/rightDrawer/drawerBg_light.svg"

            // ---------- dock [beg] ----------
            dock_context_tab_bar_color                : "#F2F2F5"
            dock_context_tab_button_default_color     : "#E8E8ED"
            dock_context_tab_button_checked_color     : "#FFFFFF"
            dock_context_tab_button_text_default_color: "#666666"
            dock_context_tab_button_text_checked_color: "#333333"
            dock_border_color                         : "#DDDDE1"
            // ---------- dock [end] ----------

            // ---------- model library [beg] ----------
            model_library_border_color                      : "#D6D6DC"

            model_library_type_button_default_color         : "transparent"
            model_library_type_button_checked_color         : "#1E9BE2"
            model_library_type_button_text_default_color    : "#333333"
            model_library_type_button_text_checked_color    : "#FFFFFF"

            model_library_back_button_default_color         : "transparent"
            model_library_back_button_checked_color         : "#1E9BE2"
            model_library_back_button_border_default_color  : "#D6D6DC"
            model_library_back_button_border_checked_color  : "#1E9BE2"
            model_library_back_button_text_default_color    : "#87878E"
            model_library_back_button_text_checked_color    : "#FFFFFF"
            model_library_back_button_default_image         : "qrc:/UI/photo/model_library_detail_back.png"
            model_library_back_button_checked_image         : "qrc:/UI/photo/model_library_detail_back_h.png"

            model_library_action_button_default_color       : "#E2F5FF"
            model_library_action_button_checked_color       : "#1E9BE2"
            model_library_action_button_border_default_color: "#E2F5FF"
            model_library_action_button_border_checked_color: "#1E9BE2"
            model_library_action_button_text_default_color  : "#1E9BE2"
            model_library_action_button_text_checked_color  : "#FFFFFF"

            model_library_import_button_default_color       : "#1E9BE2"
            model_library_import_button_checked_color       : "#1EB6E2"

            model_library_special_text_color                : "#333333"
            model_library_general_text_color                : "#666666"

            model_library_item_default_color                : "transparent"
            model_library_item_hovered_color                : "#E1E1E1"
            model_library_item_checked_color                : "#1E9BE2"
            model_library_item_text_default_color           : "#333333"
            model_library_item_text_checked_color           : "#FFFFFF"

            model_library_license_default_color             : "#E2F5FF"
            model_library_license_checked_color             : "#1E9BE2"
            model_library_license_default_image             : "qrc:/UI/photo/model_library_license.png"
            model_library_license_checked_image             : "qrc:/UI/photo/model_library_license_h.png"

            model_library_import_default_image              : "qrc:/UI/photo/model_library_import_l.png"
            model_library_import_checked_image              : "qrc:/UI/photo/model_library_import_h.png"
            model_library_download_default_image            : "qrc:/UI/photo/model_library_download_l.png"
            model_library_download_checked_image            : "qrc:/UI/photo/model_library_download_h.png"
            model_library_uncollect_default_image           : "qrc:/UI/photo/model_library_uncollect_l.png"
            model_library_uncollect_checked_image           : "qrc:/UI/photo/model_library_uncollect_h.png"
            model_library_shared_default_image              : "qrc:/UI/photo/model_library_share_l.png"
            model_library_shared_checked_image              : "qrc:/UI/photo/model_library_share_h.png"
            // ---------- model library [end] ----------

            // ---------- custom tabview [beg] ----------
            custom_tabview_border_color             : "#CBCBCC"
            custom_tabview_panel_color              : "#FFFFFF"
            custom_tabview_button_color             : "#FFFFFF"
            custom_tabview_button_text_default_color: "#333333"
            custom_tabview_button_text_checked_color: "#333333"
            custom_tabview_combo_item_hovered_color : "#B7E5FF"
            // ---------- custom tabview [end] ----------

            // ---------- manager printer dialog [beg] ----------
            manager_printer_switch_default_color: "#333333"
            manager_printer_switch_checked_color: "#333333"
            manager_printer_switch_default_image: "qrc:/UI/photo/printer_switch_lite_default.png"
            manager_printer_switch_checked_image: "qrc:/UI/photo/printer_switch_lite_checked.png"

            manager_printer_tabview_default_color: "#333333"
            manager_printer_tabview_checked_color: "#333333"

            manager_printer_label_color: "#333333"

            manager_printer_gcode_title_color: "#333333"
            manager_printer_gcode_text_color: "#333333"

            manager_printer_button_text_color: "#333333"
            manager_printer_button_border_color: "#DDDDE1"
            manager_printer_button_default_color: "#FFFFFF"
            manager_printer_button_checked_color: "#DDDDE1"
            // ---------- manager printer dialog [end] ----------

            // ---------- download manage dialig [beg] ----------
            downloadbtn_image: "qrc:/UI/photo/download_btn_dark.png"
            downloadbtn_image_hovered: "qrc:/UI/photo/download_btn_lite.png"
            downloadbtn_tip_image: "qrc:/UI/photo/download_btntip_dark.png"
            downloadbtn_tip_color: "#CAEBFD"

            downloadbtn_finished_default_color: "transparent"
            downloadbtn_finished_hovered_color: "#1E9BE2"
            downloadbtn_finished_text_default_color: "#7D7D82"
            downloadbtn_finished_text_hovered_color: "#1E9BE2"
            downloadbtn_finished_border_default_color: "#DCDCDF"
            downloadbtn_finished_border_hovered_color: "#1E9BE2"

            downloadbtn_download_default_color: "#FFFFFF"
            downloadbtn_download_hovered_color: "#FFFFFF"
            downloadbtn_download_text_default_color: "#000000"
            downloadbtn_download_text_hovered_color: "#000000"
            downloadbtn_download_border_default_color: "#CECED0"
            downloadbtn_download_border_hovered_color: "#1E9BE2"

            download_manage_tab_button_color: "#F4F4F4"
            download_manage_title_text_color: "#333333"
            download_manage_model_text_color: "#333333"
            download_manage_prograss_left_color: "#1E9BE2"
            download_manage_prograss_right_color: "#CECECF"
            download_manage_group_open_image: "qrc:/UI/photo/treeView_plus_light.png"
            download_manage_group_close_image: "qrc:/UI/photo/treeView_minus_light.png"

            download_manage_empty_button_image: "qrc:/UI/photo/cloud_logo.svg"
            download_manage_empty_text_color: "#333333"
            download_manage_empty_button_default_color: "#E2F5FF"
            download_manage_empty_button_checked_color: "#A9E2FF"
            download_manage_empty_button_text_default_color: "#333333"
            download_manage_empty_button_text_checked_color: "#333333"
            // ---------- download manage dialig [end] ----------


            // ---------- left tool bar [end] ----------
            leftbar_rocommand_btn_icon_default: "qrc:/UI/photo/leftBar/recommand_lite.svg"
            leftbar_rocommand_btn_icon_hovered: "qrc:/UI/photo/leftBar/recommand_lite.svg"
            leftbar_rocommand_btn_icon_pressed: "qrc:/UI/photo/leftBar/recommand_pressed.svg"
            leftbar_open_btn_icon_default: "qrc:/UI/photo/leftBar/open_lite.svg"
            leftbar_open_btn_icon_hovered: "qrc:/UI/photo/leftBar/open_lite.svg"
            leftbar_open_btn_icon_pressed: "qrc:/UI/photo/leftBar/open_pressed.svg"
            leftbar_pick_btn_icon_default: "qrc:/UI/photo/leftBar/pick_lite.svg"
            leftbar_pick_btn_icon_hovered: "qrc:/UI/photo/leftBar/pick_lite.svg"
            leftbar_pick_btn_icon_pressed: "qrc:/UI/photo/leftBar/pick_pressed.svg"
            leftbar_other_btn_icon_default: "qrc:/UI/photo/leftBar/other_light.svg"
            leftbar_other_btn_icon_hovered: "qrc:/UI/photo/leftBar/other_light.svg"
            leftbar_other_btn_icon_checked: "qrc:/UI/photo/leftBar/other_checked.svg"
            leftbar_btn_border_color: "#D6D6DC"
            // ---------- left tool bar [end] ----------

            // ---------- basic compenent new [beg] ----------
            switch_border_color: "#D6D6DC"
            switch_indicator_color: "#17CC5F"
            switch_background_color: "#FFFFFF"
            switch_indicator_text_color: "#FFFFFF"
            switch_background_text_color: "#333333"
            // ---------- basic compenent new [end] ----------

            // ---------- parameter [beg] ----------
            parameter_text_color: "#333333"
            parameter_text_modifyed_color: "#FF6C00"
            parameter_group_text_color: "#333333"
            parameter_unit_text_color: "#333333"
            parameter_editer_modifyed_color: "#FFEBDC"
            parameter_reset_button_image: "qrc:/UI/photo/parameter/reset_light.svg"
            // ---------- parameter [end] ----------
        }
    }

    function getContrast(rgb1, rgb2) {
        const luminance = (rgb) => {
            let a = rgb.map((v) => {
                v /= 255;
                return v <= 0.03928
                    ? v / 12.92
                    : Math.pow((v + 0.055) / 1.055, 2.4);
            });
            return 0.2126 * a[0] + 0.7152 * a[1] + 0.0722 * a[2];
        };
        let contrast = (luminance(rgb1) + 0.05) / (luminance(rgb2) + 0.05);
        if (contrast < 1) {
            contrast = 1 / contrast;
        }
        return contrast;
    }

    function setTextColor(jsColor) {
        let bgColor = [jsColor.r * 255, jsColor.g * 255, jsColor.b * 255];
        let whiteContrast = Constants.getContrast(bgColor, [255, 255, 255]);
        let blackContrast = Constants.getContrast(bgColor, [0, 0, 0]);
        if (whiteContrast > blackContrast) {
            return "#ffffff"
        } else {
            return "#000000"
        }
    }
}
