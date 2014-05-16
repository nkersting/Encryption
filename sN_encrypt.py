#!/usr/bin/env python


# N-Sum encryption for any N
#  BEFORE RUNNING: make sure this script and "omegaO2.txt" are in the same directory.
#  USAGE: Simply run this script and, when prompted, enter the message you wish to encrypt.
#  OUTPUT: Sorted list of encryption keys written to "keys.txt", one per line
########################################################################################
# This script produces usable sum-encrypted messages, but there are still some
# additional needed improvements. For example,
# 1. tokenization, e.g. handle punctuation (note convert "read." to "read" but not "U.S." to "U.S")
# 2. lemmatization, e.g. convert plural nouns to singular (WordNet has no plurals)
# 3. hash words not in the dictionary to unused unique integers (WordNet uses integers between 0 and roughly 1.6*10^7) 
# 4. compound words (connected with "_")
# 5. Part of Speech differentiation
########################################################################################

import csv
import shlex

###################################################################
def MapEntry(entryword, syndict, stopwords):
    if (syndict.has_key(entryword) and entryword not in stopwords):
        return syndict[entryword]
    else:
        return []

###################################################################
def SumWordsDFSRecursive(currwords, index, syndict, stopwords, sublist, sumdict):
    if index == len(currwords):
        total = sum(sublist)         # key computed here
        if (sumdict.has_key(total) == False):
            sumdict[total] = []
        for item in sublist:
            sumdict[total].append(item)  # build the inverted dictionary
        return
        
    for entry in MapEntry(currwords[index], syndict, stopwords):
        templist = sublist[:]
        templist.append(entry)
        SumWordsDFSRecursive(currwords, index+1, syndict, stopwords, templist, sumdict)

###################################################################
def SumNEncrypt(userwords, currwords, syndict, stopwords, N, sumdict):
    if len(currwords) == N:
        sublist = []
        SumWordsDFSRecursive(currwords, 0, syndict, stopwords, sublist, sumdict)
        return

    for i in range(0, len(userwords)):   # iterate over all n-element combinations
        newwords = currwords[:]
        newwords.append(userwords[i])
        SumNEncrypt(userwords[i+1:], newwords, syndict, stopwords, N, sumdict)

###################################################################
def main():

    N = 2   # change this for the desired security level sN
    stopwords = []    # append words to this list to exclude from encryption, such as "a", "the", "it", etc.
    synfile = "omegaO2.txt"    # map file defines M: x -> Omega_x
    outfile = open("keys.txt", "w")   # output as a text file, one key per line

    synReader = csv.reader(open(synfile,'rb'), delimiter=' ')   # read map file
    syns = []
    for synline in synReader:
        syns.append(synline)

    syndict = {}        # convert to a dictionary for faster access
    for entryline in syns:
        syndict[entryline[0]] = [int(x) for x in entryline[1:]]

    usertext = raw_input('Message to encrypt: ')    # user input
    userwords = shlex.split(usertext)

    sumdict = {}
    SumNEncrypt(userwords, [], syndict, stopwords, N, sumdict)  # encryption done here

    for key in sorted(sumdict.keys()):              # write output with one sum per line
        outfile.write(str(key) + '\n')
 
if __name__ == '__main__':
    main()
  
