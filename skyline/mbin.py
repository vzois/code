import os
import struct

def storeRank(rank,num):
    f = open("runtime_data/sum.bin","wb")
    
    num = 0;
    shf = 4 << 2
    mD = 0xF << shf
    mN = 0x0
    
    f.write(struct.pack('I', 0x1000))
    for i in range(1023):
        f.write(struct.pack('I', 0x0))
        
    for r in rank:
        f.write(struct.pack('I', r))
    
    f.write(struct.pack('I', num))
    f.write(struct.pack('I', shf))
    f.write(struct.pack('I', mD))
    f.write(struct.pack('I', mN))
    
    f.close()
 
def storePoints(points,rank,window,pstop):
    f = open("runtime_data/points.bin","wb")
    
    f.write(struct.pack('I', pstop))
    for i in range(1023):
        f.write(struct.pack('I', 0x0))
        
    for p in points:
        for d in p:
            f.write(struct.pack('I', d))
    
    for w in window:
        for v in w:
            f.write(struct.pack('I', v))
            
    for r in rank:
        f.write(struct.pack('I', r))
    
    f.close()

def storeDSkyData(parts_p,parts_r,parts_i,parts_b, Remote):
    Debug = False
    f = open("runtime_data/dsky.bin","wb")
    
    for i in range(3072):
        f.write(struct.pack('I', 0x0))
    
    count = 0
    printc = 10
    
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
                f.write(struct.pack('I', d))
        count+=1
    
    count = 0
    
    #REMOTE POINT RESERVATION
    if Remote:
        for i in range(512):
            for j in range(len(parts_p[0])):
                f.write(struct.pack('I', 0))

    print "Storing ranks in partitions!!!"    
    for pr in parts_r:
        if count < printc and False:
            print hex(pr[0])
        for r in pr:
            f.write(struct.pack('I', r))
        count+=1

    count = 0
    
    #REMOTE RANK RESERVATION
    if Remote:
        for i in range(512):
            f.write(struct.pack('I', 0))
    
    D=len(parts_p[0][0])
    print "Storing bvectors in partitions!!!"
    for i in range((len(parts_p))):
        pp=parts_p[i]
        rp=parts_r[i]
        ip=parts_i[i]
        bp=parts_b[i]
        #break
        if count < 0:
            print "<",i,">-----------------"
            
        for j in range(len(pp)):
            x=0
            shf=0
            for b in bp[j]:
                x = x | (b <<shf)
                shf+=D
            
            if count < 0:
                print "p:",[ hex(v) for v in pp[j] ],"r:",hex(rp[j]),"b:",hex(x)
            f.write(struct.pack('I', x))
        
        
          
        count+=1
        #raw_input("Press any key to continue...")
        #break
        
    #REMOTE BVECTOR RESERVATION
    if Remote:
        for i in range(512):
            f.write(struct.pack('I', 0))   
        
        
    
        
    