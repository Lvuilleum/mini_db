import socket 
from sentence_transformers import SentenceTransformer 

MODEL_NAME = 'all-MiniLM-L6-v2' 
SERVER_IP = '127.0.0.1'
SERVER_PORT = 8080

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
        
        print(full_response.replace("<END>", ""))


def insert_text(id, text):
    # Transformation de la phrase en vecteur
    vector = model.encode(text)
    # Transformation du vecteur en chaîne de caractères pour ton serveur C
    vector_str = " ".join(map(str, vector))
    command = f"insert {id} {vector_str}"
    send_to_db(command)

def search_text(text):
    vector = model.encode(text)
    vector_str = " ".join(map(str, vector))
    command = f"search {vector_str}"
    send_to_db(command)

# --- TEST ---
# 1. On insère des connaissances
insert_text(1, "Le ciel est bleu et le soleil brille")
insert_text(2, "La recette des crêpes demande du lait et de la farine")

# 2. On fait une recherche sémantique
# Note : on ne cherche pas les mêmes mots, mais le même SENS
print("\nRecherche pour : 'Comment faire de la cuisine ?'")
search_text("Comment faire de la cuisine ?")