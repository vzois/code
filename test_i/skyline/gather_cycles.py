import sys

dataset = sys.argv[1]
f = open("out","r")

lines = f.readlines()

total_cycles = 0
check = False
for line in lines:
    if check:
        data = line.strip().split(" ")
        cycles=long(data[0])
        total_cycles+=cycles
        #print line, data[0]
        check = False
    if line.strip().startswith("end"):
        check = True

f.close()

print dataset
print "total cycles:",total_cycles