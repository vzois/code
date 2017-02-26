import os
import struct

def storeRank(rank,num):
    f = open("runtime_data/sum.bin","wb")
    
    num = 0;
    shf = 4 << 2
    mD = 0xF << shf
    mN = 0x0
    
    f.write(struct.pack('i', 0x1000))
    for i in range(1023):
        f.write(struct.pack('i', 0x0))
        
    for r in rank:
        f.write(struct.pack('i', r))
    
    f.write(struct.pack('i', num))
    f.write(struct.pack('i', shf))
    f.write(struct.pack('i', mD))
    f.write(struct.pack('i', mN))
    
    f.close()
 
def storePoints(points,rank,window,pstop):
    f = open("runtime_data/points.bin","wb")
    
    f.write(struct.pack('i', pstop))
    for i in range(1023):
        f.write(struct.pack('i', 0x0))
        
    for p in points:
        for d in p:
            f.write(struct.pack('i', d))
    
    for w in window:
        for v in w:
            f.write(struct.pack('i', v))
            
    for r in rank:
        f.write(struct.pack('i', r))
    
    f.close()

def storeDSkyData(parts_p,parts_r,part_i):
    Debug = True
    f = open("runtime_data/dsky.bin","wb")
    
    for i in range(4096):
        f.write(struct.pack('i', 0x0))
    
    count = 0
    printc = 1
    
    print "Storing points in partitions!!!"
    for pp in parts_p:# for each partition
        if count == 0 and False:
            count=0
            for p in pp:
                if count % 16 == 0:
                    print "count",count/16
                count+=1
                for d in p:
                    print hex(d),
                print""
        for p in pp:# for each point in the partition
            for d in p:# write binary file of points
                f.write(struct.pack('i', d))
        count+=1
    
    count = 0
    print "Storing ranks in partitions!!!"
    
    #for pr in parts_r:
    #    if count < printc and False:
    #        print hex(pr[0])
    #   for r in pr:
    #        f.write(struct.pack('i', r))
    #    count+=1
    
    for pp in parts_p:
        for p in pp:
            pm = min(p)
            
            if count < printc and Debug:
                print [hex(v) for v in p],hex(pm)
            count+=1
            f.write(struct.pack('i', pm))
        
        if Debug:
            print "--------------->"
            count = 0
    
    f.close()
    
        
    