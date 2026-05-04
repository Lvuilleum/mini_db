import socket
import time
from sentence_transformers import SentenceTransformer

model = SentenceTransformer('all-MiniLM-L6-v2')
query = "Tell me something about nature and the sky"
vector = model.encode(query)
vector_str = " ".join(map(str, vector))

def benchmark():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect(('127.0.0.1', 8080))
        # On mesure uniquement le temps de réponse du serveur
        start = time.time()
        s.sendall(f"search {vector_str}\n".encode())
        
        response = ""
        while "<END>" not in response:
            response += s.recv(4096).decode()
        
        end = time.time()
        print(response.replace("<END>", ""))
        print(f"⏱️ Round-trip time (Network + AI + C): {(end - start) * 1000:.2f} ms")

benchmark()