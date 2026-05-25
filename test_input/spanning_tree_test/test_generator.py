import random

def generate_mst_test_fast(filename, V, E, max_weight=1000000000):
    if E < V - 1:
        return print("Hiba: E legalább V-1 kell legyen!")

    # Csak a csúcspárok tárolása (u, v) formában, ahol u < v
    # A set (halmaz) keresési és beszúrási ideje O(1)
    pair_set = set()
    
    # 1. Feszítőfa generálása az összefüggőséghez
    for i in range(1, V):
        u = i
        v = random.randint(0, i - 1)
        pair_set.add((v, u)) # Mindig a kisebb index kerül előre

    print("Összefüggőség: OK")

    # 2. Maradék élek hozzáadása
    # A random.sample vagy a randint + halmazba dobás már nagyon gyors
    while len(pair_set) < E:
        if len(pair_set) % (E // 10) == 0:
            print(f"Kész: {len(pair_set)}") 

        u = random.randint(0, V - 1)
        v = random.randint(0, V - 1)
        
        if u == v:
            continue
            
        # Rendezés, hogy (1, 2) és (2, 1) ugyanaz legyen
        if u > v: u, v = v, u
        
        # A set.add() csak akkor adja hozzá, ha még nincs benne.
        # Nincs szükség manuális any() ellenőrzésre!
        pair_set.add((u, v))

    # 3. Súlyok hozzárendelése és mentés
    # A fájlformátumhoz 1-et adunk az indexekhez
    with open(filename, 'w') as f:
        f.write(f"{V} {E}\n")
        for u, v in pair_set:
            w = random.randint(1, max_weight)
            f.write(f"{u + 1} {v + 1} {w}\n")

    print(f"Generálva: {filename}")

generate_mst_test_fast("dense_1.in", 1000, 750_000, max_weight=1_000_000_000)