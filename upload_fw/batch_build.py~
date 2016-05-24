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

MAKE_CLEAN = ["make", "clean"]
MAKE = ["make"]

def main():
    subprocess.Popen(MAKE_CLEAN)
    subprocess.Popen(MAKE)


main()

