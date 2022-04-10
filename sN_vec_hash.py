#!/usr/bin/env python

import random
import numpy as np

class SumNVecHasher():
    def __init__(self, N, vec_dict):
        self.N = N
        self.vecs = vec_dict

    def vecSum(self, words):
        return np.sum([self.vecs[x] for x in words], axis=0)

    def concat(self, words):
        return ''.join(words)
        
    def sumNHash(self, userwords, currwords, totals):
        if len(currwords) == self.N:
            totals.append(self.vecSum(currwords))
            return

        for i in range(0, len(userwords)):   # iterate over all n-element combinations
            newwords = currwords[:]
            newwords.append(userwords[i])
            self.sumNHash(userwords[i+1:], newwords, totals)




###################################################################
def main():


    N = 4   # change this for the desired security level sN
    userwords = ['a','b','c','d','e']

    vec_dict = {}
    for x in userwords:
        vec_dict[x] = (random.randint(-100,100), random.randint(-100, 100))
    
    sumHasher = SumNVecHasher(N, vec_dict)
    
    totals = []
    sumHasher.sumNHash(userwords, [], totals)  # encryption done here

    print(totals)
 
if __name__ == '__main__':
    main()
