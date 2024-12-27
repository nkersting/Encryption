from flask import Flask, request, jsonify, render_template
import os
import json
import numpy as np
from scipy.spatial.distance import pdist, squareform
from sklearn.metrics import jaccard_score

UPLOAD_FOLDER = './uploads'
app = Flask(__name__)
app.config['UPLOAD_FOLDER'] = UPLOAD_FOLDER

# Ensure upload folder exists
if not os.path.exists(UPLOAD_FOLDER):
    os.makedirs(UPLOAD_FOLDER)

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/files')
def files():
    return render_template('files.html')

@app.route('/results')
def results():
    return render_template('results.html')

@app.route('/upload', methods=['POST'])
def upload():
    if 'userfile' not in request.files:
        return 'No file uploaded', 400

    file = request.files['userfile']
    if file.filename == '':
        return 'No file selected', 400

    filepath = os.path.join(app.config['UPLOAD_FOLDER'], file.filename)
    file.save(filepath)
    compute_similarity()  # Recompute the network
    return 'File uploaded and network updated successfully.', 200

@app.route('/api/files')
def api_files():
    files = os.listdir(UPLOAD_FOLDER)
    return jsonify({'files': files})

@app.route('/api/graph')
def api_graph():
    with open('./graph.json', 'r') as f:
        graph = json.load(f)
    return jsonify(graph)

def compute_similarity():
    files = [os.path.join(UPLOAD_FOLDER, f) for f in os.listdir(UPLOAD_FOLDER)]
    sets = [set(map(int, open(f).read().splitlines())) for f in files]

    # Compute Jaccard similarity
    n = len(sets)
    nodes = [{'id': os.path.basename(f)} for f in files]
    links = []
    for i in range(n):
        for j in range(i + 1, n):
            sim = len(sets[i].intersection(sets[j])) / len(sets[i].union(sets[j]))
            if sim > 0:  # Only significant links
                links.append({'source': nodes[i]['id'], 'target': nodes[j]['id'], 'similarity': sim})

    graph = {'nodes': nodes, 'links': links}
    with open('./graph.json', 'w') as f:
        json.dump(graph, f)

if __name__ == '__main__':
    app.run(debug=True)
