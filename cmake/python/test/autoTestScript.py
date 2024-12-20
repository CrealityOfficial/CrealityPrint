#!/bin/bash
JOB_URL="${JENKINS_URL}job/${JOB_NAME}"

string1=$BUILD_DISPLAY_NAME
string2=$JOB_BASE_NAME
nowTime=$(date "+%Y-%m-%d %H:%M:%S")

CRTDIR=$(pwd)
s1='/linux-build/bin/Release/out.csv'
sfiless=$CRTDIR$s1

chmod -R 777 $sfiless

cont=$(cat $sfiless|sed ':label;N;s/\n/\\n/;b label')
cont=${cont//,/------------->} 


curl -X POST -H "Content-Type: application/json" \
-d '{"msg_type":"post","content": {"post": {"zh_cn": {"title": "自动测试结果通知","content": [[{"tag": "text","text": "'"项目名称：$string2\n构建编号：第$BUILD_NUMBER次构建\n远程分支：$GIT_BRANCH\n构建状态：成功\n构建日期：$nowTime\n输出结果：\n$cont"'"}]]} } }}' \
https://open.feishu.cn/open-apis/bot/v2/hook/3a6df364-cccf-499b-a718-8d874ef141a2