import sys

#dataset = sys.argv[1]
f = open("sim.out","r")

lines = f.readlines()

total_cycles = 0
check = False
count=0
for line in lines:
    #print line.strip()
    if check:
        data = line.strip().split(" ")
        cycles=long(data[0])
        total_cycles+=cycles
        count+=1
        #print line, data[0]
        check = False
    if line.strip().startswith("End") or line.strip().startswith("Halted"):
        check = True

f.close()

#print dataset
print "total cycles:",total_cycles, count