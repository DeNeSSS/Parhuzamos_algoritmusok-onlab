import os
import glob
import pandas as pd
import matplotlib.pyplot as plt

def create_plot(csv_files, output_filename, is_zoomed=False, top_x=None, names=None, plot_name=None, show=False):
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 7))
    
    # 1. LÉPÉS: Az összes CSV beolvasása és összefűzése
    all_dfs = []
    for file in csv_files:
        filename = os.path.basename(file)
        alg_name = filename.replace("_results.csv", "").replace("_", " ")
        if names is not None and alg_name not in names:
            continue
        try:
            df = pd.read_csv(file)
            df['Algorithm'] = alg_name
            all_dfs.append(df)
        except Exception as e:
            print(f"Hiba a fájl beolvasásakor ({filename}): {e}")

    if not all_dfs:
        print("Nem sikerült feldolgozható adatot beolvasni.")
        return

    main_df = pd.concat(all_dfs, ignore_index=True)
    alg_data_list = []
    
    # 2. LÉPÉS: Csoportosítás és átlagolás ALGORITMUS és PARAMÉTEREK szerint
    for alg_name in main_df['Algorithm'].unique():
        df_alg = main_df[main_df['Algorithm'] == alg_name]
        
        # Csak a sikereseket jelenítjük meg
        df_success = df_alg[df_alg['Status'] == 'SUCCESS'].copy()
        df_success = df_success.dropna(subset=['Wall_Avg_s', 'CPU_Work_s'])
        
        if not df_success.empty:
            # ITT TÖRTÉNIK AZ ÁTLAGOLÁS: a (1000, 10000) paraméterű 11,12,13,14 tesztek 1 sorrá válnak!
            df_grouped = df_success.groupby(['Vertices', 'Edges'], as_index=False)[['Wall_Avg_s', 'CPU_Work_s']].mean()
            
            # Sorbarendezzük őket az élek, majd a csúcsok száma szerint, hogy szép progresszív legyen a vonal
            df_grouped = df_grouped.sort_values(by=['Edges', 'Vertices']).reset_index(drop=True)
            
            # LÉTREHOZUNK EGY EGYEDI X-TENGELY CÍMKÉT (Pl: "V: 1k\nE: 10k")
            def format_label(row):
                v = f"{row['Vertices']/1000:g}k" if row['Vertices'] >= 1000 else str(int(row['Vertices']))
                e = f"{row['Edges']/1000:g}k" if row['Edges'] >= 1000 else str(int(row['Edges']))
                return f"V: {v}\nE: {e}"
            
            df_grouped['X_Label'] = df_grouped.apply(format_label, axis=1)
            
            max_edges_reached = df_grouped['Edges'].max()
            time_at_max_edges = df_grouped[df_grouped['Edges'] == max_edges_reached]['Wall_Avg_s'].iloc[0]
            
            alg_data_list.append({
                'label': alg_name,
                'df': df_grouped,
                'max_edges': max_edges_reached,
                'time_at_max': time_at_max_edges
            })

    # Rangsorolás
    alg_data_list.sort(key=lambda x: (x['max_edges'], -x['time_at_max']), reverse=True)
    if top_x is not None and top_x > 0:
        alg_data_list = alg_data_list[:top_x]
        
    all_wall_values = []
    all_cpu_values = []

    # 3. LÉPÉS: Kirajzolás indexek alapján (Kategorikus X tengely)
    for alg in alg_data_list:
        df_grouped = alg['df']
        alg_label = alg['label']
        
        # Az x_data most egyszerűen 0, 1, 2... indexek lesznek, így egyenletes lesz a követés
        x_data = df_grouped.index
        wall_data = df_grouped['Wall_Avg_s']
        cpu_data = df_grouped['CPU_Work_s']
        
        all_wall_values.extend(wall_data.tolist())
        all_cpu_values.extend(cpu_data.tolist())
        
        ax1.plot(x_data, wall_data, marker='o', linewidth=2, label=alg_label)
        ax2.plot(x_data, cpu_data, marker='s', linestyle='--', linewidth=2, label=alg_label)

    # Beállítjuk a szöveges feliratokat az X-tengelyre az első algoritmus adatai alapján
    if alg_data_list:
        reference_df = alg_data_list[0]['df']
        ax1.set_xticks(reference_df.index)
        ax1.set_xticklabels(reference_df['X_Label'], fontsize=9)
        ax2.set_xticks(reference_df.index)
        ax2.set_xticklabels(reference_df['X_Label'], fontsize=9)

    # Formázások (NINCS LOG SKÁLA, mert a szövegek maguktól sorban vannak!)
    ax1.set_ylim(bottom=0)
    ax2.set_ylim(bottom=0)
    
    if is_zoomed and all_wall_values:
        ax1.set_ylim(0, pd.Series(all_wall_values).quantile(0.80) * 1.3)
        ax2.set_ylim(0, pd.Series(all_cpu_values).quantile(0.80) * 1.3)

    ax1.set_title("Futásidő", fontsize=14)
    ax1.set_xlabel("Gráf Paraméterek (Csúcs és Él szám)", fontsize=12)
    ax1.set_ylabel("Átlagos idő (másodperc)", fontsize=12)
    ax1.grid(True, linestyle=':', alpha=0.7)
    ax1.legend(loc="upper left", fontsize=10)

    ax2.set_title("Összesített processzor munka", fontsize=14)
    ax2.set_xlabel("Gráf Paraméterek (Csúcs és Él szám)", fontsize=12)
    ax2.set_ylabel("CPU idő (másodperc)", fontsize=12)
    ax2.grid(True, linestyle=':', alpha=0.7)
    ax2.legend(loc="upper left", fontsize=10)

    plt.suptitle(plot_name if plot_name else "MST Algoritmusok Teljesítménye", fontsize=18, fontweight='bold')
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
    full_output = os.path.join(data_dir, f"mst_benchmark_plot_full_lin_{dir_name}.png")
    create_plot(csv_files, full_output, plot_name="MST Algoritmusok", show=True)

    # 2. Zoomolt grafikon legenerálása
    zoomed_output = os.path.join(data_dir, f"mst_benchmark_plot_zoomed_lin_{dir_name}.png")
    create_plot(csv_files, zoomed_output, is_zoomed=True, plot_name="MST Algoritmusok", show=True)

    # 3. Csak a legjobb 4 algoritmus megjelenítése
    top_output = os.path.join(data_dir, f"mst_benchmark_plot_top4_lin_{dir_name}.png")
    create_plot(csv_files, top_output, top_x=4, plot_name="MST Algoritmusok - Top 4 Leggyorsabb", show=True)

if __name__ == "__main__":
    main()