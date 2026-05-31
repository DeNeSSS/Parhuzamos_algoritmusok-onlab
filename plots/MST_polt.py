import os
import glob
import pandas as pd
import matplotlib.pyplot as plt

def create_plot(csv_files, output_filename, is_zoomed=False, top_x=None, names=None, plot_name=None, show=False):
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 7))
    
    # 1. LÉPÉS: Az ÖSSZES CSV fájl beolvasása és összefűzése egy nagy táblázatba
    all_dfs = []
    for file in csv_files:
        filename = os.path.basename(file)
        # Kinyerjük az algoritmus nevét a fájlnévből (pl. "Parallel_Prim_results.csv" -> "Parallel Prim")
        alg_name = filename.replace("_results.csv", "").replace("_", " ")
        
        if names is not None and alg_name not in names:
            continue
            
        try:
            df = pd.read_csv(file)
            df['Algorithm'] = alg_name  # Elmentjük, melyik algoritmushoz tartozik ez a mérési sor
            all_dfs.append(df)
        except Exception as e:
            print(f"Hiba a fájl beolvasásakor ({filename}): {e}")

    if not all_dfs:
        print("Nem sikerült feldolgozható adatot beolvasni.")
        return

    # Egyesítjük az összes adatot egyetlen közös táblázatba
    main_df = pd.concat(all_dfs, ignore_index=True)

    # 2. LÉPÉS: Globális kiértékelés és átlagolás algoritmusonként
    alg_data_list = []
    
    # Külön-kurzusban nézzük meg az egyes algoritmusokat a nagy közös táblában
    for alg_name in main_df['Algorithm'].unique():
        df_alg = main_df[main_df['Algorithm'] == alg_name]
        
        # Timeout label meghatározása az eredeti adatokból
        timeout_rows = df_alg[df_alg['Status'].str.contains('TIMEOUT|SKIPPED', na=False)]
        if not timeout_rows.empty:
            timeout_edges = timeout_rows['Edges'].iloc[0]
            if timeout_edges >= 1_000_000:
                alg_label = f"{alg_name} (Leállt {timeout_edges/1_000_000:g}M élnél)"
            elif timeout_edges >= 1_000:
                alg_label = f"{alg_name} (Leállt {timeout_edges/1_000:g}k élnél)"
            else:
                alg_label = f"{alg_name} (Leállt {timeout_edges} élnél)"
        else:
            alg_label = alg_name

        # Csak a sikeres futásokat átlagoljuk a grafikonhoz
        df_success = df_alg[df_alg['Status'] == 'SUCCESS'].copy()
        df_success = df_success.dropna(subset=['Wall_Avg_s', 'CPU_Work_s'])
        
        if not df_success.empty:
            # --- ITT TÖRTÉNIK A VALÓDI ÁTLAGOLÁS ---
            # Mivel a main_df tartalmazza a 11, 12, 13, 14-es teszteket is egyszerre, 
            # a groupby most már TÉNYLEG megtalálja őket, és egyetlen sorrá gyúrja össze!
            df_grouped = df_success.groupby(['Vertices', 'Edges'], as_index=False)[['Wall_Avg_s', 'CPU_Work_s']].mean()
            
            # Szigorú rendezés az élek száma szerint, hogy a vonal egyenesen haladjon előre
            df_grouped = df_grouped.sort_values(by='Edges').reset_index(drop=True)
            
            max_edges_reached = df_grouped['Edges'].max()
            time_at_max_edges = df_grouped[df_grouped['Edges'] == max_edges_reached]['Wall_Avg_s'].iloc[0]
            
            alg_data_list.append({
                'label': alg_label,
                'df': df_grouped,
                'max_edges': max_edges_reached,
                'time_at_max': time_at_max_edges
            })

    # 3. LÉPÉS: Algoritmusok rangsorolása (A legtovább jutott, azon belül a leggyorsabb van elöl)
    alg_data_list.sort(key=lambda x: (x['max_edges'], -x['time_at_max']), reverse=True)

    # Szűrés a top_x paraméter alapján (ha meg van adva)
    if top_x is not None and top_x > 0:
        alg_data_list = alg_data_list[:top_x]
        
    all_wall_values = []
    all_cpu_values = []

    # 4. LÉPÉS: Kirajzolás (Most már garantáltan 1 pont / tesztméret!)
    for alg in alg_data_list:
        df_grouped = alg['df']
        alg_label = alg['label']
        
        x_data = df_grouped['Edges']
        wall_data = df_grouped['Wall_Avg_s']
        cpu_data = df_grouped['CPU_Work_s']
        
        all_wall_values.extend(wall_data.tolist())
        all_cpu_values.extend(cpu_data.tolist())
        
        ax1.plot(x_data, wall_data, marker='o', linewidth=2, label=alg_label)
        ax2.plot(x_data, cpu_data, marker='s', linestyle='--', linewidth=2, label=alg_label)

    # Mindkét grafikon alja fixen 0
    ax1.set_ylim(bottom=0)
    ax2.set_ylim(bottom=0)

    # --- DINAMIKUS ZOOM (Levágjuk az extrém kiugrókat) ---
    if is_zoomed and all_wall_values:
        zoom_wall_limit = pd.Series(all_wall_values).quantile(0.80) * 1.3
        zoom_cpu_limit = pd.Series(all_cpu_values).quantile(0.80) * 1.3
        ax1.set_ylim(0, zoom_wall_limit)
        ax2.set_ylim(0, zoom_cpu_limit)

    # --- Bal oldali ábra (Wall Time) formázása ---
    ax1.set_xscale('log')
    ax1.set_title("Futásidő", fontsize=14)
    ax1.set_xlabel("Élek száma (Logaritmikus skála)", fontsize=12)
    ax1.set_ylabel("Átlagos idő (másodperc)", fontsize=12)
    ax1.grid(True, linestyle=':', alpha=0.7)
    ax1.legend(loc="upper left", fontsize=10)

    # --- Jobb oldali ábra (CPU Work) formázása ---
    ax2.set_xscale('log')
    ax2.set_title("Összesített processzor munka", fontsize=14)
    ax2.set_xlabel("Élek száma (Logaritmikus skála)", fontsize=12)
    ax2.set_ylabel("CPU idő (másodperc)", fontsize=12)
    ax2.grid(True, linestyle=':', alpha=0.7)
    ax2.legend(loc="upper left", fontsize=10)

    if plot_name is None:
        plt.suptitle("MST Algoritmusok Teljesítménye", fontsize=18, fontweight='bold')
    else:
        plt.suptitle(plot_name, fontsize=18, fontweight='bold')
        
    plt.tight_layout()
    plt.savefig(output_filename, dpi=300)
    print(f"Ábra mentve: {output_filename}")
    
    if show:
        # plt.show()
        plt.close()

    else:
        plt.close()


