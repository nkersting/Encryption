#!/usr/bin/env python


# N-sum encryption for any N
# Only tested up to N = 4 (output files tend to be O(100) MB!)

#  BEFORE RUNNING: make sure this script and "omega-hybrid.txt" are in the same directory.
#  USAGE:  Simply run this script and, when prompted, enter the message you wish to encrypt.
#  OUTPUT: File of the form "diy_keys+<timestamp>.kyx". Feel free to rename this file to your tastes, but please
#          keep the *.kyx extension.

#######################################################################
#########################################################################
# This script produces usable sum-encrypted messages, but there are still some
# additional needed improvements. For example,
# 1. tokenization, e.g. handle punctuation (note convert "read." to "read" but not "U.S." to "U.S")
# 2. lemmatization, e.g. convert plural nouns to singular (WordNet has no plurals)
# 3. hash words not in the dictionary to unused unique integers 
########################################################################################

import csv
import sys
import shlex
import time

#########################################

def uniq(list):
    set = {}
    return [set.setdefault(x,x) for x in list if x not in set]

##########################
def findlist(entryword, syndict, stopwords):
    newlist = []
    if (syndict.has_key(entryword) and entryword not in stopwords):
        return syndict[entryword]
    else:
        return newlist

####################################
def SumWordsDFSRecursive(currwords, index, syndict, stopwords, subtotal, sumlist):
    if index == len(currwords):
        sumlist.append(subtotal)
        return

    word = currwords[index]
    currints = findlist(word.lower(), syndict, stopwords)
    for entry in currints:
        SumWordsDFSRecursive(currwords, index+1, syndict, stopwords, subtotal+int(entry), sumlist)

#####################################
def SumNEncrypt(userwords, currwords, syndict, stopwords, N, sumlist):
    if len(currwords) == N:
        SumWordsDFSRecursive(currwords, 0, syndict, stopwords, 0, sumlist)
        sumlist = uniq(sumlist)       # reduce duplicate entries as we go to save memory
        return

    for i in range(0, len(userwords)):
        newwords = currwords[:]
        newwords.append(userwords[i])
        SumNEncrypt(userwords[i+1:], newwords, syndict, stopwords, N, sumlist)

##########################################
stopwords = ['a', 'about', 'above', 'after', 'again', 'against', 'all', 'am', 'an', 'and', 'any', 'are', 'aren\'t', 'as', 'at', 'be', 'because', 'been', 'before', 'being', 'below', 'between', 'both', 'but', 'by', 'can\'t', 'cannot', 'could', 'couldn\'t', 'did', 'didn\'t', 'do', 'does', 'doesn\'t', 'doing', 'don\'t', 'down', 'during', 'each', 'few', 'for', 'from', 'further', 'had', 'hadn\'t', 'has', 'hasn\'t', 'have', 'haven\'t', 'having', 'he', 'he\'d', 'he\'ll', 'he\'s', 'her', 'here', 'here\'s', 'hers', 'herself', 'him', 'himself', 'his', 'how', 'how\'s', 'i', 'i\'d', 'i\'ll', 'i\'m', 'i\'ve', 'if', 'in', 'into', 'is', 'isn\'t', 'it', 'it\'s', 'its', 'itself', 'let\'s', 'me', 'more', 'most', 'mustn\'t', 'my', 'myself', 'no', 'nor', 'not', 'of', 'off', 'on', 'once', 'only', 'or', 'other', 'ought', 'our', 'ours', 'ourselves', 'out', 'over', 'own', 'same', 'shan\'t', 'she', 'she\'d', 'she\'ll', 'she\'s', 'should', 'shouldn\'t', 'so', 'some', 'such', 'than', 'that', 'that\'s', 'the', 'their', 'theirs', 'them', 'themselves', 'then', 'there', 'there\'s', 'these', 'they', 'they\'d', 'they\'ll', 'they\'re', 'they\'ve', 'this', 'those', 'through', 'to', 'too', 'under', 'until', 'up', 'very', 'was', 'wasn\'t', 'we', 'we\'d', 'we\'ll', 'we\'re', 'we\'ve', 'were', 'weren\'t', 'what', 'what\'s', 'when', 'when\'s', 'where', 'where\'s', 'which', 'while', 'who', 'who\'s', 'whom', 'why', 'why\'s', 'with', 'won\'t', 'would', 'wouldn\'t', 'you', 'you\'d', 'you\'ll', 'you\'re', 'you\'ve', 'your', 'yours', 'yourself', 'yourselves']


N = 4   # change this for the desired sN encryption

synfile = "omega-hybrid.txt"    

saveplace = "diy_keys" + time.strftime("%Y_%m_%d_%H_%M_%S_%P") + ".kyx"
outfile = open(saveplace, "w")

synReader = csv.reader(open(synfile,'rb'), delimiter=' ')

syns = []
for synline in synReader:
    syns.append(synline)

syndict = {}
for test in syns:
    if syndict.has_key(test[0]) == False:
        templist = []
        for j in range (1, len(test)):
            if (test[j] != ''):
                templist.append(test[j])
        syndict[test[0]] = templist
            
usertext = raw_input('Your text (hit Enter to quit): ')

if usertext == '':
    sys.exit()                ## quit the program

userwords = shlex.split(usertext)

currwords = []
sumlist = []
SumNEncrypt(userwords, currwords, syndict, stopwords, N, sumlist)

 

for key in sorted(uniq(sumlist)):              # write output with one sum per line
    outfile.write(str(key) + '\n')
