from heapq import nsmallest
from radixselect import rselect
from skydata import genData
from heapq import nsmallest
import math
import operator
import time

from mbin import storeDSkyData
from skydata import scale

N=0
D=0
P=0
PSIZE=0
parts_p=list()
parts_i=list()
parts_r=list()

def pfmtb(v):
    return "{0:#0{1}x}".format(v,10)
    #return str(format(v, fmt))

def DT(p,q):
    pb = False
    qb = False
    
    for i in range(len(p)):
        pb = ( p[i] < q[i] ) | pb;#At least one dimension better
        qb = ( q[i] < p[i] ) | qb;#No dimensions better
    
    return ~qb & pb

def split(points,sumf,n):
    global N,D,P,parts_p,parts_r,parts_i
    if N/P == n:
        parts_p.append(points)
        parts_r.append(sumf)
    else:
        kval = rselect(sumf,n/2)
        print "kval:",kval
        lp = [points[i] for i in range(len(points)) if sumf[i] <= kval ]
        ls = [sumf[i] for i in range(len(points)) if sumf[i] <= kval ]
        rp = [points[i] for i in range(len(points)) if sumf[i] > kval ]
        rs = [sumf[i] for i in range(len(points)) if sumf[i] > kval ]
        split(lp,ls,n/2)
        split(rp,rs,n/2)

def split2(indices,sumf,n):
    global N,D,P,parts_p,parts_r,parts_i
    if N/P == n:
        parts_i.append(indices)
        parts_r.append(sumf)
    else:
        kval = rselect(sumf,n/2)
        #print "kval:",kval
        li = [indices[i] for i in range(len(indices)) if sumf[i] <= kval ]
        ls = [sumf[i] for i in range(len(indices)) if sumf[i] <= kval ]
        ri = [indices[i] for i in range(len(indices)) if sumf[i] > kval ]
        rs = [sumf[i] for i in range(len(indices)) if sumf[i] > kval ]
        split2(li,ls,n/2)
        split2(ri,rs,n/2)     

def createPartitions(points,sumf):
    global N,D,P,parts_p,parts_r,parts_i
    x=0
    kth = N/2
    
    indices = [i for i in range(len(sumf))]
    split2(indices,sumf,N)
    
    stddev = 0
    for pi in parts_i:
        stddev+= ((N/P)-len(pi))**2
        
    stddev/=P
    print "stddev:",math.sqrt(stddev)#smaller when large variation in values within dimensions
    
    part_i_t = list()
    part_r_t = list()
    for i in range(len(parts_i)):
        z = sorted(zip(parts_r[i],parts_i[i]))
        rnk,ind = zip(*z)
        #print rnk
        #print ind
        part_i_t.append(list(ind))
        part_r_t.append(list(rnk))
        #break
    parts_i = part_i_t
    parts_r = part_r_t
    
    #print "parts_i: (0)",parts_i[0]
    #print "parts_r: (0)",parts_r[0]
    #print "parts_i: (1)",parts_i[1]
    #print "parts_r: (1)",parts_r[1]
    for pi in parts_i:
        parts_p.append([points[i] for i in pi ])
    
    #print "parts_p: (0)",parts_p[0], sum(parts_p[0][0])

def spiral(parts,dpus):
    
    if dpus > parts:
        print "dpus > parts"
        return
    
    count=0
    cmp=0
    for i in range(0,parts,dpus):
        
        if count % 2 == 0:
            cmp+=i
            #for j in range(i,i+dpus,1):
            #    print j,
        else:
            cmp+=i+dpus-1
            #for j in range(i+dpus-1,i-1,-1):
            #    print j,
        count+=1       
        #print ""
    #print "partition comparison count per DPU:",cmp
    return cmp

def sfs(points, rank):
    keys = range(len(rank))
    keys = [k for (r,k) in sorted(zip(rank,keys))]
    sky = [keys[0]]
    for i in keys:
        q = points[i]
        dt = False
        #print "sky:",sky
        for j in sky:
            #print j
            p = points[j]
            #print i,"=",p,",",q, "<",DT(p,q)
            if DT(p,q):
                dt = True
                break
        if not dt and (i not in sky):
            sky.append(i)
    
    return sky

