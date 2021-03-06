from heapq import nsmallest
from radixselect import rselect
from skydata import genData
from heapq import nsmallest
import math
import operator
import time
import sys

from mbin import storeDSkyData
from skydata import scale

N=0
D=0
P=0
L=0
PSIZE=0
parts_p=list()
parts_i=list()
parts_r=list()
parts_b=list()

dt_num_red = 0
dt_num = 0

sys.setrecursionlimit(100000)

def pfmtb(v):
    return "{0:#0{1}x}".format(v,10)
    #return str(format(v, fmt))
    
def pfmtbin(v):
    return format(v, '#06b')
    #return str(format(v, fmt))

def DT(p,q):
    pb = False
    qb = False
    
    for i in range(len(p)):
        pb = ( p[i] < q[i] ) | pb;#At least one dimension better
        qb = ( q[i] < p[i] ) | qb;#No dimensions better
    
    return ~qb & pb

def split(indices,sumf,n):
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
        split(li,ls,n/2)
        split(ri,rs,n/2)     

def popC(v):
    c = v - ((v >> 1) & 0x55555555)
    c = ((c >> 2) & 0x33333333) + (c & 0x33333333)
    c = ((c >> 4) + c) & 0x0F0F0F0F
    c = ((c >> 8) + c) & 0x00FF00FF
    c = ((c >> 16) + c) & 0x0000FFFF
    return c

def popc_4d(v):
    x=0

def createBitVectors(points):
    global N,D,L
    
    pivots=list()
    step = N/(2**L) 
    for i in range(D):
        dim_vec = [ p[i] for p in points ]
        pp = list()
        
        for kth in range(step,N,step):
            kval = rselect(dim_vec,kth)
            pp.append(kval)
        pivots.append(pp)
    
    #print pivots
    
    p_vecs = list()
    for p in points:
        bit=0
        vecs = [0 for m in range(L)]
        for i in range(D):
            pvt = pivots[i]
            offset = len(pvt)/2
            
            k=0
            for j in range(0,L):
                set = p[i] >= pvt[offset]
                if set:
                    vecs[k] = vecs[k] ^ (0x1 << bit);
                offset = (offset << 1) if set else (offset >> 1)
                offset = min(offset,len(pvt)-1)
                #print p[i],">=", pvt[offset],set,vecs[k]
                k+=1
            bit+=1
        #print "<",p,">",[pfmtbin(v) for v in vecs]
        #raw_input("Press Enter to continue...")
        p_vecs.append(vecs)      
    
    return p_vecs
  
def sortSplit(points,lvl):
    global N,D,P,parts_p,parts_r,parts_i,parts_b
    sumf = [sum(p) for p in points]
    minf = [min(p) for p in points]
    indices = [i for i in range(N)]
    p_vecs=createBitVectors(points)
    p =[[indices[i],minf[i],sumf[i]] for i in range(N)]
    p = sorted(p, key = operator.itemgetter(1,2))
    
    
    count = 0
    pp = list()
    pr = list()
    pi = list()
    pb = list()
    
    psize = N/P
    for v in p:
        i = v[0]
        pp.append(points[i])
        pr.append(minf[i])
        pb.append(p_vecs[i])
        pi.append(i)
        count+=1
        if count == psize:
            #print pp[0],pi[0]
            parts_p.append(pp)
            parts_r.append(pr)
            parts_i.append(pi)
            parts_b.append(pb)
            pp=list()
            pr=list()
            pi=list()
            pb=list()
            count = 0
    print "parts_p len:",len(parts_p)
    print "parts_r len:",len(parts_r)
    
def createPartitions(points,sumf):
    global N,D,P,parts_p,parts_r,parts_i,parts_b
    x=0
    kth = N/2
    
    p_vecs=createBitVectors(points)
    indices = [i for i in range(N)]
    split(indices,sumf,N)#Recursive split
    #sortSplit(points,sumf)
    
    stddev = 0
    for pi in parts_i:
        stddev+= ((N/P)-len(pi))**2
        
    stddev/=P
    print "stddev:",math.sqrt(stddev)#smaller when large variation in values within dimensions
    
    part_i_t = list()
    part_r_t = list()
    for i in range(len(parts_i)):#build partitions
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
        parts_b.append([p_vecs[i] for i in pi ])
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

