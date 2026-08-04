from requests.models import Response
import os, sys
import json
import hashlib
import gzip
import uuid
import requests
import importlib
import time
import platform
import tempfile
import zipfile
import shutil
import appdirs
from typing import Dict
from urllib3 import HTTPConnectionPool, HTTPSConnectionPool
https_adapter = requests.adapters.HTTPAdapter(pool_connections=2, pool_maxsize=5)
USER_AGENT_SUFFIX = "(dark) Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/107.0.0.0 Safari/537.36 Edg/107.0.1418.52"

def _generateDUID() -> str:
    return str(uuid.uuid1())[-12:]

def getUserAgent(app_version: str) -> str:
    version = app_version.strip()
    if not version:
        raise ValueError("app_version is required")
    normalized_version = version if version.startswith("v") else f"v{version}"
    return f"Creality-Slicer/{normalized_version} {USER_AGENT_SUFFIX}"

def getCommonHeaders(app_version: str) -> Dict[str, str]:
    headers = {
        "User-Agent": getUserAgent(app_version),
        "Content-Type": "application/json; charset=UTF-8",
        "__CXY_APP_ID_": "creality_model",
        "__CXY_OS_LANG_": "0",
        "__CXY_DUID_": _generateDUID(),
        "__CXY_OS_VER_": platform.system(),
        "__CXY_PLATFORM_": "11",
        "__CXY_REQUESTID_": str(uuid.uuid1()),
        "__CXY_APP_VER_": "4.4.0",
        "__CXY_APP_CH_": "Android_Beta",
        "__CXY_BRAND_": "creality",
        "__CXY_TIMEZONE_": str(time.time()),
        "__cxy_token_":os.getenv('cxy_token'),
        "__cxy_uid_":"5996392753"
    }
    return headers
def processLocalParamPack(working_path, build_type, engine_type, engine_version) -> None:
    server_path_prefixes = ["server_0", "server_1"]
    for server_path_prefix in server_path_prefixes:
        if sys.platform.startswith('win'):
            default_path = os.path.join(working_path, "build","resources", "sliceconfig",server_path_prefix)
        if sys.platform.startswith('linux'):
            default_path = os.path.join(working_path, "linux-build", "build","resources", "sliceconfig")
        if sys.platform.startswith('darwin'):
            default_path = os.path.join(working_path, "mac-build", "build","resources", "sliceconfig")  
        if os.path.exists(default_path):
            shutil.rmtree(default_path)
        shutil.copytree(os.path.join(working_path, "resources", "sliceconfig", server_path_prefix), default_path)    
        print("use local parampack:"+os.path.join(working_path, "resources", "sliceconfig", server_path_prefix))     