def sfs_p(points, rank):
    keys = range(len(rank))
    #keys = [k for (r,k) in sorted(zip(rank,keys))]
    sky = [keys[0]]
    for i in keys:
        q = points[i]
        dt = False
        #print "sky:",sky
        for j in sky:
            #print j
            p = points[j]
            #print i,"=",p,",",q, "<",DT(p,q)
            if DT(p,q):
                dt = True
                break
        if not dt and (i not in sky):
            sky.append(i)
    
    return sky

def bit_vectors(gsky_i):
    bv = [0 for i in range(PSIZE/32)]
    for i in gsky_i:
        pbit = i & 0x1F
        ppos = (i & 0xE0) >> 5
        bv[ppos] = bv[ppos] | (0x1 << pbit)
        
    print "<<{"
    for v in bv:
        print pfmtb(v) 
    #[ pfmtb(v) for v in bv ]
    
    print "}>>"
    

def dskyline():
    global parts_p,parts_r, parts_i
    global PSIZE
    
    gsky = []
    part = 0
    part_index = 0;
    total = 0;
    gsky_i = list()
    g_ps = 0xFFFFFFFF
    for i in range(len(parts_p)):
        pp=parts_p[i]
        rp=parts_r[i]
        ip=parts_i[i]
        if g_ps <= rp[0] :
            break
        tsky = sfs_p(pp,rp)
        for j in tsky:
            q = pp[j]
            dt = 0
            
            for p in gsky:
                if DT(p,q):
                    dt = 1
                    break
            
            if dt == 0:
                g_ps=min([max(pp[j]),g_ps])
                
                gsky_i.append(j+total)
                gsky.append(q)
        
        if (part_index < 1 or (len(gsky_i) > 0)) and True:
            print "{",hex(g_ps),"}<",part_index,"> = [",len(gsky_i),",",hex(len(gsky_i)),"]"
            print gsky_i
            bit_vectors(gsky_i)
                
        part_index+=1
        total+=PSIZE
        gsky_i = list()
    print "dkyline len:",len(gsky), hex(len(gsky))
 
print "Generating data...."    
fp = open("common/config.h","r")
for line in fp.readlines():
#print line
    if line.strip().startswith("#define DATA_N"):
        N = int(line.strip().split(" ")[2])
    elif line.strip().startswith("#define D "):
        D = int(line.strip().split(" ")[2])
    elif line.strip().startswith("#define K"):
        k = int(line.strip().split(" ")[2])
    elif line.strip().startswith("#define WRAM_BUFFER "):
        wpt = int(line.strip().split(" ")[2])
    elif line.strip().startswith("#define TASKLETS"):
        tasklets = int(line.strip().split(" ")[2])
    elif line.strip().startswith("#define PSIZE "):
        PSIZE = int(line.strip().split(" ")[2])
        P = N/PSIZE

fp.close()    
        
print "scale factor:",scale    

distr="i"#Choose distribution
points=genData(N,D,distr)
rank = [min(min(points[i]),sum(points[i])) for i in range(N)]
#rank = [sum(points[i]) for i in range(N)]
#rank = [min(points[i]) for i in range(N)]

if False:
    dskyline_t = time.time()
    createPartitions(points,rank)
    dskyline()
    dskyline_t = time.time() - dskyline_t
    print "dskyline elapsed time:",dskyline_t

    sfs_t = time.time()
    sky_sfs=sfs(points,rank)
    sfs_t = time.time() - sfs_t
    print "sfs elapsed time:",sfs_t
    print "sfs len:",len(sky_sfs)
else:
    print "Creating Partitions..."
    dskyline_t = time.time()
    createPartitions(points,rank)
    dskyline()
    dskyline_t = time.time() - dskyline_t
    print "create partitions elapsed time:",dskyline_t

#Execution Information
dpus = 2048
part_n = (N*dpus)/PSIZE
part_1 = N/PSIZE
print "dpus used:",dpus
print "single dpu partition count on complete data:",part_n
print "multi dpu partition count on complete data:",part_1

cmp_single_dpu = spiral(N/PSIZE,1)
cmp_multi_dpu = spiral((N*dpus)/PSIZE,2048)
print "single dpu comparison count:",cmp_single_dpu
print "multi dpu comparison count:",cmp_multi_dpu

storeDSkyData(parts_p,parts_r,parts_i)


