#!/usr/bin/python

import os
import random

maxClients = 100
ip = '192.168.78.130'
port = 8888

os.system("cp ../Client/Client.out .")

for i in range(0, maxClients):
    with open("user.txt", "w") as f:
        f.write(str(random.randint(0, 100000000)) + ' ' 
              + str(random.randint(0, 100000000)))
    os.system("./Client.out " + ip + " > user.txt &")

os.system("rm -rf user.txt")
os.system("Client.out")
