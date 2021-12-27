#metasploit table HTTP grabbing practice

#import requests

#url="http://192.168.214.132"
#response=requests.get(url)

#print("상태코드: ", response.status_code)
#print("결과 페이지: ", response.text)


import urllib.request

url=input("분석할 URL 입력>> ")
http_req=urllib.request.urlopen(url)

if http_req.code==200:
    print(http_req.headers)

