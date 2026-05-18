import main_ai
import socket
import json
import os
import re
from pathlib import Path
from sentence_transformers import SentenceTransformer


MODEL_NAME = 'all-MiniLM-L6-v2'
SERVER_IP = '127.0.0.1'
SERVER_PORT = 8080

print("Chargement du modèle d'IA...")
model = SentenceTransformer(MODEL_NAME)


print("\nRecherche pour : 'what is our government ?'")
main_ai.search_text("the government.")