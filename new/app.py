from flask import Flask, request, jsonify, render_template
import os
import json

UPLOAD_FOLDER = './uploads'
GRAPH_FILE = './graph.json'
app = Flask(__name__)
app.config['UPLOAD_FOLDER'] = UPLOAD_FOLDER

# Ensure upload folder exists
if not os.path.exists(UPLOAD_FOLDER):
    os.makedirs(UPLOAD_FOLDER)

# Ensure graph file exists
if not os.path.exists(GRAPH_FILE):
    with open(GRAPH_FILE, 'w') as f:
        json.dump({'nodes': [], 'links': []}, f)

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
    compute_similarity(file.filename)  # Recompute the network for the new file
    return 'File uploaded and network updated successfully.', 200

@app.route('/api/files')
def api_files():
    files = os.listdir(UPLOAD_FOLDER)
    return jsonify({'files': files})

@app.route('/api/graph')
def api_graph():
    with open(GRAPH_FILE, 'r') as f:
        graph = json.load(f)
    return jsonify(graph)

@app.route('/api/recompute', methods=['POST'])
def api_recompute():
    try:
        recompute_network()
        return jsonify({'success': True})
    except Exception as e:
        return jsonify({'success': False, 'error': str(e)}), 500

SIMILARITY_THRESHOLD = 0.01

def compute_similarity(new_file):
    new_filepath = os.path.join(UPLOAD_FOLDER, new_file)
    new_set = set(map(int, open(new_filepath).read().splitlines()))

    # Ensure graph file exists
    if not os.path.exists(GRAPH_FILE):
        with open(GRAPH_FILE, 'w') as f:
            json.dump({'nodes': [], 'links': []}, f)

    with open(GRAPH_FILE, 'r') as f:
        graph = json.load(f)

    new_node = {'id': new_file, 'set': list(new_set)}
    graph['nodes'].append(new_node)

    for node in graph['nodes']:
        if node['id'] == new_file:
            continue
        existing_set = set(node['set'])
        sim = len(new_set.intersection(existing_set)) / len(new_set.union(existing_set))
        if sim >= SIMILARITY_THRESHOLD:  # Only significant links
            graph['links'].append({'source': new_file, 'target': node['id'], 'similarity': sim})

    with open(GRAPH_FILE, 'w') as f:
        json.dump(graph, f)

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

if __name__ == '__main__':
    app.run(debug=True)
