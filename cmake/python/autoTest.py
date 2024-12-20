import sys
import os
import osSystem
import json
import requests
import platform
import subprocess
import csv
import FeiShu
from pathlib import Path

class AutoTestBench():
    def __init__(self):
        self.origin_path = Path(sys.path[0] + '/../../')
        self.bin_path = self.origin_path.joinpath('linux-build/bin/Release/')
        self.system = platform.system()
        self.JECKINS_INFO = eval(sys.argv[1])
        
        self.webhook = self.JECKINS_INFO['WEBHOOK']
        self.commit_id = self.JECKINS_INFO['COMMIT_ID']
        self.branch_name = self.JECKINS_INFO['BRANCH_NAME']
        self.job_name = self.JECKINS_INFO['JOB_NAME']
        self.user = self.JECKINS_INFO['USER']
        self.url = self.JECKINS_INFO['URL']
        self.download_url = self.JECKINS_INFO['DOWNLOAD_URL']
        self.csv_name = '{}-{}.csv'.format(self.branch_name.replace("/", "-"), self.commit_id)
        self.download_url = '{}/{}/{}'.format(self.download_url, self.job_name, self.csv_name)
        self.scp_url = '{}:{}/{}/'.format(self.user, self.url, self.job_name)
        
        if self.system == 'Windows':
            self.bin_path = self.origin_path.joinpath('win32-build/bin/Release/')
            
        self.print_info()
        self.reset_working_directory()

    def print_info(self):
        print("AutoTest jeckins info " + str(self.JECKINS_INFO))
        print("AutoTest origin_path: " + str(self.origin_path))
        print("AutoTest bin_path: " + str(self.bin_path))
        
    def reset_working_directory(self):
        osSystem.cd(str(self.bin_path))
        
    def ListFilesToList(self, dir,file,wildcard,recursion):
        exts = wildcard.split(" ")
        for root, subdirs, files in os.walk(dir):
            for name in files:
                for ext in exts:
                    if(name.endswith(ext)):
                        file.append(name)
                        break
            if(not recursion):
                break
         
    def execute_directory(self, name, directory):
        files = os.listdir(directory)
        
        datas = []
        for file in files:
            exe_test = '{}/{} "./{}/{}"'.format(str(self.bin_path), name, directory, file) 
            ret, value = subprocess.getstatusoutput(exe_test)
            
            data = {}
            data['input'] = file
            data['value'] = value
            if ret == 0:
                print(exe_test + " success!")
            else:
                data['value'] = "subprocess error! " + data['value']
                print("subprocess.getstatusoutput error.")        
            
            datas.append(data)
            
        return datas
        
    def execute(self, name, tests):
        datas = []
        for test in tests:
            exe_test = '{}/{} "./data/{}"'.format(str(self.bin_path), name, test) 
            ret, value = subprocess.getstatusoutput(exe_test)
            
            data = {}
            data['input'] = test
            data['value'] = value
            if ret == 0:
                print(exe_test + " success!")
            else:
                data['value'] = "subprocess error! " + data['value']
                print("subprocess.getstatusoutput error.")        
            
            datas.append(data)
            
        return datas
        
    def clone_source_to(self, url, name):
        path = Path(name)
        if path.exists():
            osSystem.cd(str(path))
            osSystem.system("git pull")
        else:
            osSystem.system("git clone {} {}".format(str(url), name))
            
        self.reset_working_directory()
        
    def clone_source(self, url):
        path = Path('data')
        if path.exists():
            osSystem.cd(str(path))
            osSystem.system("git pull")
        else:
            osSystem.system("git clone " + url + " data")
            
        self.reset_working_directory()
    
    def send_feishu_notice(self, notice):
        self.save_scp_csv(notice['datas'])
        notice['name'] = '{}-{}-{}'.format(notice['name'], self.branch_name, self.commit_id)
        notice['url'] = self.webhook
        notice['download_url'] = self.download_url
        
        FeiShu.send_auto_test_notice(notice)
        
    def save_scp_csv(self, datas):
        csv_name = '{}/temp.csv'.format(str(self.bin_path))
        with open(csv_name, 'w', encoding='utf-8-sig', newline="") as f:
            writer = csv.writer(f)
            names = ['输入', '结果', '细节']
            writer.writerow(names)
            csv_datas = []
            for data in datas:
                csv_data = [data['input'], 'PASS' if data['state'] == True else 'FAILED', data['value']]
                csv_datas.append(csv_data)
                
            writer.writerows(csv_datas)
            f.close()
        
        scp_cmd = 'scp {} {}/{}'.format(csv_name, self.scp_url, self.csv_name)            
        ret, value = subprocess.getstatusoutput(scp_cmd)   
        if ret == 0:
            print(scp_cmd + " success!")
        else:
            print(scp_cmd + " error.")         
        