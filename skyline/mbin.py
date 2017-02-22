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

        
    