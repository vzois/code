import sys
from random import randint
from heapq import nsmallest

import random
from datetime import datetime

import os
import struct
from mbin import storeRank, storePoints
from radixselect import findK
from skydata import genData
import time
#from pskyline import pskyline_f
from ksky_mpass import ksky_mpass

MAX_VALUE = 1024 * 1024 * 32
kss_len = 0
sfs_len = 0

def randValue():
    global MAX_VALUE
    random.seed(datetime.now())
    return randint(1,MAX_VALUE)

def DT(p,q):
    pb = False
    qb = False
    
    for i in range(len(p)):
        pb = ( p[i] < q[i] ) | pb;#At least one dimension better
        qb = ( q[i] < p[i] ) | qb;#No dimensions better
    
    return ~qb & pb

def kss(window,points, qm, pstop):
    sky=[]
    dt_num = 0
    for j in range(len(points)):
        dt = False
        #if True:
        if  pstop > qm[j] :
            q = points[j]
            dt_num = dt_num + 1
            #print rank[j],",",pstop
            for i in window:
                p = points[i]
                dt = DT(p,q)
                if dt:
                    dt = True
                    break
            
            if not dt:
                sky.append(j)
      
    print "kss dt num:",dt_num      
    return sky

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

def sfs2(points, rank):
    keys = range(len(rank))
    keys = [k for (r,k) in sorted(zip(rank,keys))]
    sky = [keys[0]]
    pc = points[keys[0]]
    
    for i in keys:
        q = points[i]
        dt = False
        #print "sky:",sky
        
        if DT(pc,q):
            dt = True
        
        print pc,q, dt,
        if not dt:
            pc_t = [0] * len(pc)
            for j in range(len(pc)):
                pc_t[j] = min(pc[j],q[j])
            pc = pc_t
            sky.append(i)
        print ">>",pc
    return sky

def sfs_test(points,rank):
    global sfs_len
    sfstime = time.time()
    sky = sfs(points,rank)
    sfstime = time.time() - sfstime
    print "sfs:",len(sky),"sfs time",sfstime
    
    #sky2 = sfs2(points,rank)
    #print "sfs2:",len(sky2)
    
    if len(sky) <=50:
        print "sfs indices:",sorted(sky)
    sfs_len = len(sky)
    return sky
    #skySet(points,sky,rank)

def kss_test(points,window,rank,pstop):
    global kss_len
    rank2 = [min(points[i]) for i in range(N)]# minimum rank
    
    ksky_mpass(points,rank)
    
    ksstime = time.time()
    sky = kss(window,points,rank2,pstop)#
    ksstime = time.time()-ksstime
    print "kss:",len(sky),"kss time:",ksstime
    kss_len = len(sky)
    sky_ret = sky
    if len(sky) <= 50:
        print "kss candidates:",sorted(sky)
    
    sky_p = [ points[i] for i in sky ] # EXTRACT CANDIDATES
    rank_p = [ rank[i] for i in sky ] # EXTRACT RANK OF CANDIDATES
    print "Running SFS.."
    ksstime = time.time()
    sky_ = sfs(sky_p,rank_p)# FIND TRUE SKYLINE POINTS
    ksstime = time.time()-ksstime
    print "kss sfs prunning time:",ksstime
    sky = [ sky[i] for i in sky_ ] # EXTRACT RELATIVE INDICES
    
    print "kss:",len(sky)
    if len(sky) <= 50:
        print "kss indices:",sorted(sky)
    #skySet(points,sky,rank)
    return sky_ret    

points = []
store = []
k=0
wpt = 0
tasklets=0
D=0
N=0

Debug = False
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

distr="i"#Choose distribution
points=genData(N,D,distr)
rank = [sum(points[i]) for i in range(N)]

#pskyline_f(points)

#############################################
#RADIX SELECT
#############################################
rdxK=findK(k,rank)

#############################################
#SKYLINE COMPUTATION
#############################################
#print points
print "-----------------------------------------------------"
window = [i for i in range(len(rank)) if (rank[i]<= rdxK)]
pp_window = [points[i] for i in window ] #
#pstop = [ for p in points min(points[i]) ]

pstop = 1024*1024*1024
for p in pp_window:
    pm=max(p)
    if pm < pstop:
        pstop = pm
        point = p
#vals = [ max(p) for p in pp_window ]
#pstop = min(vals)
#point = points[vals.index(pstop)]
print "pstop value:", pstop, hex(pstop)
print "pstop point:", point
print "-----------------------------------------------------"

####################################################
#Different Algorithms
####################################################
sk=[]
sky_ret=[]
print "Computing the skyline..."
if True:
    sk=sfs_test(points,rank)
    sky_ret=kss_test(points,window,rank,pstop)#points: list of lists, window: indices, rank: values of max dim, pstop: single value min(max(window_points))
    
#exit(1)
#exit(1)
####################################################
#STORE TEST CASE
####################################################
#print pp_
print "--------------------------------------"
storeRank(rank,rdxK)
rank2 = [min(points[i]) for i in range(N)]# minimum rank
sky_ret_points = [points[i] for i in sky_ret]
storePoints(points,rank2,pp_window,pstop)

if False:
    #for i in range(len(sky_ret_points)):
    for i in range(10):
        print [hex(d) for d in sky_ret_points[i]]

ppt = (wpt >> 2) / D
#print "window points:",len(pp_window),"points per task:",ppt,"tasklets:",tasklets, "window support:", ppt*tasklets
print"Window:"
if False:
    for i in range(0,k,ppt):
        m = 0
        for j in range(i,i+ppt):
            print i/ppt,":(", m,")",
            m+=1
            for p in pp_window[j]:
                print hex(p),
            print ""
        print ""
    print "--------------------------------------"

    print "Rank:"
    for i in range(10):
        print hex(rank[i])
    print "--------------------------------------"

    print"Points:"
    for i in range(10):
        for p in points[i]:
            print hex(p),
        print ""
    print "--------------------------------------"

print "kss len:",kss_len,hex(kss_len), len(sky_ret)
print "sky len:",sfs_len,hex(sfs_len)

bit = 0
pos = 0
bvector = [0 for i in range(N/32)]
print "bvector len:",len(bvector)
#for i in sky_ret:
#    bit = i & 0x1F
#    pos = (i & 0xFFFFFFE0) >> 5
#    print i
#    bvector[pos] = bvector[pos] | (0x1 << bit)
    
#for b in bvector:
#    print '0x%08x' % b
    
#for i in sky_ret:
#    print i



    






