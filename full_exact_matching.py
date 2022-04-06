
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
    docs.append(random.sample(lexicon.keys(), random.randint(2,10)))

print("Building doc map ...")
document_map = {}
for d in docs:
    document_map[' '.join(d)] = (sum([lexicon[w][0] for w in d]),
                                 sum([lexicon[w][1] for w in d]))
    
print("First 10 docs:")
for i in range(10):
    print(docs[i], document_map[' '.join(docs[i])])

print("building inverse map:")
vec_lookup = defaultdict(list)
for d in docs:
    key = ' '.join(d)
    vec_lookup[document_map[key]].append(key)

num_collisions = 0
for key in vec_lookup:
    num_collisions += len(vec_lookup[key]) - 1
print("Number of collisions: ", num_collisions)


# FULL EXACT MATCH
query = ' '.join(docs[0])
print(f"All full exact matches for query = \"{query}\": {vec_lookup[document_map[query]]}")


"""

Limit  Num_Collisions
10     6500
100     201
10^6      0

"""


    
