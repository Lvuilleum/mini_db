import socket
import json
import os
import re
from pathlib import Path
from sentence_transformers import SentenceTransformer

MODEL_NAME = 'all-MiniLM-L6-v2'
SERVER_IP = '127.0.0.1'
SERVER_PORT = 8080
METADATA_FILE = 'scripts/metadata.json' # Fichier où on va stocker les phrases
TEXT_FILE = Path(__file__).resolve().parent.parent / 'book.txt'

print("Chargement du modèle d'IA...")
model = SentenceTransformer(MODEL_NAME)

def send_to_db(command):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((SERVER_IP, SERVER_PORT))
        s.sendall(command.encode() + b'\n')
        
        full_response = ""
        while True:
            chunk = s.recv(4096).decode()
            if not chunk:
                break
            full_response += chunk
            if "<END>" in full_response: 
                break
        
        return full_response.replace("<END>", "")


def insert_text(doc_id, text):
    # Transformation de la phrase en vecteur
    vector = model.encode(text)
    vector_str = " ".join(map(str, vector))
    command = f"insert {doc_id} {vector_str}"
    
    send_to_db(command)
    
    metadata = {}
    if os.path.exists(METADATA_FILE):
        with open(METADATA_FILE, 'r') as f:
            metadata = json.load(f)
            
    metadata[str(doc_id)] = text
    
    with open(METADATA_FILE, 'w') as f:
        json.dump(metadata, f, indent=4)
        
    print(f"Inséré -> ID {doc_id} : '{text}'")


def search_text(text):
    vector = model.encode(text)
    vector_str = " ".join(map(str, vector))
    command = f"search {vector_str}"
    
    response = send_to_db(command)
    print(response) 
    
    metadata = {}
    if os.path.exists(METADATA_FILE):
        with open(METADATA_FILE, 'r') as f:
            metadata = json.load(f)
            
    print("--- Traduction des résultats ---")
    for line in response.split('\n'):
        if "ID " in line:
            try:
                parts = line.split("ID ")
                found_id = parts[1].split(" ")[0].strip()
                
                phrase = metadata.get(found_id, "Texte généré par le stress test (non indexé)")
                print(f"-> {line} | Phrase: {phrase}")
            except:
                continue


def load_phrases_from_txt(file_path):
    with open(file_path, 'r', encoding='utf-8') as file:
        content = file.read()

    phrases = []
    for block in content.splitlines():
        block = block.strip()
        if not block:
            continue

        parts = re.split(r'(?<=[.!?])\s+', block)
        for phrase in parts:
            phrase = phrase.strip()
            if phrase:
                phrases.append(phrase)

    return phrases


if __name__ == "__main__":
    if not TEXT_FILE.exists():
        raise FileNotFoundError(f"Fichier introuvable: {TEXT_FILE}")

    for idx, phrase in enumerate(load_phrases_from_txt(TEXT_FILE), start=1):
        if idx > 5000:
            break
        insert_text(idx, phrase)

    print("\nRecherche pour : 'Comment faire de la cuisine ?'")
    search_text("Comment faire de la cuisine ?")