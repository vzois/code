from radixselect import findK
from radixselect import rselect

from heapq import nsmallest
import time

def DT(p,q):
    pb = False
    qb = False
    
    for i in range(len(p)):
        pb = ( p[i] < q[i] ) | pb;#At least one dimension better
        qb = ( q[i] < p[i] ) | qb;#No dimensions better
    
    return ~qb & pb

def kss(window,points):
    sky=[]
    for j in range(len(points)):
        dt = False
        q = points[j]
        #print rank[j],",",pstop
        for i in window:
            p = points[i]
            dt = DT(p,q)
            if dt:
                dt = True
                break
            
        if not dt:
            sky.append(j)      
    return sky

def ksky_mpass(points,sum):
    K = 1024
    ppoints = points
    psum = sum
    size = len(ppoints)
    
    ksky_mpass_time = time.time()
    iter = 0
    while True:
        kval=rselect(psum,K)
        #print "kth-largest element:",kval
        window = [i for i in range(len(psum)) if (psum[i]<= kval)]
        pp_window = [ppoints[i] for i in window ]
        sky = kss(window,ppoints)
        psum = [psum[i] for i in sky]
        ppoints = [ppoints[i] for i in sky]
        #print "ppoints size:",len(ppoints)
        if size == len(ppoints) or K == len(ppoints)-1:
            break
        K = min(K << 1,len(ppoints)-1)
        print "K:",K
        size = len(ppoints)
        iter = iter + 1
    print "ppoints size:",len(ppoints)
    ksky_mpass_time = time.time() - ksky_mpass_time
    print "ksky_mpass_tim:",ksky_mpass_time
    print "iter:",iter
     
    
    
    