def downloadParamPack(working_path, build_type, engine_type, engine_version, app_version) -> None:
    server_path_prefixes = ["server_0"]
    base_urls = ['https://api.crealitycloud.cn/', 'https://api.crealitycloud.com/']
    base_alpha_urls = ['https://admin-pre.crealitycloud.cn/', 'https://admin-pre.crealitycloud.cn/']
    idx = 0
    for server_path_prefix in server_path_prefixes:
        if build_type == 'Release' or build_type == 'Beta':
            base_url = base_urls[idx]
        else:
            base_url = base_alpha_urls[idx]     
        print("Base url:"+base_url)
        idx+=1        
        if sys.platform.startswith('win'):
            default_path = os.path.join(working_path, "build","resources", "sliceconfig", server_path_prefix, engine_type, "default")
        if sys.platform.startswith('linux'):
            default_path = os.path.join(working_path, "linux-build", "build","resources", "sliceconfig", server_path_prefix, engine_type, "default")
        if sys.platform.startswith('darwin'):
            default_path = os.path.join(working_path, "mac-build", "build","resources", "sliceconfig", server_path_prefix, engine_type, "default")
        default_path = os.path.join(working_path, server_path_prefix, engine_type, "default")    
        try:
            
            response = requests.post(
                base_url + "api/cxy/v2/slice/profile/official/printerList", data=json.dumps({"engineVersion": engine_version}), 
                headers=getCommonHeaders(app_version)).text
            response = json.loads(response)
            print(str(response))
            if (response["code"] == 0):
                file_path = os.path.join(default_path, "machineList.json")
                print(file_path)
                if os.path.exists(default_path):
                    shutil.rmtree(default_path)
                os.makedirs(default_path)
                with open(file_path, 'w+', encoding='utf8') as json_file:
                    json.dump(response["result"], json_file, ensure_ascii=False)
                printer_list = response["result"]["printerList"]
                session = requests.Session()
                session.mount('https://', https_adapter)
                session.headers.update({"User-Agent": getUserAgent(app_version)})
                #session2 = requests.Session()
                for printer in printer_list:
                    zip_url = printer['zipUrl']
                    if zip_url == "":
                        continue
                    unique_printer_name = printer['printerIntName']
                    for nozzleDiameter in printer['nozzleDiameter']:
                        unique_printer_name = unique_printer_name + "-" + nozzleDiameter
                    print(unique_printer_name)
                    unique_file_name = zip_url.split('/')[-2]
                    cache_dir = appdirs.user_cache_dir("CrealityPrint")
                    tmpdirname = os.path.join(cache_dir, "temp")
                    if not os.path.exists(tmpdirname):
                        os.makedirs(tmpdirname)
                    tmpdirname = os.path.join(tmpdirname, unique_file_name + '.zip')
                    print(tmpdirname)
                    if not os.path.exists(tmpdirname):
                        r = session.get(zip_url, stream=True) 
                        open(tmpdirname, 'wb+').write(r.content)
                    with zipfile.ZipFile(tmpdirname, 'r') as zObject: 
                        zObject.extractall(path=os.path.join(default_path, "parampack", unique_printer_name))
                    #download thumb
                    thumb_url = printer['thumbnail']
                    if thumb_url == "":
                        continue
                    unique_file_name = thumb_url.split('/')[-1]
                    tmpdirname = os.path.join(cache_dir,"temp", unique_file_name + '.png')
                    if not os.path.exists(tmpdirname):
                        r = session.get(thumb_url, stream=True)
                        open(tmpdirname, 'wb+').write(r.content)
                    imagedir = os.path.join(default_path, "machineImages")
                    if not os.path.exists(imagedir):
                        os.makedirs(imagedir)
                    imagedirname = os.path.join(imagedir, printer['printerIntName'] + '.png')                    
                    shutil.copyfile(tmpdirname, imagedirname)
                    print(tmpdirname)

            else:
                print("get parampack cloud error")
        except Exception as e:
            print("get parampack exception" + str(e))            
        try:
            response = requests.post(
                base_url + "api/cxy/v2/slice/profile/official/materialList", data=json.dumps({"engineVersion": engine_version, "pageSize": 1000}), 
                headers=getCommonHeaders(app_version)).text
            response = json.loads(response)
            if (response["code"] == 0):
                file_path = os.path.join(default_path, "materialList.json")
                material_list = response["result"]["list"]
                material_array = []
                for material in material_list:
                    material_obj = {
                        'id': material['id'],
                        'name': material['name'],
                        'type': material['meterialType'],
                        'brand': material['brand'],
                        'supportDiameters': material['diameter'],
                        'rank': material['rank']
                    }
                    material_array.append(material_obj)
                material_json = {
                    'materials': material_array
                }
                
                with open(file_path, 'w+', encoding='utf8') as json_file:
                    json.dump(material_json, json_file, ensure_ascii=False)
        
            else:
                print("get parampack cloud error")
        except Exception as e:
            print("get parampack exception" + str(e))


# if __name__ == "__main__":
#     downloadParamPack("D:\\work\\c3d_5.0.2\\c3d", "Release", "orca", "1.6.0", "7.2.0.5226")
