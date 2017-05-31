import os.path

if not os.path.isfile("simpy.out") or  not os.path.isfile("sim.out"):
    print "<WARNING>: Result files not generated!!! Uncomment line 461 in dskyline.py to compare results!!!"
    exit(1)
      

fpy = open("simpy.out","r")
fdpu = open("sim.out","r")

linespy = fpy.readlines()
linesdpu = fdpu.readlines()

pyd = dict()
for line in linespy:
    if line.startswith("0x"):
        data = line.strip().split("|")
        pyd[data[0].strip()] = data[1].strip()
        #print data[0]+data[1]

dpud = dict()
for line in linesdpu:
    if line.startswith("0x"):
        data = line.strip().split("|")
        dpud[data[0].strip()] = data[1].strip()
        #print data[0]+data[1]

error = False
for addr in pyd:
    py_v = pyd[addr]
    dpu_v =dpud[addr]
    #print addr,py_v,dpu_v
    if py_v != dpu_v:
        print "<ERROR>:",addr,py_v,dpu_v
        error = True
   
if not error:
    print "Results Verified!!!"

fpy.close()
fdpu.close()