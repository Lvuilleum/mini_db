import socket
from faker import Faker
from sentence_transformers import SentenceTransformer
import time

# Config
MODEL_NAME = 'all-MiniLM-L6-v2'
SERVER_IP = '127.0.0.1'
SERVER_PORT = 8080
NUM_SAMPLES = 10000

fake = Faker()
model = SentenceTransformer(MODEL_NAME)

def run_stress_test():
    print(f"🚀 Démarrage du stress test : {NUM_SAMPLES} entrées...")
    
    start_time = time.time()

    for i in range(1, NUM_SAMPLES + 1):
        # 1. Générer une phrase aléatoire
        text = fake.sentence(nb_words=10)
        
        # 2. Encoder
        vector = model.encode(text)
        vector_str = " ".join(map(str, vector))
        
        # 3. Envoyer (On ouvre/ferme la socket à chaque fois pour simuler 10k clients)
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect((SERVER_IP, SERVER_PORT))
                command = f"insert {i} {vector_str}\n"
                s.sendall(command.encode())
                
                # Lire la réponse avant de fermer la socket
                response = b""
                while True:
                    chunk = s.recv(1024)
                    if not chunk:
                        break
                    response += chunk
                    if b"<END>" in response:
                        break
        except Exception as e:
            print(f"Erreur à l'ID {i}: {e}")
            break

        if i % 100 == 0:
            print(f"✅ {i}/{NUM_SAMPLES} insérés...")

    end_time = time.time()
    print(f"--- Terminé en {end_time - start_time:.2f} secondes ---")

if __name__ == "__main__":
    run_stress_test()