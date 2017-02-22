import sys
from random import randint
from heapq import nsmallest

import random
from datetime import datetime

import os
import struct
from mbin import storeRank, storePoints

import time

MAX_VALUE = 1024*1024*1024

def randValue():
    global MAX_VALUE
    random.seed(datetime.now())
    return randint(1,MAX_VALUE)

def rselect(input,k):
    tmpK = k
    num = 0x0
    mN = 0x0
    mD = 0x0
    tmpV = input
    
    for d in range(7,-1,-1):
        buckets = [0 for i in range(17)]
        shf = d << 2
        mD = (0xF << shf)
        for i in input:
            digit = (i & mD) >> shf
            inc = ((i & mN) == num )
            buckets[digit+1]+=inc
        
        #print [ v for v in buckets ]
        pos = 0    
        for i in range(1,16):
            buckets[i+1] += buckets[i]
        
        for i in range(1,17):
            if tmpK <= buckets[i]:
                pos = i
                break
            
        if tmpK != buckets[i-1]:
            tmpK = tmpK - buckets[i-1]
        
        num = num | ((pos-1) <<shf)
        mN = mN | mD
        #print hex(mN), hex(mD)
    return num

def findK(k,rank):
   nsmall = nsmallest(k,rank)
   heapK=max(nsmall)
   rdxK=rselect(rank,k)
   print "(heapK) k smallest element:", heapK,hex(heapK)
   print "(rdxK) k smallest element:", rdxK,hex(rdxK)
   return rdxK

    






