import os
import sys
import time
import logging
import paramiko

from stat import S_ISDIR as isdir

class FTPHost:
    def __init__(self, ip, port, name, password):
        host = ip
        port = port
        username = name
        password = password

        print('host {}'.format(host))
        print('port {}'.format(port))
        print('name {}'.format(username))
        print('password {}'.format(password))
        # 连接远程服务器
        self.t = paramiko.Transport((host, port))
        self.t.connect(username=username, password=password)
        self.sftp = paramiko.SFTPClient.from_transport(self.t)
 
    def down_from_remote(self, remote_dir_name, local_dir_name):
        """
        远程下载文件及文件夹
        :remote_dir_name: 远程文件路径
        :local_dir_name: 本地文件存放路径
        """
        remote_file = self.sftp.stat(remote_dir_name)
        if isdir(remote_file.st_mode):
            # 文件夹，不能直接下载，需要继续循环
            if check_local_dir(local_dir_name)==True:
                print('本地文件夹已存在：' + local_dir_name)
            else:
                print('开始创建文件夹：' + remote_dir_name)
            for remote_file_name in self.sftp.listdir(remote_dir_name):
                sub_remote = os.path.join(remote_dir_name, remote_file_name)
                sub_remote = sub_remote.replace('\\', '/')
                sub_local = os.path.join(local_dir_name, remote_file_name)
                sub_local = sub_local.replace('\\', '/')
                self.down_from_remote(sub_remote, sub_local)
        else:
            # 文件，直接下载
            if check_local_file(local_dir_name)==True:
                print('本地文件已存在：' + local_dir_name)
            else:
                print('开始下载文件：' + remote_dir_name)
                try:
                    self.sftp.get(remote_dir_name, local_dir_name)
                except:
                    print('下载文件失败：' + remote_dir_name)
 
    def __del__(self):
        # 关闭连接
        self.t.close()
 
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
    remote_dir = '/scene/'
    local_dir = 'C:/Users/anoob/code/data/load/'
    h = FTPHost(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4])
    # 远程文件开始下载
    h.down_from_remote(remote_dir, local_dir)
    del h