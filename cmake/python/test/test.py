import sys;

if __name__ == '__main__':
    #localPath = 'E:/Data/test/sftp_test.txt'
    #remotePath = '/home/data/vagrantbox/sharedata/www/data/readme.txt'

    localPath = 'E:/Data/test/小熊1.stl'
    remotePath = '/home/data/vagrantbox/sharedata/www/data/model/小熊1.stl'

    directory = sys.path[0]
    sys.path.append(sys.path[0] + '/../')

    import SFTP
    sftp = SFTP.createCXSFTP()

    SFTP.getFile(sftp, remotePath, localPath)
    #SFTP.putFile(sftp, localPath, remotePath)