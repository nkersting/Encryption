#!/usr/bin/env python3

import cgi
import os
import cgitb
import json

cgitb.enable()  # Enable debugging

UPLOAD_FOLDER = '../uploads'
SIMILARITY_THRESHOLD = 0.01
GRAPH_FILE = '../graph.json'


# Ensure upload folder exists
if not os.path.exists(UPLOAD_FOLDER):
    os.makedirs(UPLOAD_FOLDER)

def save_uploaded_file(fileitem):
    MAX_FILE_SIZE = 1 * 1024 * 1024  # 1MB size limit

    if fileitem.filename:
        # Strip leading path from file name to avoid directory traversal attacks
        filename = os.path.basename(fileitem.filename)
        filepath = os.path.join(UPLOAD_FOLDER, filename)

        # Check if the file already exists
        if os.path.exists(filepath):
            return False, filename, "File already exists."

        # Check file size
        fileitem.file.seek(0, os.SEEK_END)
        file_size = fileitem.file.tell()
        fileitem.file.seek(0, os.SEEK_SET)

        if file_size > MAX_FILE_SIZE:
            return False, filename, "File exceeds size limit of 1MB. Please choose smaller file."

        # Save the file
        with open(filepath, 'wb') as f:
            f.write(fileitem.file.read())
        return True, filename, ""
    else:
        return False, None, "No file selected."

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


print("Content-Type: text/html")
print()

form = cgi.FieldStorage()
fileitem = form['userfile']

success, filename, status = save_uploaded_file(fileitem)

if success:
    compute_similarity(filename)
    print(f"<html><body><h2>File '{filename}' uploaded successfully.</h2>")
    print("<p><a href='../index.html'>Back</a></p></body></html>")
else:
    print(f"<html><body><h2>File upload failed. {status}</h2>")
    print("<p><a href='../index.html'>Back</a></p></body></html>")
