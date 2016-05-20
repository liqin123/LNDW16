#!/usr/bin/env python

"""
Pass a value to the makefile
CPPFLAGS=-DMYFLAG=
set environment variable correctly
PATH=$PATH:/media/intern/Coding/..


import subprocess, os
my_env = os.environ.copy()
my_env["PATH"] = "/usr/sbin:/sbin:" + my_env["PATH"]
subprocess.Popen(my_command, env=my_env)


create a new directory for each firmware (0x0000, 0x4000)
copy the firmware
"""

CPPFLAGS = "CPPFLAGS=-DMYFLAG="
FW = "fw"

import subprocess
import os
import shutil

MAKE_CLEAN = ["make", "clean"]
MAKE = ["make"]

NUM_OF_ESPS = 50
FW_DUMP_DIR = "batch_firmwares/"

def main():
    my_env = os.environ.copy()
    my_env["PATH"] = "/media/intern/Coding/SHK/esp-open-sdk/xtensa-lx106-elf/bin:" + my_env["PATH"]
    for last_mac_byte in range(NUM_OF_ESPS):
        # hack: run 'make clean' each time to ensure that the firmware is actually rebuild. Use touch instead?
        subprocess.Popen(MAKE_CLEAN).wait()
        # firmwares will be stored under "FW_DUMP_DIR/0xYY"
        if (not os.path.exists(FW_DUMP_DIR)):
            os.mkdir(FW_DUMP_DIR)
        cpp_flag = "CPPFLAGS=-DMYFLAG="+hex(last_mac_byte)
        subprocess.Popen(MAKE + [cpp_flag], env=my_env).wait()
        # create the subdir and copy the files
        sub_dump_dir = FW_DUMP_DIR+hex(last_mac_byte)
        os.mkdir(sub_dump_dir)
        shutil.copy("firmware/0x00000.bin", sub_dump_dir)
        shutil.copy("firmware/0x40000.bin", sub_dump_dir)
main()

