import operator

def hSkyline(points):
    N = len(points)
    D = len(points[0])
    #print points[:][0]
    
    print"(",N,D,")"
    
    indices = [list() for i in range(D)]
    for i in range(D):
        for j in range(N):
            #print points[j][i]
            indices[i].append((j,points[j][i]))
            #tmp.append((j,points[j][i]))
        
        #indices[i].append(tmp)
        #break
    
    for i in range(D):
        indices[i].sort(key=operator.itemgetter(1))
    
    ##count = 5
    print"----------------"
    for i in range(D):
        print indices[i][0]
    print"----------------"    
    stop = False
    
    j=0
    sky = dict()
    for j in range(N):
        print ""
        for i in range(D):
            tmp = indices[i][j]
            print i,":",points[tmp[0]],tmp[0],
            if tmp[0] in sky:
                if (sky[tmp[0]] + 1 == D):
                    sky[tmp[0]]+=1
                    stop = True
                    
        if stop == True:
            break
        
        for i in range(D):
            tmp = indices[i][j]
            if tmp[0] in sky:
                sky[tmp[0]]+=1
            else:
                sky[tmp[0]]=1
        #j+=1
        #if j > 2:
        #    break
    
    S=list()
    for k in sky:
        #if sky[k] == D:
        S.append(k)
    #print S
    print ""
    return S
        
