#!/usr/bin/env python3
import os
import json

GRAPH_FILE = '../graph.json'
UPLOAD_FOLDER = '../uploads'
SIMILARITY_THRESHOLD = 0.01

def recompute_network():
    files = os.listdir(UPLOAD_FOLDER)
    graph = {'nodes': [], 'links': []}

    for file in files:
        filepath = os.path.join(UPLOAD_FOLDER, file)
        file_set = set(map(int, open(filepath).read().splitlines()))
        graph['nodes'].append({'id': file, 'set': list(file_set)})

    for i, node_a in enumerate(graph['nodes']):
        for j, node_b in enumerate(graph['nodes']):
            if i >= j:
                continue
            similarity = compute_similarity_score(set(node_a['set']), set(node_b['set']))
            if similarity > SIMILARITY_THRESHOLD:
                graph['links'].append({
                    'source': node_a['id'],
                    'target': node_b['id'],
                    'similarity': similarity
                })

    with open(GRAPH_FILE, 'w') as f:
        json.dump(graph, f)

def compute_similarity_score(set_a, set_b):
    intersection = len(set_a & set_b)
    union = len(set_a | set_b)
    return intersection / union


print("Content-Type: application/json")
print()

try:
    recompute_network()
    print(json.dumps({'success': True}))
except Exception as e:
    print(json.dumps({'success': False, 'error': str(e)}))
