import os

menuconfig_cmd = 'menuconfig'
auto_header_cmd = 'python .\kconfig.py Kconfig .config autoconfig.h log.tex .config'

os.system(menuconfig_cmd)
os.system(auto_header_cmd)

