import os
import glob
import pandas as pd
import matplotlib.pyplot as plt

def create_plot(csv_files, output_filename, is_zoomed=False, top_x=None, names=None, plot_name=None, show=False):
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 7))
    
    # 1. LÉPÉS: Adatok begyűjtése és kiértékelése
    alg_data_list = []
    
    for file in csv_files:
        filename = os.path.basename(file)
        alg_name = filename.replace("_results.csv", "").replace("_", " ")
        if not names == None and alg_name not in names:
            continue
        df = pd.read_csv(file)
        
        # Timeout ellenőrzés
        timeout_rows = df[df['Status'] == 'TIMEOUT']
        if not timeout_rows.empty:
            timeout_size_m = timeout_rows['Size'].iloc[0] / 1_000_000
            alg_label = f"{alg_name} (Timeout {timeout_size_m:g}M-nál)"
        else:
            alg_label = alg_name
            
        df_valid = df.dropna(subset=['Wall_Avg_s', 'CPU_Work_s'])
        
        if not df_valid.empty:
            # Rangsorolási metrikák kiszámítása
            max_size_reached = df_valid['Size'].max()
            # Kikeressük az ehhez a legnagyobb mérethez tartozó átlagos futásidőt
            time_at_max_size = df_valid[df_valid['Size'] == max_size_reached]['Wall_Avg_s'].iloc[0]
            
            alg_data_list.append({
                'label': alg_label,
                'df': df_valid,
                'max_size': max_size_reached,
                'time_at_max': time_at_max_size
            })

    # 2. LÉPÉS: Algoritmusok rangsorolása
    alg_data_list.sort(key=lambda x: (x['max_size'], -x['time_at_max']), reverse=True)

    # 3. LÉPÉS: Levágás a top_x paraméter alapján
    if top_x is not None and top_x > 0:
        alg_data_list = alg_data_list[:top_x]
        
    all_wall_values = []
    all_cpu_values = []

    # 4. LÉPÉS: A (már szűrt és rendezett) adatok kirajzolása
    for alg in alg_data_list:
        df_valid = alg['df']
        alg_label = alg['label']
        
        x_data_millions = df_valid['Size'] / 1_000_000
        wall_data = df_valid['Wall_Avg_s']
        cpu_data = df_valid['CPU_Work_s']
        
        all_wall_values.extend(wall_data.tolist())
        all_cpu_values.extend(cpu_data.tolist())
        
        ax1.plot(x_data_millions, wall_data, marker='o', linewidth=2, label=alg_label)
        ax2.plot(x_data_millions, cpu_data, marker='s', linestyle='--', linewidth=2, label=alg_label)

    # Mindkét grafikon alja fixen 0
    ax1.set_ylim(bottom=0)
    ax2.set_ylim(bottom=0)

    # --- DINAMIKUS ZOOM (Levágjuk az extrém kiugrókat) ---
    if is_zoomed and all_wall_values:
        # A 80. percentilist vesszük alapul, és adunk neki egy kis margót (x1.3)
        # Ez automatikusan levágja a nagyon lassú (pl. ThreadPool) vonalakat
        zoom_wall_limit = pd.Series(all_wall_values).quantile(0.80) * 1.3
        zoom_cpu_limit = pd.Series(all_cpu_values).quantile(0.80) * 1.3
        
        ax1.set_ylim(0, zoom_wall_limit)
        ax2.set_ylim(0, zoom_cpu_limit)


    # --- Bal oldali ábra (Wall Time) formázása ---
    ax1.set_xscale('log')
    ax1.set_title("Futásidő", fontsize=14)
    ax1.set_xlabel("Vektor mérete (Millió elem)", fontsize=12)
    ax1.set_ylabel("Átlagos idő (másodperc)", fontsize=12)
    ax1.grid(True, linestyle=':', alpha=0.7)
    
    # Legend pozíciója kívülre vagy a bal felső sarokba
    ax1.legend(loc="upper left", fontsize=10)

    # --- Jobb oldali ábra (CPU Work) formázása ---
    ax2.set_xscale('log')
    ax2.set_title("Összesített processzor munka", fontsize=14)
    ax2.set_xlabel("Vektor mérete (Millió elem)", fontsize=12)
    ax2.set_ylabel("CPU idő (másodperc)", fontsize=12)
    ax2.grid(True, linestyle=':', alpha=0.7)
    ax2.legend(loc="upper left", fontsize=10)

    if plot_name == None:
        plt.suptitle(f"Rendező Algoritmusok Teljesítménye", fontsize=18, fontweight='bold')
    else:
        plt.suptitle(plot_name, fontsize=18, fontweight='bold')
    plt.tight_layout()
    
    plt.savefig(output_filename, dpi=300)
    print(f"Ábra mentve: {output_filename}")
    
    if show:
        plt.show()
    else:
        plt.close()


def main():
    dir_name = "mergeSort"
    data_dir = f"test_data/sorting/{dir_name}/"
    csv_files = glob.glob(os.path.join(data_dir, "*_results.csv"))
    
    if not csv_files:
        print(f"Hiba: Nem találtam CSV fájlokat a '{data_dir}' mappában.")
        return

    # # 1. Teljes grafikon legenerálása (mindenki látszik)
    # full_output = os.path.join(data_dir, f"benchmark_plot_{dir_name}.png")
    # create_plot(csv_files, full_output, show=True)

    # # 2. Zoomolt grafikon legenerálása (csak a gyorsak versenye látszik)
    # zoomed_output = os.path.join(data_dir, f"benchmark_plot_zoomed_{dir_name}.png")
    # create_plot(csv_files, zoomed_output, is_zoomed=True, show=True)

    # 3.
    merge3 = os.path.join(data_dir, f"benchmark_plot_best5_{dir_name}.png")
    create_plot(csv_files, merge3, top_x=5, names=["Parallel Merge Sort", "Parallel Merge Sort 2 - 100", "Parallel Merge Sort 3"], show=True)

    # 4. 
    # bubble_output = os.path.join(data_dir, f"benchmark_plot_reduction_{dir_name}.png")
    # create_plot(csv_files, bubble_output, 
    #             names=["Serial Odd-Even Bubble", "Parallel Odd-Even Bubble"], 
    #             plot_name="bubble sort algoritmusok",
    #             show=True)

    # # 5.
    # tomorites_output = os.path.join(data_dir, f"benchmark_plot_compression_{dir_name}.png")
    # create_plot(csv_files, tomorites_output,
    #             names=["Serial", "OMP Compression", "Strided Compression", "Recursive Async"],
    #             plot_name="Tömörítő algoritmusok",
    #             )

    

    print("\nSikeresen legeneráltam mindkét ábrát!")

if __name__ == "__main__":
    main()