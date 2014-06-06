#!/usr/bin/env python


# N-Sum encryption for any N ( = 2 by default)
# Also available at https://github.com/nkersting/Encryption
#
#  BEFORE RUNNING: make sure this script and "omegaO2.txt" are in the same directory.
#  USAGE: Simply run this script and, when prompted, enter the message you wish to encrypt.
#  OUTPUT: Sorted list of encryption keys written to "diy_keys+<timestamp>.kyx". Feel free to rename this file to your tastes, but please
#          keep the *.kyx extension.
#  UPLOAD: You are encouraged to upload the output file to the Quantum Repoire Server (http://www.quantumrepoire.com)
#
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
import time

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
    return
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
    return
###################################################################
def main():

    N = 2   # change this for the desired security level sN

    # append words to this list to exclude from encryption, such as "a", "the", "it", etc.
    stopwords = ['a', 'about', 'above', 'after', 'again', 'against', 'all', 'am', 'an', 'and', 'any', 'are', 'aren\'t', 'as', 'at', 'be', 'because', 'been', 'before', 'being', 'below', 'between', 'both', 'but', 'by', 'can\'t', 'cannot', 'could', 'couldn\'t', 'did', 'didn\'t', 'do', 'does', 'doesn\'t', 'doing', 'don\'t', 'down', 'during', 'each', 'few', 'for', 'from', 'further', 'had', 'hadn\'t', 'has', 'hasn\'t', 'have', 'haven\'t', 'having', 'he', 'he\'d', 'he\'ll', 'he\'s', 'her', 'here', 'here\'s', 'hers', 'herself', 'him', 'himself', 'his', 'how', 'how\'s', 'i', 'i\'d', 'i\'ll', 'i\'m', 'i\'ve', 'if', 'in', 'into', 'is', 'isn\'t', 'it', 'it\'s', 'its', 'itself', 'let\'s', 'me', 'more', 'most', 'mustn\'t', 'my', 'myself', 'no', 'nor', 'not', 'of', 'off', 'on', 'once', 'only', 'or', 'other', 'ought', 'our', 'ours', 'ourselves', 'out', 'over', 'own', 'same', 'shan\'t', 'she', 'she\'d', 'she\'ll', 'she\'s', 'should', 'shouldn\'t', 'so', 'some', 'such', 'than', 'that', 'that\'s', 'the', 'their', 'theirs', 'them', 'themselves', 'then', 'there', 'there\'s', 'these', 'they', 'they\'d', 'they\'ll', 'they\'re', 'they\'ve', 'this', 'those', 'through', 'to', 'too', 'under', 'until', 'up', 'very', 'was', 'wasn\'t', 'we', 'we\'d', 'we\'ll', 'we\'re', 'we\'ve', 'were', 'weren\'t', 'what', 'what\'s', 'when', 'when\'s', 'where', 'where\'s', 'which', 'while', 'who', 'who\'s', 'whom', 'why', 'why\'s', 'with', 'won\'t', 'would', 'wouldn\'t', 'you', 'you\'d', 'you\'ll', 'you\'re', 'you\'ve', 'your', 'yours', 'yourself', 'yourselves']


    synfile = "omegaO2.txt"    # map file defines M: x -> Omega_x
    outfile = open("diy_keys" + time.strftime("%Y_%m_%d_%H_%M_%S_%P") + ".kyx", "w")   # output as a text file, one key per line

    synReader = csv.reader(open(synfile,'rb'), delimiter=' ')   # read map file
    syns = []
    for synline in synReader:
        syns.append(synline)

    syndict = {}        # convert to a dictionary for faster access
    for entryline in syns:
        syndict[entryline[0]] = [int(x) for x in entryline[1:]]

    usertext = raw_input('Message to encrypt: ')    # user input
    userwords = shlex.split(usertext)

    sumdict = {}                                # keys are the n-sums, values are lists of words contributing to that n-sum (for assigning a matching score to each word in the message)
    SumNEncrypt(userwords, [], syndict, stopwords, N, sumdict)  # encryption done here

    for key in sorted(sumdict.keys()):              # write output with one sum per line
        outfile.write(str(key) + '\n')
 
if __name__ == '__main__':
    main()
  
