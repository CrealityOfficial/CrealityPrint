BUNDLE_ID="com.creality.creative3d"
USERNAME="dev@cxsw3d.com"
PASSWORD=$3
CURRENT_DIR=$1
BUILDTIME=`date "+%y%m%d%H%M"`
TARGET=$2
EXPORT_PATH="$CURRENT_DIR"
APP_PATH="$EXPORT_PATH/$TARGET.app"
ZIP_PATH="$EXPORT_PATH/${TARGET}-${BUILDTIME}.zip"
echo "APP_PATH=$APP_PATH"
echo "ZIP_PATH=$ZIP_PATH"
function uploadFileAndNotarized()
{
	echo "start notarized $1 ..."
	xcrun altool --notarize-app --primary-bundle-id "$BUNDLE_ID" --username "$USERNAME" --password "$PASSWORD" --file $1 &> tmp
	# 从日志文件中读取UUID,并隔一段时间检查一次公证结果
	# 只有成功的格式是 RequestUUID = 
	uuid=`cat tmp | grep -Eo 'RequestUUID = [[:alnum:]]{8}-([[:alnum:]]{4}-){3}[[:alnum:]]{12}' | grep -Eo '[[:alnum:]]{8}-([[:alnum:]]{4}-){3}[[:alnum:]]{12}' | sed -n "1p"`
	# 如果上传过了，则会返回 The upload ID is 
	if [[ "$uuid" == "" ]];then
		uuid=`cat tmp | grep -Eo 'The upload ID is [[:alnum:]]{8}-([[:alnum:]]{4}-){3}[[:alnum:]]{12}' | grep -Eo '[[:alnum:]]{8}-([[:alnum:]]{4}-){3}[[:alnum:]]{12}' | sed -n "1p"`
		echo "The software asset has already been uploaded. The upload ID is $uuid"
	fi
	echo "notarization UUID is $uuid"
	# 即没有上传成功，也没有上传过，则退出
	if [[ "$uuid" == "" ]]; then
		echo "No success no uploaded, unknown error"
		cat tmp  | awk 'END {print}'
		return 1
	fi
	
	while true; do
	    echo "checking for notarization..."
	 
	    xcrun altool --notarization-info "$uuid" --username "$USERNAME" --password "$PASSWORD" &> tmp
	    r=`cat tmp`
	    t=`echo "$r" | grep "success"`
	    f=`echo "$r" | grep "invalid"`
	    if [[ "$t" != "" ]]; then
	        echo "notarization done!"
	        xcrun stapler staple "$1"
	        # xcrun stapler staple "Great.dmg"
	        echo "stapler done!"
	        break
	    fi
	    if [[ "$f" != "" ]]; then
	        echo "Failed : $r"
	        return 1
	    fi
	    echo "not finish yet, sleep 1min then check again..."
	    sleep 60
	done
	return 0
}
appNotarization(){
	echo "####### Archive and Notarization Script / appNotarization start #######"
	# 第一步:公证app
	ditto -c -k --keepParent "$APP_PATH" "$ZIP_PATH"
	echo "####### Archive and Notarization Script / 完成打包流程 appAchive #######"
	uploadFileAndNotarized $ZIP_PATH
	if [ $? -ne 0 ];then
	echo "####### Archive and Notarization Script / appNotarization failed #######"
    #rm $ZIP_PATH
	return 1
	fi
	# 删除原始的app
	#rm -r "$APP_PATH"
    
	# 解压公证后的zip
	#unzip "$ZIP_PATH" -d "$EXPORT_PATH"
	# 对app进行stapler,uploadFileAndNotarized方法仅会对zip进行stapler
	#xcrun stapler staple "$APP_PATH"

	#NAME_APP_ZIP="$EXPORT_PATH/${TARGET}-${BUILDTIME}_Notarization.zip"

	#mv "$ZIP_PATH" "$NAME_APP_ZIP"
    rm $ZIP_PATH
	echo "####### Archive and Notarization Script / appNotarization end #######"
	return 0
}
appNotarization
