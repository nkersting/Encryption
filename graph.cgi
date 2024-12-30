#!/usr/bin/env python3
import json

GRAPH_FILE = '../graph.json'

print("Content-Type: application/json")
print()

with open(GRAPH_FILE, 'r') as f:
    graph = json.load(f)
print(json.dumps(graph))
