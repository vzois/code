from heapq import nsmallest
from radixselect import rselect
from skydata import genData

def ppoints(points,L,N,D):
    step = N/(2**L) 
    pivots = [list() for i in range(D/2)]
    for kth in range(step,N,step):
        for j in range(0,D,2):
            rank = [p[j] for p in points]
            x=rselect(rank,kth)
            rank = [p[j+1] for p in points]
            y=rselect(rank,kth)
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
    for i in range(0,L):
        print "<offset>:",x_offset, y_offset,
        xbit = p[0] >= x_dim[x_offset]
        ybit = p[1] >= y_dim[y_offset]
        acmsk = acmsk << 2
        
        print p," <p> ", [x_dim[x_offset],y_dim[y_offset]],
        msk = (xbit<<1) | ybit
        acmsk = (acmsk | msk)
        print bin(msk)
        x_offset = (x_offset >> 1) if xbit == 0 else (x_offset << 1)
        y_offset = (y_offset >> 1) if ybit == 0 else (y_offset << 1)
        x_offset = min(x_offset,len(pivots)-1)
        y_offset = min(y_offset,len(pivots)-1)
    
    print bin(acmsk)
    return acmsk

def pskyline_f(points):
    L = 3
    N = len(points)
    D = len(points[0])
    pivots = ppoints(points,L,N,D)
    
    
    for p in points:
        print p
        for i in range(0,D,2):
            print "pivots:",pivots[i]
            masks(p[i:i+2],pivots[i],L)
            break
        
        break
    


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

fp.close()

distr="a"#Choose distribution
points=genData(N,D,distr)
rank = [sum(points[i]) for i in range(N)]

pskyline_f(points)
    
    
    
    