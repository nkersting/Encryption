
import random
from collections import defaultdict
from sN_vec_hash import SumNVecHasher
import numpy as np


def cosine(u,v):
    norm_u = np.linalg.norm(u)
    norm_v = np.linalg.norm(v)
    if norm_u == 0 or norm_v == 0:
        return 0
    else:
        return np.dot(u,v)/norm_u/norm_v


glove_file = "glove.6B.50d.txt"
num_docs = 10000


lexicon = {}
with open(glove_file,'r') as f:
    for line in f.readlines():
        fields = line.split()
        lexicon[fields[0]] = [float(x) for x in fields[1:]]


# 3-Sum hashing for this expt
hasher = SumNVecHasher(3, lexicon)


print("Building documents ...")
docs = []
for i in range(num_docs):
    docs.append(random.sample(lexicon.keys(), random.randint(3,6)))

    
print("Building doc map ...")
document_map = defaultdict(list)
for d in docs:
    document_map[' '.join(d)] = hasher.getNHash(d)


    
print("First 10 docs:")
for i in range(10):
    print(docs[i])

print("building inverse map:")
vec_lookup = defaultdict(set)
for d in docs:
    key = ' '.join(d)
    for v in document_map[key]:
        vec_lookup[tuple(v)].add(key)

# PARTIAL SEMANTIC MATCH
query_words = docs[0][:-1]
query_words.append('apple')  # replace last word of first document as query
query = ' '.join(query_words)
query_vecs = hasher.getNHash(query_words)


print(f"Best semantic matches for query = \"{query}\:")
for v in query_vecs:
    top_sim = 0
    top_key = ""
    for target in vec_lookup.keys():
        curr_cos = cosine(v,target)
        if curr_cos > top_sim:
            top_sim = curr_cos
            top_key = vec_lookup[target]

    print(top_key)

    



    
