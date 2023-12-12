import requests
import json
import os
import sys
import random
from requests.auth import HTTPBasicAuth
from tqdm import tqdm

def pretty_print(data):
    out = json.dumps(data, sort_keys=False, indent=4, separators=(',', ':'))
    print(out)

class HTTPHost:
    def __init__(self, url, name, password):
        self.url = url
        self.name = name
        self.password = password

        print('url {}'.format(self.url))
        print('name {}'.format(self.name))
        print('password {}'.format(self.password))
        
    def request_files(self, local_dir):
        try:
            resp = requests.get(self.url, headers=self._headers(),auth=HTTPBasicAuth(self.name, self.password))
            if resp.content:
                self._parse_content(resp.content, local_dir)
                
        except Exception as ex:
            print('exception {}'.format(ex))
            
    def _headers(self):
        return {"Content-Type": "application/json;charset=UTF-8"}
        
    def _parse_content(self, content, local_dir):
        from bs4 import BeautifulSoup
        soup = BeautifulSoup(content,'html.parser')
        links =[]
        for link in soup.find_all('a'):
            href = link.get('href')
            if not href.endswith('/'):
                links.append(href)
        for link in links:
            fileurl = '{0}/{1}'.format(self.url, link)
            filename = '{0}/{1}'.format(local_dir, link)
            #self._export_file(fileurl, filename)
            self._download_by_stream(256 * 1024, fileurl, filename)
            
    def _export_file(self, fileurl, filename):
        response = requests.get(fileurl, auth=HTTPBasicAuth(self.name, self.password))
        with open(filename,'wb') as f:
            f.write(response.content)
            
    def _auth(self):
        return HTTPBasicAuth(self.name, self.password)
        
    def _download_by_stream(self, chunk_size, url, save_path):
        # 如果文件存在, 删除重新下
        if os.path.exists(save_path):
            os.remove(save_path)
        # tqdm可选total参数,不传递这个参数则不显示文件总大小
        desc = '下载 {}'.format(os.path.basename(save_path))
        progress_bar = tqdm(initial=0, unit='B', unit_divisor=1024, unit_scale=True, desc=desc)
    
        # 设置stream=True参数读取大文件
        response = requests.get(url, stream=True, auth=self._auth())
        with open(save_path, 'ab') as fp:
            # 每次最多读取chunk_size个字节
            for chunk in response.iter_content(chunk_size=chunk_size):
                if chunk:
                    fp.write(chunk)
                    progress_bar.update(len(chunk))
        progress_bar.close()
        
def check_local_dir(local_dir_name):
    """
    本地文件夹是否存在，不存在则创建
    :local_dir_name: 本地文件存放路径
    """
    if not os.path.exists(local_dir_name):
        os.makedirs(local_dir_name)
        return False
    else:
        return True
 
def check_local_file(local_dir_name):
    """
    本地文件是否存在，不存在则下载
    :local_dir_name: 本地文件存放路径
    """
    if not os.path.exists(local_dir_name):
        return False
    else:
        return True
 
if __name__ == "__main__":
    local_dir = 'C:/Users/anoob/code/data/load/'
    httpHost = HTTPHost(sys.argv[1], sys.argv[2], sys.argv[3])
    # 远程文件开始下载
    httpHost.request_files(local_dir)