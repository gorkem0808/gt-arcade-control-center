@echo off
cd /d "%~dp0"
py -m pip install -r requirements.txt
py gt_arcade_control_center.py
pause
