import ftplib

def anonyLogin(host):
    try:
        ftp=ftplib.FTP(host)
        ftp.login('msfadmin','msfadmin') #('anonymous','**'), (msfadmin,msfadmin)
        print(host,'의 익명 연결이 성공.')
        ftp.retrlines('LIST')
        ftp.quit()
        return True
    except:
        print('익명 연결이 실패.')
        return False

host='192.168.214.132'
anonyLogin(host)