def main():
    dir_name = "sparse" 
    
    data_dir = f"test_data/mst/{dir_name}/"
    if not os.path.exists(data_dir):
        data_dir = f"../test_data/mst/{dir_name}/"
        
    csv_files = glob.glob(os.path.join(data_dir, "*_results.csv"))
    
    if not csv_files:
        print(f"Hiba: Nem találtam CSV fájlokat a '{data_dir}' mappában.")
        return

    print(f"[INFO] {len(csv_files)} CSV fájl globális összefűzése és elemzése...")

    # 1. Teljes grafikon legenerálása
    full_output = os.path.join(data_dir, f"mst_benchmark_plot_full_log_{dir_name}.png")
    create_plot(csv_files, full_output, plot_name="MST Algoritmusok", show=True)

    # 2. Zoomolt grafikon legenerálása
    zoomed_output = os.path.join(data_dir, f"mst_benchmark_plot_zoomed_log_{dir_name}.png")
    create_plot(csv_files, zoomed_output, is_zoomed=True, plot_name="MST Algoritmusok", show=True)

    # 3. Csak a legjobb 4 algoritmus megjelenítése
    top_output = os.path.join(data_dir, f"mst_benchmark_plot_top4_log_{dir_name}.png")
    create_plot(csv_files, top_output, top_x=4, plot_name="MST Algoritmusok - Top 4 Leggyorsabb", show=True)

if __name__ == "__main__":
    main()