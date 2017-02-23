from heapq import nsmallest
from radixselect import rselect
from skydata import genData
from heapq import nsmallest

import operator

N=0;
D=0
L=0

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

def pfmtb(v,pad):
    fmt = '0'+str(pad)+'b'
    return str(format(v, fmt))

def DT(p,q):
    pb = False
    qb = False
    
    for i in range(len(p)):
        pb = ( p[i] < q[i] ) | pb;#At least one dimension better
        qb = ( q[i] < p[i] ) | qb;#No dimensions better
    
    return ~qb & pb

def ppoints(points,L,N,D):
    step = N/(2**L) 
    pivots = [list() for i in range(D/2)]
    for kth in range(step,N,step):
        for j in range(0,D,2):
            rank = [p[j] for p in points]
            x=rselect(rank,kth)
            #x = max(nsmallest(kth,rank))
            rank = [p[j+1] for p in points]
            y=rselect(rank,kth)
            #y = max(nsmallest(kth,rank))
            pivots[j>>1].append([x,y]) 
            
    #print pivots[0]
    return pivots

def masks(p,pivots,L):
    x_dim = [ q[0] for q in pivots ] 
    y_dim = [ q[1] for q in pivots ]
    
    middle = len(pivots)/2
    x_offset = len(pivots)/2
    y_offset = len(pivots)/2
    acmsk = 0
    xmsk = 0
    ymsk = 0
    
    for i in range(0,L):
        #print "<offset>:",x_offset, y_offset,
        xbit = p[0] >= x_dim[x_offset]
        ybit = p[1] >= y_dim[y_offset]
        acmsk = acmsk << 2
        xmsk = xmsk << 1
        ymsk = ymsk << 1
        
        #print p," <p> ", [x_dim[x_offset],y_dim[y_offset]],
        msk = (xbit<<1) | ybit
        xmsk = xmsk | xbit
        ymsk = ymsk | ybit
        acmsk = (acmsk | msk)
        #print bin(msk)
        x_offset = (x_offset >> 1) if xbit == 0 else (x_offset << 1)
        y_offset = (y_offset >> 1) if ybit == 0 else (y_offset << 1)
        x_offset = min(x_offset,len(pivots)-1)
        y_offset = min(y_offset,len(pivots)-1)
    
    #print bin(acmsk),pfmtb(xmsk,L),pfmtb(ymsk,L)
    #print hex((xmsk << L) | ymsk)
    return (xmsk << L) | ymsk
    #return acmsk

def popC(v):
    c = v - ((v >> 1) & 0x55555555)
    c = ((c >> 2) & 0x33333333) + (c & 0x33333333)
    c = ((c >> 4) + c) & 0x0F0F0F0F
    c = ((c >> 8) + c) & 0x00FF00FF
    c = ((c >> 16) + c) & 0x0000FFFF
    return c

def popC4(v):
    c = v - ((v >> 1) & 0x55555555)
    c = ((c >> 2) & 0x33333333) + (c & 0x33333333)
    return c
    
def BvDT(p, Bp, q, Bq):
    global N,D,L
    x=0
    bG = D/2
    m = (1 << (L<<1) ) - 1#Group partition bits
    shfG = 0
    mD = (1 << L) - 1
    
    DTd = 1
    DTi = 0
    DTu = 0
    sLp = 0
    sLq = 0
    print "m:",m,"mD:",mD
    for i in range(bG):
        BpG = (Bp & m) >> shfG
        BpY = (BpG & mD)
        BpX = (BpG & ( mD << L ) ) >> L
        
        BqG = (Bq & m) >> shfG
        BqY = (BqG & mD )
        BqX = (BqG & mD << L ) >> L
        
        print "p:",pfmtb(BpX,L),"(",popC4(BpX),")", pfmtb(BpY,L),"(",popC4(BpY),")"
        print "q:",pfmtb(BqX,L),"(",popC4(BqX),")", pfmtb(BqY,L),"(",popC4(BqY),")"
        #DTi = (popC4(BpX) == popC4(BqX)) & ( BpX != BqX )
        
        diffX = ( BpX != BqX )
        diffY = ( BpY != BqY )
        
        DTi |= ( diffX & ( BpX < BqX ) ) & ( diffY & ( BqY < BpY ) )#incomparable case A
        DTi |= ( diffY & ( BpY < BqY ) ) & ( diffX & ( BqX < BpX ) )#incomparable case B
        sLp |= (diffX & diffY) & (BpX < BqX) & (BpY < BqY)#strictly better p
        sLq |= (diffX & diffY) & (BqX < BpX) & (BqY < BpY)#strictly worse p
        DTd &= (diffX & diffY) & (BpX < BqX) & (BpY < BqY)#
        
        #print  "DTi:",DTi, "DTd:", DTd
        #print "<===>"
        #(popC4(BpX) == popC4(BqX)) & ( BqX ==
        
        shfG+=(L<<1)
        m <<= shfG
    DTi |= (sLp & sLq)
    print  "final:"," DTi:",DTi, "DTd:", DTd
    
    

def bvector_skyline(pbv):
    global N,D,L
    pad = (L << 1)*(D >> 1)
    x=0
    sky = [pbv[0][1]]
    print "first bit vector skyline:",sky
    
    pbv.pop(0)
    for ppack in pbv:
        i = ppack[1][0]
        Bq = ppack[1][1]
        q = ppack[1][2]
        print "q:",pfmtb(Bq,pad),q
        
        for qpack in sky:
            #print "qpack:",qpack
            j = qpack[0]
            Bp = qpack[1]
            p = qpack[2]
            print "p:",pfmtb(Bp,pad),p
            BvDT(p,Bp,q,Bq)
        
        break

def pskyline_f(points,rank):
    global L
    N = len(points)
    D = len(points[0])
    print "Computing pivots..."
    pivots = ppoints(points,L,N,D)
    
    for pvt in pivots:
        print [p[0] for p in pvt]
        print [p[1] for p in pvt]
        print "---------------------------->"
    
    pbv = list()
    pad = (L << 1)*(D >> 1)
    print "Computing Masks..."
    for i in range(len(points)):
        p = points[i]
        prank = rank[i]
        #print "p:",p
        pmsk = 0
        shf = (L<<1) - L
        for j in range(0,D,2):
            #print "pivots:",pivots[j>>1]
            pmsk = (pmsk << (L << 1))
            msk = masks(p[j:j+2],pivots[j>>1],L)
            pmsk = msk | pmsk
            #pmsk = pmsk | (msk << shf)
            #shf-=L
            #print pfmtb(pmsk,pad)
            #print hex(pmsk)
            #pad = pad + (L << 1)
        #print pfmtb(pmsk,pad)
        #return 
        #break
        pbv.append([prank,[i,pmsk,p]])
    #return
    pbv.sort(key=operator.itemgetter(0))
    
    print pbv[0]
    print pbv[1]
    bvector_skyline(pbv)
    
    #print pbv

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
    elif line.strip().startswith("#define L"):
        L = int(line.strip().split(" ")[2])

fp.close()

distr="a"#Choose distribution
points=genData(N,D,distr)
rank = [sum(points[i]) for i in range(N)]


#sky_sfs=sfs(points,rank)
#print "first point sfs:",sky_sfs[0]
pskyline_f(points,rank)
    
    
    
    