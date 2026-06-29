pipeline {
    agent none

    options {
        disableConcurrentBuilds()
        skipDefaultCheckout(true)
        timestamps()
    }

    parameters {
        booleanParam(name: 'BUILD_WIN', defaultValue: true, description: 'Build Windows package')
        booleanParam(name: 'BUILD_MAC_X86', defaultValue: false, description: 'Build macOS x86_64 package')
        booleanParam(name: 'BUILD_MAC_ARM', defaultValue: false, description: 'Build macOS arm64 package')
        booleanParam(name: 'CLEAN_WORKSPACE', defaultValue: false, description: 'Delete Jenkins workspace before checkout')
        text(name: 'BRANCH', defaultValue: 'origin/release-260330', description: 'C3DSlicer branch to build')
        text(name: 'TAG_NAME', defaultValue: '7.1.0', description: 'Base version tag')
        choice(name: 'RTYPE', choices: ['Beta', 'Alpha', 'Beta1', 'Beta2', 'Dev', 'Release'], description: 'Release type')
        string(name: 'APP_NAME', defaultValue: 'CrealityPrint', description: 'App name')
        booleanParam(name: 'WEB_SYNC', defaultValue: true, description: 'Sync Community resources by WEB_BRANCH')
        text(name: 'NOTIFY', defaultValue: '1', description: '1 to enable Feishu notification')
        text(name: 'SLICER_HEADER', defaultValue: '1', description: 'libslicer3r cache flag used by mac packaging')
        text(name: 'SYNCPRESET', defaultValue: '1', description: '1 to generate presets')
        string(name: 'WEB_BRANCH', defaultValue: 'release-260330', description: 'CrealityCommunity branch')
        text(name: 'PACKAGE_TYPE', defaultValue: '0', description: 'Windows only. 0: nsis, 1: zip')
    }

    environment {
        COMMUNITY_REPO = 'ssh://jenkins@172.20.180.12:29418/yanfa4/CrealityCommunity'
        CORE_REPO = 'ssh://jenkins@172.20.180.12:29418/yanfa4/core/C3DSlicer'
        BUILD_SHARE_HOST = '172.20.180.14'
        BUILD_SHARE_PORT = '9122'
        BUILD_SHARE_ROOT = '/vagrant_data/www/shared/build'
        FEISHU_WEBHOOK_CRED = 'creality-release-feishu-webhook'
        ALPHA_PRESET_TOKEN_CRED = 'creality-cloud-preset-token'
        OPENCLAW_WEBHOOK_CRED = 'openclaw-webhook'
    }

    stages {
        stage('Windows') {
            when {
                expression { params.BUILD_WIN }
            }
            agent { label 'PackageServer_Win' }
            environment {
                QT5_DIR = 'F:\\Qt5.15.2\\lib\\cmake'
            }
            stages {
                stage('Checkout') {
                    steps {
                        script {
                            if (params.CLEAN_WORKSPACE) {
                                deleteDir()
                            }
                        }
                        checkout([
                            $class: 'GitSCM',
                            branches: [[name: "${params.BRANCH}"]],
                            doGenerateSubmoduleConfigurations: false,
                            extensions: [],
                            userRemoteConfigs: [[credentialsId: 'jenkins', url: "${env.CORE_REPO}"]]
                        ])
                        checkout([
                            $class: 'GitSCM',
                            branches: [[name: "${params.WEB_BRANCH}"]],
                            doGenerateSubmoduleConfigurations: false,
                            extensions: [[$class: 'RelativeTargetDirectory', relativeTargetDir: 'Community']],
                            userRemoteConfigs: [[credentialsId: 'jenkins', url: "${env.COMMUNITY_REPO}"]]
                        ])
                    }
                }

                stage('Sync Web Resources') {
                    steps {
                        bat '''
@echo off
if /I not "%WEB_SYNC%"=="true" exit /b 0
echo python
set defult_para_type=Beta
IF "%RTYPE%"=="Alpha" (
    set defult_para_type=Alpha
)

rmdir /s /q "%cd%\\resources"
git reset --hard

set web_file=.\\scripts\\sync_web.bat
if exist %web_file% (
    .\\scripts\\sync_web.bat
)
'''
                    }
                }

                stage('Build Package') {
                    steps {
                        script {
                            def buildScript = '''
@echo off
set defult_para_type=Beta
IF "%RTYPE%"=="Alpha" (
    set defult_para_type=Alpha
    if not "%CXY_TOKEN%"=="" (
        set cxy_token=%CXY_TOKEN%
    )
)

IF "%SYNCPRESET%"=="1" (
    python .\\scripts\\generate_creality_presets.py -b %defult_para_type% -n "3.0.0" || exit /b -1
)

echo %PATH%
SET C3D_BUILD_DIR=%cd%\\build_Release
IF EXIST %C3D_BUILD_DIR% (
    echo RD /S /Q %C3D_BUILD_DIR%
    RD /S /Q %C3D_BUILD_DIR%
) ELSE (
    echo HINT: %C3D_BUILD_DIR% NOT EXIST, skip remove
)

set package_name=zip
if "%PACKAGE_TYPE%"=="0" (
    set package_name=package
)

del *.zip
echo start package.bat
.\\scripts\\jenkins_package_build.bat %TAG_NAME% %package_name% %APP_NAME% %RTYPE% || exit /b -1
echo build_steps_end
'''

                            if (params.RTYPE == 'Alpha') {
                                withCredentials([string(credentialsId: env.ALPHA_PRESET_TOKEN_CRED, variable: 'CXY_TOKEN')]) {
                                    bat buildScript
                                }
                            } else {
                                bat buildScript
                            }

                            if (fileExists('var.prop')) {
                                readFile('var.prop').split(/\r?\n/).findAll { it?.trim() }.each { line ->
                                    def idx = line.indexOf('=')
                                    if (idx > 0) {
                                        env."${line.substring(0, idx)}" = line.substring(idx + 1)
                                    }
                                }
                            }
                        }
                    }
                }

                stage('Sign And Upload Debug Artifacts') {
                    steps {
                        bat '''
@echo off
git log --oneline --since="yesterday" > changes.txt
set SIGN_PACKAGE_FILE=%SIGN_PACKAGE_NAME%
if exist "%WORKSPACE%\\build_Release\\%SIGN_PACKAGE_NAME%" (
    set SIGN_PACKAGE_FILE=%WORKSPACE%\\build_Release\\%SIGN_PACKAGE_NAME%
)

if "%PACKAGE_TYPE%"=="1" (
    scp -P %BUILD_SHARE_PORT% "%SIGN_PACKAGE_FILE%" cxsw@%BUILD_SHARE_HOST%:%BUILD_SHARE_ROOT%/%JOB_NAME%/%SIGN_PACKAGE_NAME%
    exit /b 0
)

signtool verify /pa /q "%SIGN_PACKAGE_FILE%" || exit /b -1
"%WORKSPACE%\\tools\\7z.exe" a %SIGN_PACKAGE_NAME%.pdb.zip %WORKSPACE%\\build_Release\\src\\Release\\*.pdb
scp -P %BUILD_SHARE_PORT% %SIGN_PACKAGE_NAME%.pdb.zip cxsw@%BUILD_SHARE_HOST%:%BUILD_SHARE_ROOT%/%JOB_NAME%/%SIGN_PACKAGE_NAME%.pdb.zip

IF EXIST ".\\scripts\\breakpad.py" (
    cd %WORKSPACE%\\build_Release\\src\\Release
    python %WORKSPACE%\\scripts\\breakpad.py
    "%WORKSPACE%\\tools\\7z.exe" a %SIGN_PACKAGE_NAME%.sym.zip symbols
    scp -P %BUILD_SHARE_PORT% %SIGN_PACKAGE_NAME%.sym.zip cxsw@%BUILD_SHARE_HOST%:%BUILD_SHARE_ROOT%/%JOB_NAME%/%SIGN_PACKAGE_NAME%.sym.zip
    cd %WORKSPACE%
)
'''
                    }
                }

                stage('Archive Artifacts') {
                    steps {
                        archiveArtifacts artifacts: 'build_Release/*.zip,build_Release/*.exe,*.pdb.zip,build_Release/src/Release/*.sym.zip', onlyIfSuccessful: true
                    }
                }
            }
            post {
                success {
                    script {
                        def tagNumb = ''
                        if (fileExists('maxcmmid') && fileExists('tagcmmid')) {
                            tagNumb = bat(script: '''
@echo off
set /p MAXCMMID=<maxcmmid
set /p TAGCMMID=<tagcmmid
set /a TAGNUMB=%MAXCMMID%-%TAGCMMID%
echo %TAGNUMB%
''', returnStdout: true).trim()
                        }

                        def versionName = params.TAG_NAME
                        if (tagNumb) {
                            versionName = "${params.TAG_NAME}.${tagNumb}"
                        }
                        writeFile file: 'version.txt', text: "${versionName}-${params.RTYPE}\r\n"

                        if (params.NOTIFY == '1') {
                            try {
                                withCredentials([string(credentialsId: env.FEISHU_WEBHOOK_CRED, variable: 'FEISHU_WEBHOOK')]) {
                                    def packageUrl = "http://${env.BUILD_SHARE_HOST}/shared/build/${env.JOB_NAME}/${env.SIGN_PACKAGE_NAME}"
                                    def changesUrl = "${env.BUILD_URL}changes"
                                    def cardPayload = [
                                        msg_type: 'interactive',
                                        card    : [
                                            config : [wide_screen_mode: true],
                                            header : [
                                                template: 'green',
                                                title   : [tag: 'plain_text', content: "Windows package succeeded: ${params.APP_NAME} ${versionName}-${params.RTYPE}"]
                                            ],
                                            elements: [
                                                [tag: 'div', text: [tag: 'lark_md', content: """**Job**: ${env.JOB_NAME}
**Build**: #${env.BUILD_NUMBER}
**Branch**: ${params.BRANCH}
**Version**: ${versionName}-${params.RTYPE}
**Platform**: Windows
**Package**: ${env.SIGN_PACKAGE_NAME}"""]],
                                                [tag: 'action', actions: [
                                                    [tag: 'button', text: [tag: 'plain_text', content: 'Build'], type: 'primary', url: env.BUILD_URL],
                                                    [tag: 'button', text: [tag: 'plain_text', content: 'Changes'], type: 'default', url: changesUrl],
                                                    [tag: 'button', text: [tag: 'plain_text', content: 'Package'], type: 'default', url: packageUrl]
                                                ]]
                                            ]
                                        ]
                                    ]
                                    httpRequest(
                                        httpMode: 'POST',
                                        contentType: 'APPLICATION_JSON',
                                        ignoreSslErrors: true,
                                        quiet: false,
                                        requestBody: groovy.json.JsonOutput.toJson(cardPayload),
                                        url: "${FEISHU_WEBHOOK}",
                                        validResponseCodes: '100:399'
                                    )
                                }
                            } catch (err) {
                                echo "Skip Feishu notification: ${err.message}"
                            }
                        }

                        withCredentials([string(credentialsId: env.OPENCLAW_WEBHOOK_CRED, variable: 'OPENCLAW_WEBHOOK')]) {
                            httpRequest(
                                httpMode: 'POST',
                                contentType: 'APPLICATION_JSON',
                                ignoreSslErrors: true,
                                quiet: false,
                                requestBody: """{
"job": {
"name": "${env.JOB_NAME}",
"url": "${env.BUILD_URL}"
},
"build": {
"number": ${env.BUILD_NUMBER},
"status": "${currentBuild.currentResult}",
"url": "${env.BUILD_URL}"
}
}""",
                                url: "${OPENCLAW_WEBHOOK}",
                                validResponseCodes: '100:399'
                            )
                        }
                    }
                }
            }
        }

        stage('macOS') {
            when {
                expression { params.BUILD_MAC_X86 || params.BUILD_MAC_ARM }
            }
            parallel {
                stage('macOS x86_64') {
                    when {
                        expression { params.BUILD_MAC_X86 }
                    }
                    agent { label 'mac_virt' }
                    environment {
                        QT5_DIR = '/Users/cxsw_imac2/Qt/5.15.2/clang_64/'
                        MAC_X86_KEYCHAIN_PATH = '/Users/creality/Library/Keychains/login.keychain-db'
                        MAC_X86_PYTHON_ACTIVATE = '/Users/creality/python3/bin/activate'
                        MAC_X86_DEPS_ENV_DIR = '/Users/creality/Orca_work/dep_x86_64'
                        MAC_X86_NODE_PATH = '/Users/creality/.nvm/versions/node/v22.16.0/bin'
                        MAC_X86_CMAKE_BIN = '/Applications/CMake.app/Contents/bin'
                    }
                    stages {
                        stage('Checkout') {
                            steps {
                                script {
                                    if (params.CLEAN_WORKSPACE) {
                                        deleteDir()
                                    }
                                }
                                checkout([
                                    $class: 'GitSCM',
                                    branches: [[name: "${params.BRANCH}"]],
                                    doGenerateSubmoduleConfigurations: false,
                                    extensions: [],
                                    userRemoteConfigs: [[credentialsId: 'jenkins', url: "${env.CORE_REPO}"]]
                                ])
                                checkout([
                                    $class: 'GitSCM',
                                    branches: [[name: "${params.WEB_BRANCH}"]],
                                    doGenerateSubmoduleConfigurations: false,
                                    extensions: [[$class: 'RelativeTargetDirectory', relativeTargetDir: 'Community']],
                                    userRemoteConfigs: [[credentialsId: 'jenkins', url: "${env.COMMUNITY_REPO}"]]
                                ])
                            }
                        }

                        stage('Build') {
                            steps {
                                script {
                                    def macScript = '''
echo ${TAG_NAME}
security unlock-keychain "-p" "${MAC_KEYCHAIN_PASSWORD}" "${MAC_KEYCHAIN_PATH}"
CMMID=`git show-ref ${TAG_NAME} | awk -F ' ' '{print $1}'`
MAXCMMID=`git rev-list HEAD | wc -l`
TAGCMMID=`git rev-list ${CMMID} | wc -l`
TAGNUMB=$((MAXCMMID-TAGCMMID))
export PATH="${NODE_PATH}:${CMAKE_BIN}:/usr/local/bin/:$PATH"
source "${PYTHON_ACTIVATE}"
cd "$WORKSPACE"
defult_para_type="Beta"
if [ "$RTYPE" = "Alpha" ]; then
    defult_para_type="Alpha"
fi
chmod +x "$(pwd)/resources"
rm -rf "$(pwd)/resources"
git reset --hard
web_file="./scripts/sync_web.sh"
if [ -f "$web_file" ] && [ "$WEB_SYNC" = "true" ]; then
    chmod +x "$web_file"
    "$web_file" "$WEB_BRANCH" || exit -2
fi
if [ "$SYNCPRESET" = "1" ]; then
    python3 ./scripts/generate_creality_presets.py -b "$defult_para_type" -n "3.0.0" || exit -2
fi
export SLICER_BUILD_TARGET=all
export SLICER_CMAKE_GENERATOR=Ninja
export ARCH="x86_64"
export DEPS_ENV_DIR="${MAC_DEPS_ENV_DIR}"
rm -f build_x86_64/*.tar.gz
rm -f build_x86_64/*.dmg
./scripts/build_package_macos.sh ${TAG_NAME}.${TAGNUMB} ${APP_NAME} ${RTYPE} ${SLICER_HEADER} || exit -2
echo SIGN_PACKAGE_PATH=$JOB_NAME > var.prop
echo X86_SIGN_PACKAGE_NAME=${APP_NAME}-${TAG_NAME}.${TAGNUMB}-macx-x86_64-${RTYPE}.dmg >> var.prop
echo SIGN_PACKAGE_SYM_NAME=${APP_NAME}-${TAG_NAME}.${TAGNUMB}-macx-x86_64-${RTYPE}.sym.tar.gz >> var.prop
echo SIGN_PACKAGE_DSYM_NAME=${APP_NAME}-${TAG_NAME}.${TAGNUMB}-macx-x86_64-${RTYPE}.dSYM.tar.gz >> var.prop
echo PACKAGE_NUMB=${TAGNUMB} >> var.prop
'''

                                    withCredentials([string(credentialsId: 'mac-x86-keychain-password', variable: 'MAC_KEYCHAIN_PASSWORD')]) {
                                        withEnv([
                                            "MAC_KEYCHAIN_PATH=${env.MAC_X86_KEYCHAIN_PATH}",
                                            "PYTHON_ACTIVATE=${env.MAC_X86_PYTHON_ACTIVATE}",
                                            "MAC_DEPS_ENV_DIR=${env.MAC_X86_DEPS_ENV_DIR}",
                                            "NODE_PATH=${env.MAC_X86_NODE_PATH}",
                                            "CMAKE_BIN=${env.MAC_X86_CMAKE_BIN}"
                                        ]) {
                                            sh macScript
                                        }
                                    }

                                    if (fileExists('var.prop')) {
                                        readFile('var.prop').split(/\r?\n/).findAll { it?.trim() }.each { line ->
                                            def idx = line.indexOf('=')
                                            if (idx > 0) {
                                                env."${line.substring(0, idx)}" = line.substring(idx + 1)
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        stage('Upload And Archive') {
                            steps {
                                sh '''
cd "$WORKSPACE"
scp -P ${BUILD_SHARE_PORT} ./build_x86_64/$X86_SIGN_PACKAGE_NAME cxsw@${BUILD_SHARE_HOST}:${BUILD_SHARE_ROOT}/$SIGN_PACKAGE_PATH/$X86_SIGN_PACKAGE_NAME
scp -P ${BUILD_SHARE_PORT} ./build_x86_64/$SIGN_PACKAGE_SYM_NAME cxsw@${BUILD_SHARE_HOST}:${BUILD_SHARE_ROOT}/$SIGN_PACKAGE_PATH/$SIGN_PACKAGE_SYM_NAME
scp -P ${BUILD_SHARE_PORT} ./build_x86_64/$SIGN_PACKAGE_DSYM_NAME cxsw@${BUILD_SHARE_HOST}:${BUILD_SHARE_ROOT}/$SIGN_PACKAGE_PATH/$SIGN_PACKAGE_DSYM_NAME
git log --oneline --since="yesterday" > changes.txt
'''
                                archiveArtifacts artifacts: 'build_x86_64/*.dmg,build_x86_64/*.tar.gz', onlyIfSuccessful: true
                            }
                        }
                    }
                    post {
                        success {
                            script {
                                writeFile file: 'version.txt', text: "${params.TAG_NAME}.${env.PACKAGE_NUMB}-${params.RTYPE}\n"
                                if (params.NOTIFY == '1') {
                                    try {
                                        withCredentials([string(credentialsId: env.FEISHU_WEBHOOK_CRED, variable: 'FEISHU_WEBHOOK')]) {
                                            def packageUrl = "http://${env.BUILD_SHARE_HOST}/shared/build/${env.JOB_NAME}/${env.X86_SIGN_PACKAGE_NAME}"
                                            def changesUrl = "${env.BUILD_URL}changes"
                                            def cardPayload = [
                                                msg_type: 'interactive',
                                                card    : [
                                                    config : [wide_screen_mode: true],
                                                    header : [
                                                        template: 'green',
                                                        title   : [tag: 'plain_text', content: "macOS x86_64 package succeeded: ${params.APP_NAME} ${params.TAG_NAME}.${env.PACKAGE_NUMB}-${params.RTYPE}"]
                                                    ],
                                                    elements: [
                                                        [tag: 'div', text: [tag: 'lark_md', content: """**Job**: ${env.JOB_NAME}
**Build**: #${env.BUILD_NUMBER}
**Branch**: ${params.BRANCH}
**Version**: ${params.TAG_NAME}.${env.PACKAGE_NUMB}-${params.RTYPE}
**Platform**: macOS x86_64
**Package**: ${env.X86_SIGN_PACKAGE_NAME}"""]],
                                                        [tag: 'action', actions: [
                                                            [tag: 'button', text: [tag: 'plain_text', content: 'Build'], type: 'primary', url: env.BUILD_URL],
                                                            [tag: 'button', text: [tag: 'plain_text', content: 'Changes'], type: 'default', url: changesUrl],
                                                            [tag: 'button', text: [tag: 'plain_text', content: 'Package'], type: 'default', url: packageUrl]
                                                        ]]
                                                    ]
                                                ]
                                            ]
                                            httpRequest(
                                                httpMode: 'POST',
                                                contentType: 'APPLICATION_JSON',
                                                ignoreSslErrors: true,
                                                quiet: false,
                                                requestBody: groovy.json.JsonOutput.toJson(cardPayload),
                                                url: "${FEISHU_WEBHOOK}",
                                                validResponseCodes: '100:399'
                                            )
                                        }
                                    } catch (err) {
                                        echo "Skip Feishu notification: ${err.message}"
                                    }
                                }
                            }
                        }
                    }
                }

                stage('macOS arm64') {
                    when {
                        expression { params.BUILD_MAC_ARM }
                    }
                    agent { label 'mac_m2' }
                    environment {
                        MAC_ARM_KEYCHAIN_PATH = '/Users/qprj/Library/Keychains/login.keychain-db'
                        MAC_ARM_DEPS_ENV_DIR = '/Users/qprj/work/DEPS_LIB_DIR'
                        MAC_ARM_EXTRA_PATH = '/opt/homebrew/bin:/opt/homebrew/opt/node@20/bin:/Users/qprj/breakpad/breakpad/src/tools/mac/dump_syms/build/Release/dump_syms:/Users/qprj/breakpad/breakpad/src/processor/minidump_stackwalk'
                    }
                    stages {
                        stage('Checkout') {
                            steps {
                                script {
                                    if (params.CLEAN_WORKSPACE) {
                                        deleteDir()
                                    }
                                }
                                checkout([
                                    $class: 'GitSCM',
                                    branches: [[name: "${params.BRANCH}"]],
                                    doGenerateSubmoduleConfigurations: false,
                                    extensions: [],
                                    userRemoteConfigs: [[credentialsId: 'jenkins', url: "${env.CORE_REPO}"]]
                                ])
                                checkout([
                                    $class: 'GitSCM',
                                    branches: [[name: "${params.WEB_BRANCH}"]],
                                    doGenerateSubmoduleConfigurations: false,
                                    extensions: [[$class: 'RelativeTargetDirectory', relativeTargetDir: 'Community']],
                                    userRemoteConfigs: [[credentialsId: 'jenkins', url: "${env.COMMUNITY_REPO}"]]
                                ])
                            }
                        }

                        stage('Build') {
                            steps {
                                script {
                                    def macArmScript = '''
echo ${TAG_NAME}
security unlock-keychain "-p" "${MAC_KEYCHAIN_PASSWORD}" "${MAC_KEYCHAIN_PATH}"
CMMID=`git show-ref ${TAG_NAME} | awk -F ' ' '{print $1}'`
MAXCMMID=`git rev-list HEAD | wc -l`
TAGCMMID=`git rev-list ${CMMID} | wc -l`
TAGNUMB=$((MAXCMMID-TAGCMMID))
export PATH="${MAC_EXTRA_PATH}:$PATH"
export DEPS_ENV_DIR="${MAC_DEPS_ENV_DIR}"
cd "$WORKSPACE"
defult_para_type="Beta"
if [ "$RTYPE" = "Alpha" ]; then
    defult_para_type="Alpha"
fi
chmod +x "$(pwd)/resources"
rm -rf "$(pwd)/resources"
git reset --hard
web_file="./scripts/sync_web.sh"
if [ -f "$web_file" ] && [ "$WEB_SYNC" = "true" ]; then
    /bin/sh "$web_file" "$WEB_BRANCH" || exit -2
fi
if [ "$SYNCPRESET" = "1" ]; then
    /usr/bin/python3 ./scripts/generate_creality_presets.py -b "$defult_para_type" -n "3.0.0" || exit -2
fi
rm -f build_arm64/*.tar.gz
rm -f build_arm64/*.dmg
/bin/bash ./scripts/build_package_macos.sh ${TAG_NAME}.${TAGNUMB} ${APP_NAME} ${RTYPE} ${SLICER_HEADER} || exit -2
echo SIGN_PACKAGE_PATH=$JOB_NAME > var.prop
echo SIGN_PACKAGE_NAME=${APP_NAME}-${TAG_NAME}.${TAGNUMB}-macx-arm64-${RTYPE}.dmg >> var.prop
echo SIGN_PACKAGE_SYM_NAME=${APP_NAME}-${TAG_NAME}.${TAGNUMB}-macx-arm64-${RTYPE}.sym.tar.gz >> var.prop
echo SIGN_PACKAGE_DSYM_NAME=${APP_NAME}-${TAG_NAME}.${TAGNUMB}-macx-arm64-${RTYPE}.dSYM.tar.gz >> var.prop
echo PACKAGE_NUMB=${TAGNUMB} >> var.prop
'''

                                    withCredentials([string(credentialsId: 'mac-arm-keychain-password', variable: 'MAC_KEYCHAIN_PASSWORD')]) {
                                        withEnv([
                                            "MAC_KEYCHAIN_PATH=${env.MAC_ARM_KEYCHAIN_PATH}",
                                            "MAC_DEPS_ENV_DIR=${env.MAC_ARM_DEPS_ENV_DIR}",
                                            "MAC_EXTRA_PATH=${env.MAC_ARM_EXTRA_PATH}"
                                        ]) {
                                            sh macArmScript
                                        }
                                    }

                                    if (fileExists('var.prop')) {
                                        readFile('var.prop').split(/\r?\n/).findAll { it?.trim() }.each { line ->
                                            def idx = line.indexOf('=')
                                            if (idx > 0) {
                                                env."${line.substring(0, idx)}" = line.substring(idx + 1)
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        stage('Upload And Archive') {
                            steps {
                                sh '''
cd "$WORKSPACE"
scp -P ${BUILD_SHARE_PORT} ./build_arm64/$SIGN_PACKAGE_NAME cxsw@${BUILD_SHARE_HOST}:${BUILD_SHARE_ROOT}/$SIGN_PACKAGE_PATH/$SIGN_PACKAGE_NAME
scp -P ${BUILD_SHARE_PORT} ./build_arm64/$SIGN_PACKAGE_SYM_NAME cxsw@${BUILD_SHARE_HOST}:${BUILD_SHARE_ROOT}/$SIGN_PACKAGE_PATH/$SIGN_PACKAGE_SYM_NAME
scp -P ${BUILD_SHARE_PORT} ./build_arm64/$SIGN_PACKAGE_DSYM_NAME cxsw@${BUILD_SHARE_HOST}:${BUILD_SHARE_ROOT}/$SIGN_PACKAGE_PATH/$SIGN_PACKAGE_DSYM_NAME
git log --oneline --since="yesterday" > changes.txt
'''
                                archiveArtifacts artifacts: 'build_arm64/*.dmg,build_arm64/*.tar.gz', onlyIfSuccessful: true
                            }
                        }
                    }
                    post {
                        success {
                            script {
                                writeFile file: 'version.txt', text: "${params.TAG_NAME}.${env.PACKAGE_NUMB}-${params.RTYPE}\n"
                                if (params.NOTIFY == '1') {
                                    try {
                                        withCredentials([string(credentialsId: env.FEISHU_WEBHOOK_CRED, variable: 'FEISHU_WEBHOOK')]) {
                                            def packageUrl = "http://${env.BUILD_SHARE_HOST}/shared/build/${env.JOB_NAME}/${env.SIGN_PACKAGE_NAME}"
                                            def changesUrl = "${env.BUILD_URL}changes"
                                            def cardPayload = [
                                                msg_type: 'interactive',
                                                card    : [
                                                    config : [wide_screen_mode: true],
                                                    header : [
                                                        template: 'green',
                                                        title   : [tag: 'plain_text', content: "macOS arm64 package succeeded: ${params.APP_NAME} ${params.TAG_NAME}.${env.PACKAGE_NUMB}-${params.RTYPE}"]
                                                    ],
                                                    elements: [
                                                        [tag: 'div', text: [tag: 'lark_md', content: """**Job**: ${env.JOB_NAME}
**Build**: #${env.BUILD_NUMBER}
**Branch**: ${params.BRANCH}
**Version**: ${params.TAG_NAME}.${env.PACKAGE_NUMB}-${params.RTYPE}
**Platform**: macOS arm64
**Package**: ${env.SIGN_PACKAGE_NAME}"""]],
                                                        [tag: 'action', actions: [
                                                            [tag: 'button', text: [tag: 'plain_text', content: 'Build'], type: 'primary', url: env.BUILD_URL],
                                                            [tag: 'button', text: [tag: 'plain_text', content: 'Changes'], type: 'default', url: changesUrl],
                                                            [tag: 'button', text: [tag: 'plain_text', content: 'Package'], type: 'default', url: packageUrl]
                                                        ]]
                                                    ]
                                                ]
                                            ]
                                            httpRequest(
                                                httpMode: 'POST',
                                                contentType: 'APPLICATION_JSON',
                                                ignoreSslErrors: true,
                                                quiet: false,
                                                requestBody: groovy.json.JsonOutput.toJson(cardPayload),
                                                url: "${FEISHU_WEBHOOK}",
                                                validResponseCodes: '100:399'
                                            )
                                        }
                                    } catch (err) {
                                        echo "Skip Feishu notification: ${err.message}"
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    post {
        always {
            script {
                def enabledPlatforms = []
                if (params.BUILD_WIN) {
                    enabledPlatforms << 'Windows'
                }
                if (params.BUILD_MAC_X86) {
                    enabledPlatforms << 'macOS x86_64'
                }
                if (params.BUILD_MAC_ARM) {
                    enabledPlatforms << 'macOS arm64'
                }
                def platformSummary = enabledPlatforms ? enabledPlatforms.join(', ') : 'none'
                def finalStatus = currentBuild.currentResult ?: 'UNKNOWN'

                if (params.NOTIFY == '1') {
                    try {
                        withCredentials([string(credentialsId: env.FEISHU_WEBHOOK_CRED, variable: 'FEISHU_WEBHOOK')]) {
                            def headerTemplate = finalStatus == 'SUCCESS' ? 'green' : (finalStatus == 'FAILURE' ? 'red' : 'orange')
                            def cardPayload = [
                                msg_type: 'interactive',
                                card    : [
                                    config : [wide_screen_mode: true],
                                    header : [
                                        template: headerTemplate,
                                        title   : [tag: 'plain_text', content: "Jenkins pipeline finished: ${env.JOB_NAME}"]
                                    ],
                                    elements: [
                                        [tag: 'div', text: [tag: 'lark_md', content: """**Status**: ${finalStatus}
**Build**: #${env.BUILD_NUMBER}
**Platforms**: ${platformSummary}
**Branch**: ${params.BRANCH}
**Trigger**: ${currentBuild.getBuildCauses().collect { it.shortDescription }.join('; ')}"""]],
                                        [tag: 'action', actions: [
                                            [tag: 'button', text: [tag: 'plain_text', content: 'Build'], type: 'primary', url: env.BUILD_URL],
                                            [tag: 'button', text: [tag: 'plain_text', content: 'Changes'], type: 'default', url: "${env.BUILD_URL}changes"]
                                        ]]
                                    ]
                                ]
                            ]
                            httpRequest(
                                httpMode: 'POST',
                                contentType: 'APPLICATION_JSON',
                                ignoreSslErrors: true,
                                quiet: false,
                                requestBody: groovy.json.JsonOutput.toJson(cardPayload),
                                url: "${FEISHU_WEBHOOK}",
                                validResponseCodes: '100:399'
                            )
                        }
                    } catch (err) {
                        echo "Skip final Feishu notification: ${err.message}"
                    }
                }

                try {
                    withCredentials([string(credentialsId: env.OPENCLAW_WEBHOOK_CRED, variable: 'OPENCLAW_WEBHOOK')]) {
                        httpRequest(
                            httpMode: 'POST',
                            contentType: 'APPLICATION_JSON',
                            ignoreSslErrors: true,
                            quiet: false,
                            requestBody: """{
"job": {
"name": "${env.JOB_NAME}",
"url": "${env.BUILD_URL}"
},
"build": {
"number": ${env.BUILD_NUMBER},
"status": "${finalStatus}",
"url": "${env.BUILD_URL}"
}
}""",
                            url: "${OPENCLAW_WEBHOOK}",
                            validResponseCodes: '100:399'
                        )
                    }
                } catch (err) {
                    echo "Skip final openclaw notification: ${err.message}"
                }
            }
        }
    }
}
