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
from typing import Dict, List, Optional
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

def getCommonHeaders(app_version: str, token: Optional[str] = None) -> Dict[str, str]:
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
        "__cxy_uid_":"5996392753"
    }
    if token:
        headers["__cxy_token_"] = token
    return headers

def _parse_unreleased_models(value: str) -> List[str]:
    models = []
    for item in value.split(';'):
        model = item.strip()
        if model and model not in models:
            models.append(model)
    if not models:
        raise ValueError("UNRELEASED_MODELS must contain at least one printerIntName")
    return models

def _request_printer_catalog(base_url: str, engine_version: str, app_version: str,
                             token: Optional[str] = None) -> Dict:
    response = requests.post(
        base_url + "api/cxy/v2/slice/profile/official/printerList",
        data=json.dumps({"engineVersion": engine_version}),
        headers=getCommonHeaders(app_version, token))
    response.raise_for_status()
    payload = response.json()
    if payload.get("code") != 0:
        raise RuntimeError(
            "get printer list failed: code={}, message={}".format(
                payload.get("code"), payload.get("msg", payload.get("message", ""))))
    result = payload.get("result")
    if not isinstance(result, dict) or not isinstance(result.get("printerList"), list):
        raise RuntimeError("get printer list failed: invalid response data")
    return result

def _merge_requested_unreleased_printers(published_result: Dict, all_result: Dict,
                                         requested_models: List[str]) -> Dict:
    published_printers = published_result.get("printerList", [])
    all_printers = all_result.get("printerList", [])
    published_names = {
        printer.get("printerIntName") for printer in published_printers
        if printer.get("printerIntName")
    }

    unreleased_by_name = {}
    for printer in all_printers:
        printer_name = printer.get("printerIntName")
        if printer_name and printer_name not in published_names:
            unreleased_by_name.setdefault(printer_name, []).append(printer)

    missing_models = [model for model in requested_models if model not in unreleased_by_name]
    if missing_models:
        raise ValueError(
            "The following printerIntName values are not in the unpublished model list: {}"
            .format(';'.join(missing_models)))

    selected_printers = []
    for model in requested_models:
        selected_printers.extend(unreleased_by_name[model])

    merged_result = dict(published_result)
    merged_result["printerList"] = published_printers + selected_printers

    published_series = list(published_result.get("series", []))
    existing_series_ids = {series.get("id") for series in published_series}
    selected_series_ids = {printer.get("seriesId") for printer in selected_printers}
    for series in all_result.get("series", []):
        if series.get("id") in selected_series_ids and series.get("id") not in existing_series_ids:
            published_series.append(series)
            existing_series_ids.add(series.get("id"))
    if published_series or "series" in published_result or "series" in all_result:
        merged_result["series"] = published_series

    return merged_result
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
        token = os.getenv('cxy_token', '').strip()
        unreleased_models_value = os.getenv('unreleased_models', '').strip()
        include_unreleased_models = (
            os.getenv('include_unreleased_models', 'false').strip().lower() == 'true')

        if include_unreleased_models:
            if not token:
                raise ValueError("cxy_token is required when unpublished models are enabled")
            if not unreleased_models_value:
                raise ValueError("UNRELEASED_MODELS is required when unpublished models are enabled")

            # The public request is the source of truth for released models.
            published_result = _request_printer_catalog(
                base_url, engine_version, app_version)
            requested_models = _parse_unreleased_models(unreleased_models_value)
            # The authenticated request returns released and unreleased models.
            all_result = _request_printer_catalog(
                base_url, engine_version, app_version, token)
            printer_result = _merge_requested_unreleased_printers(
                published_result, all_result, requested_models)
            print("Included unpublished printerIntName: " + ';'.join(requested_models))
        else:
            # Legacy behavior: make one request and package every model it returns.
            printer_result = _request_printer_catalog(
                base_url, engine_version, app_version, token or None)

        file_path = os.path.join(default_path, "machineList.json")
        print(file_path)
        if os.path.exists(default_path):
            shutil.rmtree(default_path)
        os.makedirs(default_path)
        with open(file_path, 'w+', encoding='utf8') as json_file:
            json.dump(printer_result, json_file, ensure_ascii=False)

        printer_list = printer_result["printerList"]
        session = requests.Session()
        session.mount('https://', https_adapter)
        session.headers.update({"User-Agent": getUserAgent(app_version)})
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
            # download thumb
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
        try:
            response = requests.post(
                base_url + "api/cxy/v2/slice/profile/official/materialList", data=json.dumps({"engineVersion": engine_version, "pageSize": 1000}), 
                headers=getCommonHeaders(app_version, token)).text
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
