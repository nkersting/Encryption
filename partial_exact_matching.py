
import random
from collections import defaultdict

glove_file = "glove.6B.50d.txt"
num_docs = 10000
limit = 1000000

lexicon = {}
with open(glove_file,'r') as f:
    for line in f.readlines():
        fields = line.split()
        lexicon[fields[0]] = (random.randint(-limit,limit),random.randint(-limit,limit) )


print("Building documents ...")
docs = []
for i in range(num_docs):
    docs.append(random.sample(lexicon.keys(), 4))  # for simplicity, only length-4 docs

triples = [(0,1,2), (0,1,3), (0,2,3), (1,2,3)]  # hard-coded for now
    
print("Building doc map ...")
document_map = defaultdict(list)
for d in docs:
    for t in triples:
        triple_vecs = zip(lexicon[d[t[0]]],lexicon[d[t[1]]], lexicon[d[t[2]]])
        document_map[' '.join(d)].append(tuple(map(sum, triple_vecs)))


    
print("First 10 docs:")
for i in range(10):
    print(docs[i], document_map[' '.join(docs[i])])

print("building inverse map:")
vec_lookup = defaultdict(set)
for d in docs:
    key = ' '.join(d)
    for v in document_map[key]:
        vec_lookup[v].add(key)

# PARTIAL EXACT MATCH
query_words = docs[0][:-1]
query_words.append('apple')  # replace last word of first document as query
query = ' '.join(query_words)

query_vecs = []
for t in triples:
    triple_vecs = zip(lexicon[query_words[t[0]]],
                      lexicon[query_words[t[1]]],
                      lexicon[query_words[t[2]]])
    query_vecs.append(tuple(map(sum, triple_vecs)))

print(f"All partial exact matches for query = \"{query}\": {[vec_lookup[x] for x in query_vecs]}")



    