def sfs_p(points, rank, bvecs):
    global dt_num_red,dt_num
    keys = range(len(rank))
    #keys = [k for (r,k) in sorted(zip(rank,keys))]
    sky = [keys[0]]
    for i in keys:
        q = points[i]
        Mi = bvecs[i][0]
        Qi = bvecs[i][1]
        dt = False
        #print "sky:",sky
        for j in sky:
            #print j
            p = points[j]
            Mj = bvecs[j][0]
            Qj = bvecs[j][1]
            dt_num+=1# count regular DTs
            
            #print i,"=",p,",",q, "<",DT(p,q)
            #Bitvector DT comparison#
            if ((Mi | Mj) > Mj):
                continue
            elif popC(Mi) < popC(Mj):
                continue
            elif (((Mj | ~Mi) & Qj) | Qi) > Qi:
                continue
            elif (Mi == Mj) and ((Qj | Qi) > Qi):
                continue
            
            if DT(p,q):
                dt = True
                break
            dt_num_red+=1# count reduced DTs
            
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
    global D
    global minC
    global parts_p,parts_r, parts_i, parts_b
    global PSIZE
    global dt_num_red,dt_num
    
    count_part_alive = 0
    gsky = []
    part = 0
    part_index = 0;
    total = 0;
    gsky_i = list()
    g_ps = 0xFFFFFFFF
    gsky_b = list()
    early_stop = 0
    for i in range(len(parts_p)):
        #print "i:",i,len(parts_p),len(parts_r)
        pp=parts_p[i]#get points in the partition
        rp=parts_r[i]#get level of each point in the partition
        ip=parts_i[i]#get index of each point in the partition
        bp=parts_b[i]#get bit vectors for each point in the partition
        
        l = rp[0] if minC else (rp[0] - (D-1)*scale)#compute level based on monotone function
        if g_ps <= l:#if level greater than or equal to the global pstop then all partitions including this are pruned
            print "Stoping on [",part_index,"]: <g_ps>=",hex(g_ps),",<level>=",hex(rp[0]),",<first point of part>=",[hex(d) for d in  pp[0]]
            break
        early_stop+=1
        
        tsky = sfs_p(pp,rp,bp) # self prune partition
        for j in tsky: # prune surviving points against global skyline
            q = pp[j]
            Mi = bp[j][0]
            Qi = bp[j][1]
            dt = 0
            
            for k in range(len(gsky)):#check points in the global skyline
                p = gsky[k]
                Mj = gsky_b[k][0]
                Qj = gsky_b[k][1]
                
                dt_num+=1# count regular DTs
                #Fast DT with bitvectors
                if ((Mj | Mi) > Mi):
                    continue
                elif popC(Mi) < popC(Mj):
                    continue
                elif (((Mj | ~Mi) & Qj) | Qi) > Qi:
                    continue
                elif (Mi == Mj) and ((Qj | Qi) > Qi):
                    continue
                dt_num_red+=1# count reduced DTs
                
                if DT(p,q):
                    dt = 1
                    break
            
            if dt == 0:#update skyline information when point is not dominated
                g_ps=min([max(pp[j]),g_ps])#update stop point
                gsky_i.append(j+total)#index of local skyline
                
                gsky.append(q)#append point in the global skyline
                gsky_b.append(bp[j])# append bit vector to skyline
        
        if (len(gsky_i) > 0):
            count_part_alive+=1
            
        if (part_index < 1 and (len(gsky_i) > 0)) and True:#debugging data
            print "{",hex(g_ps),"}<",part_index,"> = [",len(gsky_i),",",hex(len(gsky_i)),"]"
            print gsky_i
            bit_vectors(gsky_i)
            
                
        part_index+=1
        total+=PSIZE
        gsky_i = list()
    print "dkyline len:",len(gsky), hex(len(gsky))
    print "dskyline total full DTs:",dt_num
    print "dskyline reduced full DTs:",dt_num_red
    print "dskyline alive part count:",count_part_alive
    print "dskyline early stop:",early_stop

print "Generating data...."    
fp = open("common/config.h","r")
distr="c"
remote = False
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
    elif line.strip().startswith("#define L"):
        L = int(line.strip().split(" ")[2])
    elif line.strip().startswith("#define DISTR"):
        distr = line.strip().split(" ")[2][1]
    elif line.strip().startswith("#define SEQ_DSKY"):
        remote = False
    elif line.strip().startswith("//#define SEQ_DSKY"):
        remote = True
fp.close()

def part_cmp_count(P,DPUS):
    Pc = P/DPUS
    M = DPUS-1
    count = 2 * M + 1
    mt = (Pc - 2)/2
    print "mt:",mt
    count += mt * ((2 * M + 1) + (2 * M + 2))
    print "xx:",((2 * M + 1) + (2 * M + 2))
    return count
        
print "scale factor:",scale    

minC = True
print distr
#distr="i"#Choose distribution
points=genData(N,D,distr)

if minC:
    rank = [min(points[i]) for i in range(N)]
else:
    rank = [sum(points[i]) for i in range(N)]

if False:
    dskyline_t = time.time()
    #createPartitions(points,rank)
    sortSplit(points,rank)
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
    createpart_t = time.time()
    #createPartitions(points,rank)
    sortSplit(points,rank)
    createpart_t = time.time() - createpart_t
    
    dskyline_t = time.time()
    #dskyline()
    dskyline_t = time.time() - dskyline_t
    print "dskyline elapsed time:",dskyline_t
    print "create partition elapsed time:",createpart_t

#Execution Information
dpus = 2048
part_n = (N*dpus)/PSIZE
part_1 = N/PSIZE
print "dpus used:",dpus
print "partition count:",part_1
print "partition comparison count:", sum([i for i in range(1,part_1+1)])
#print "single dpu partition count on complete data:",part_n
#print "multi dpu partition count on complete data:",part_1

cmp_single_dpu = spiral(131072,2048)
#cmp_multi_dpu = spiral((N*dpus)/PSIZE,2048)
print "assignment dpu comparison count:",cmp_single_dpu
#print "equation dpu comparison count:",part_cmp_count(8192,2048)
#print "multi dpu comparison count:",cmp_multi_dpu

print "---------------------------------------------"
storeDSkyData(parts_p,parts_r,parts_i,parts_b, remote)


