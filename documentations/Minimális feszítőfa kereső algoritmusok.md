# Minimális feszítőfa (MST) kereső algoritmusok

Ez a dokumentum a klasszikus minimális feszítőfa (Minimum Spanning Tree – MST) algoritmusokat foglalja össze, kitérve a szekvenciális (soros) futtatásra és a modern, többmagos processzorokra tervezett párhuzamos implementációkra. Az MST egy összefüggő, irányítatlan gráf éleinek olyan részhalmaza, amely minden csúcsot összeköt körök kialakulása nélkül, és ahol az élek összsúlya a lehető legkisebb.

Források: [GeeksforGeeks - Spanning Tree](https://www.geeksforgeeks.org/dsa/spanning-tree/)
## Feladat
A feladat egy G(V, E) gráfban egy minimális összsúlyú feszítőfát keresni és ennek a súlyát kiszámolni. A gráf összefüggő és minden élsúly pozitív 0-nál nagyobb egész szám. 

## Jelölések

- **G:** gráf
- **V**: Csúcsok (Vertices) száma
- **E**: Élek (Edges) száma

## Gráf Reprezentációk és Komplexitásuk

A gráf tárolásának módja kritikus hatással van az algoritmusok sebességére:

- **Szomszédsági mátrix:** Gyors elérést biztosít ($O(1)$), de nagy a memóriaigénye ($O(V^2)$).
    
- **Szomszédsági lista:** Sokkal helytakarékosabb ($O(E)$), viszont az élek keresése lassabb ($O(\log E)$ vagy átlagosan $O(E/V)$ egy adott csúcsból indulva).
    
## 0. Diszjunkt halmazok uniója (Disjoint Set Union - DSU - Unio-Holvan)

A Disjoint Set Union (DSU), más néven Union-Find, egy olyan adatszerkezet, amely egymástól diszjunkt (közös elem nélküli) halmazok nyilvántartására szolgál. Az MST algoritmusoknál (különösen a Kruskal és a Borůvka esetében) ez az adatszerkezet felel a fák (komponensek) egyesítéséért és a **rendkívül hatékony kördetektálásért**. Két alapvető művelete van: a `find` (megkeresi, melyik halmaz gyökeréhez tartozik egy elem) és a `unite` (két halmazt egyesít).

A magas teljesítmény eléréséhez a DSU két kritikus optimalizációt alkalmaz:

### Optimalizációk

- **Útvonal tömörítés (Path Compression):** Amikor a `find` művelet során elindulunk egy csúcstól felfelé, és kikeressük annak legfelső gyökerét, egyúttal optimalizáljuk is a fát. Minden csúcsnak, amelyet az út során érintettünk, a szülő mutatóját közvetlenül a frissen megtalált gyökérre állítjuk át. Ennek köszönhetően a jövőbeni lekérdezések ezekből a csúcsokból (és az ő leszármazottaikból) már szinte azonnal elérik a gyökeret, drasztikusan ellapítva a fa szerkezetét.
    
- **Rang szerinti egyesítés (Union by Rank):** Az egyesítések (`unite`) során el kell kerülni, hogy a fák egyensúlyozatlanul mélyre nőjenek (ami lassítaná a jövőbeli bejárásokat). A DSU-ban minden halmaz egy 0-ás rangról indul. Amikor két halmazt egyesítünk, mindig a kisebb rangú (sekélyebb) fát kötjük a nagyobb rangú fa gyökeréhez. A rang csak akkor növekszik (pontosan eggyel), ha két tökéletesen megegyező rangú fát vonunk össze. Rangegyenlőség esetén egy előre meghatározott szabály dönti el a hierarchiát (például a kisebb indexű csúcs marad a gyökér).
    

> **A komplexitás eredménye:** A két optimalizáció együttes alkalmazásával a DSU műveleteinek amortizált időkomplexitása $O(\alpha(V))$ lesz, ahol $\alpha$ az inverz Ackermann-függvény. Ez az érték bármilyen gyakorlati méretű gráf (akár milliárdnyi csúcs) esetén $\le 4$, így a kördetektálás és az egyesítés a gyakorlatban **konstans idejűnek** ($O(1)$) tekinthető.

### Szálbiztos / Párhuzamos változat (Parallel DSU)

Amikor modern, többmagos processzorokon futtatunk párhuzamos MST algoritmusokat (például Párhuzamos Borůvka vagy Parallel Prim), a klasszikus DSU implementáció végzetes hibákat okozhat. Ha egy szál épp útvonalat tömörít, miközben egy másik szál egy új ágat csatol a fához, a mutatók átírása miatt a fa "szétszakadhat", és a gráf érvénytelenné válik.

Ennek elkerülésére a **szálbiztos DSU** a következő fejlett technikákat alkalmazza:

- **Atomi mutatók:** A csúcsok szülő-kapcsolatait atomi változókként (`std::atomic<int>`) tároljuk. Mivel a gyökérkeresés a leggyakoribb művelet, az olvasásokhoz a leggyorsabb, memóriakorlátok nélküli `memory_order_relaxed` utasítást használjuk, így elkerülve a szálak közti várakozást (Lock-Free olvasás).
    
- **Útvonal-felezés (Path Halving):** A teljes útvonaltömörítés (ami hosszú utakat ír át visszamenőleg) párhuzamosan nem biztonságos. Ehelyett a `find` művelet az útvonal-felezést használja: ahogy halad felfelé, minden csúcsot csak a saját nagyszülőjéhez köt át. Ez egyetlen, oszthatatlan lépésben történik, így még akkor sem rontja el a fa struktúráját, ha a gyökeret közben egy másik szál áthelyezi. Ez aszimptotikusan majdnem ugyanolyan jó laposítást eredményez, de teljesen zárolásmentesen (Lock-Free).
    
- **Finomszemcsés zárolás egyesítéskor:** Bár a `find` zárolásmentes, az `unite` művelet végrehajtásakor a két érintett gyökeret egy nagyon rövid időre egyszerre kell zárolni (pl. C++ `std::scoped_lock` segítségével). Ez a deadlock-mentes zárolás garantálja, hogy a rang szerinti összevonás biztonságosan lefusson anélkül, hogy a fák inkonzisztens állapotba kerülnének.

## 1. Kruskal algoritmusa

A Kruskal-algoritmus egy **éleken alapuló mohó megközelítés**.

Forrás: [GeeksforGeeks - Kruskal's MST](https://www.geeksforgeeks.org/dsa/kruskals-minimum-spanning-tree-algorithm-greedy-algo-2/)

### Rövid leírás

1. **Rendezzük** a gráf összes élét súly szerint, nem csökkenő sorrendbe.
    
2. Inicializáljunk egy erdőt, ahol kezdetben minden csúcs egy különálló komponenst (fát) alkot.
    
3. Iteráljunk végig a rendezett éleken, és adjuk hozzá az adott élt az MST-hez, ha az két különböző komponenst köt össze (vagyis nem hoz létre kört).
    

### Implementációs részletek

- **Gráf reprezentáció:** Éllista (Edge List).
    
- **Segédstruktúra:** **Diszjunkt halmazok (Disjoint Set Union - DSU)** 
    
- **Él rendezés optimalizáció:** A teljes $O(E \log E)$ idejű rendezés helyett használható egy **Min-Kupac *(Min-Heap*)** a legkisebb élek egyesével történő kinyerésére ($O(E + k \log E)$). Ez különösen akkor hatékony, ha az MST-t megtaláljuk még azelőtt, hogy az összes élt feldolgoznánk.
    
- **Időkomplexitás:** $O(E \log E)$ vagy $O(E \log V)$.
    

## 2. Prim algoritmusa

A Prim-algoritmus egy csúcsokon alapuló mohó megközelítés.

### Leírás

1. Kezdjük egy tetszőlegesen kiválasztott csúccsal, és adjuk hozzá az MST-hez.
    
2. Minden iterációban keressük meg azt a legkisebb súlyú élt, amely egy már MST-n belüli csúcsot köt össze egy MST-n kívüli csúccsal.
    
3. Ismételjük ezt addig, amíg az összes csúcs be nem kerül az MST-be.
    

### Szekvenciális (Soros) Implementációs részletek

- **Sűrű gráfok (Dense Graphs):** Használjunk Szomszédsági mátrixot (Adjacency Matrix) és egy tömböt, amely nyomon követi a még nem meglátogatott csúcsokhoz vezető minimális élsúlyokat.
    
    - Legjobb sűrű gráfokhoz, ahol $E \approx V^2$.
        
    - **Idő:** $O(V^2)$.
        
- **Ritka gráfok (Sparse Graphs):** Használjunk Szomszédsági listát (Adjacency List) és egy Min-Kupacot (Prioritási sor - Priority Queue) a következő legkisebb él gyors megtalálásához.
    
    - Legjobb ritka gráfokhoz.
        
    - **Idő:** $O((E + V) \log V)$.
        

### Párhuzamos Prim algoritmusa (Setia-féle megközelítés)

A Prim algoritmus párhuzamosítása kihívást jelent az inerens szekvenciális (fa-növesztő) természete miatt. A `parallelPrimSetiaMST` implementáció több szálon, egyidejűleg indít el fa-növesztéseket különböző "színezetlen" gyökerekből.

**Forrás:** [Parallel Minimum Spanning Tree Algorithm - Xiwen Chen](https://wiki.eecs.yorku.ca/course_archive/2010-11/W/6490A/_media/public:xiwen.pdf)

#### **Implementáció és Technikák:**

- Az alapelv a főbb ciklusok párhuzamosítása. Ahelyett, hogy egyetlen fát építenénk, egyszerre annyi fát növesztünk, ahány szál (thread) rendelkezésre áll, majd ezeket összekötjük.

- **Segédstruktúra:** **Szálbiztos Diszjunkt halmazok (Disjoint Set Union - DSU)**
    
- **Szál-lokális állapotok:** Minden szál (`ThreadData`) saját prioritási sort (`pq`) és részleges MST súlyt tart karban. A globális (központi) változók helyett szál-specifikus számlálókat érdemes használni a probléma méretének mérésére, csökkentve a zárolási (lock) konfliktusokat.
    
- **Szinkronizáció:** Kerüljük el a sok szál által egyidejűleg módosított kritikus szakaszokat, és csak rövid zárolásokat (lockokat) alkalmazzunk.
    
- **Ütközések kezelése (MergeTree):** Ha egy szál olyan csúcsot talál, amely már egy másik szál fáihoz tartozik, a fák ütköznek, és a két szálnak össze kell olvadnia (a kisebb azonosítójú (ID) szál beolvasztja a nagyobbat).
    
- **Az "Elveszett Élek" (Orphaned Edge) problémája:** Gyakori hiba, hogy ha egy élt kiveszünk a feldolgozáshoz, de a szálat időközben beolvasztja egy másik (mielőtt az élt lekezelné), az él örökre elveszhet.
    
    - **A "Peek, Process, Pop" megoldás:** Az élt csak megnézzük, és szigorúan csak azután vesszük ki a sorból, ha a csúcs foglalása és színezése sikeresen megtörtént, vagy ha az élt biztonságosan átadtuk a nyertes szálnak.
    
- **Piroritási sorok egyesítése:** A prioritási sorok egyesítése standard bináris kupac esetében $O(N)$ lépésszámú, ami gyenge hatékonyságú az algoritmus többi részéhez képest, tekintve hogy gyakoriak az egyesítések. A c++ `std priority queue` helyett `Pairing Heap` adatszerkezetet használ az implementáció, amelynek az egyesítési komplexitása $O(1)$. [Pairing heap forrás](https://www.geeksforgeeks.org/dsa/pairing-heap/)

#### **Heurisztikus Optimalizáció (`parallelPrimSetiaMSTWithHeuristic`):**

A tiszta Parallel Prim hajlamos a szálak közti intenzív versengésre és a "Cache Line Bouncing"-ra. Annak eldöntésére, hogy egy szál mikor álljon le végleg több opció is implementálható. Az alap implementációmban egy közös változóban tárolom, hogy hány csúcs van már benne az MST-ben. Viszont az, hogy egy változót sokszor kell írni több szálnak versengéshez vezethet.  Ezek mérséklésére dinamikus heurisztikák alkalmazhatók:

1. **Véletlenszerű kezdőpont (Randomization):** A szálak egy szál-specifikus (Thread-Local) véletlenszám-generátorral (a lockok elkerülése érdekében) keresnek új, színezetlen gyökeret. Minden szál máshonnan kezdi a fa építését, elkerülve a kezdeti ütközéseket.
    
2. **"Keresési" Heurisztika:** Ha egy szálnak sokadszorra sem sikerül feldolgozatlan (színezetlen) csúcsot találnia, az azt jelenti, hogy a gráf nagy része már le van fedve, ezért a szál békésen leáll (tétlen lesz).
    
3. **"Fa méret" Heurisztika:** A cél, hogy a fák minél nagyobbra nőjenek, mielőtt össze kellene őket olvasztani. Ha egy szálat folyamatosan, nagyon kis korában beolvasztanak (vagy a probléma mérete túlságosan lecsökken), a szál kilép, átadva a CPU időt a sikeresebb fáknak. Elég kis probléma méretnél az algoritmus akár át is válthat a szekvenciális (soros) futtatásra a nagyobb hatékonyság érdekében.

A keresési és a fa méret heurisztika egy szemléletmódváltást tartalmaz a szálak kezelésében. Ahelyett, hogy a szálakat közösen kezelnénk közös paraméterek alapján, minden szál saját magát irányítja próbálkozás és lokális információk alapján. Emiatt nem biztosított, hogy minden helyzetben optimális stratégiát választ minden szál, de sok esetben elég ha statisztikailag valószínű, hogy jól dönt. A szemléletmód nyeresége viszont, hogy nem kell közös változókat frissíteni, így sok várakozást és versengést elkerülhetünk.

## 3. Borůvka algoritmusa

A Borůvka-algoritmus egy **komponenseken (fákon) alapuló megközelítés**, amely természeténél fogva a leginkább alkalmas párhuzamosításra (hiszen kezdetben minden csúcs egy önálló fa).

### Leírás

1. Kezdjük úgy, hogy minden csúcs egy különálló komponenst (fát) képvisel.
    
2. Minden iterációban keressük meg _minden_ komponenshez azt a legolcsóbb élt, amely egy másik komponenshez köti.
    
3. Adjuk hozzá ezeket az éleket az MST-hez, és vonjuk össze (merge) a komponenseket.
    
4. Ismételjük addig, amíg az egész gráf egyetlen komponenssé nem válik.
    

### Szekvenciális (Soros) Implementáció

- Az éleken iterálva minden komponenshez eltároljuk a legolcsóbb kimenő élt egy `cheapest` tömbben.
    
- Ezután a `cheapest` éleken iterálva DSU (Disjoint Set Union) segítségével összevonjuk a komponenseket.
    

### Párhuzamos Borůvka algoritmusok

Mivel minden komponens legolcsóbb élének megkeresése független folyamat, az algoritmus kiválóan párhuzamosítható.

#### Implementáció és technikák
- **Segédstruktúra:** **Szálbiztos Diszjunkt halmazok (Disjoint Set Union - DSU)** 

#### **1. Naiv Párhuzamos megközelítés (`naivParallelBoruvkaMST`)**:

- **Implementáció:** A legkisebb súlyú kimenő élek (`cheapest` vektor) keresését párhuzamosítjuk.
    
- **Szinkronizáció és Probléma:** Amikor egy szál frissíteni akarja a közös `cheapest` vektort, azt egy kritikus szakaszba (pl. `#pragma omp critical`) kell zárni. Ez komoly szűk keresztmetszet, mivel a kritikus szekció megakasztja az összes többi szálat a futásban, lassítva a teljesítményt az összegzésnél (Summation Problem). Ezen kívül a DSU általi egyesítések is figyelmet igényelnek a deadlockok (holtpontok) elkerülése végett (pl. beépített `scoped_lock` használata).
    

#### **2. Pufferelt (Buffered) Párhuzamos megközelítés (`bufferedParallelBoruvkaMST`)**:

- **Implementáció:** Minden szál kap egy **saját, lokális `cheapest` puffert**. A szálak csak ide írnak a feltáró fázisban, elkerülve a kritikus szekciókat.
    
- **Összegzés:** A lokális pufferek tartalmát egy második lépésben fésüljük össze a globális adattáblába.
    
- **Előnyök:** Teljesen megszünteti a módosításkori zárolásokat a legdrágább (O(E)) fázisban.
    

#### **3. CAS (Compare-And-Swap) Párhuzamos megközelítés (`casParallelBoruvkaMST`)**:

Ez a megközelítés a legfejlettebb, teljesen zárolásmentes (Lock-Free) algoritmus, amely a modern processzorok hardveres atomi utasításait használja ki a szálak szinkronizálására, kiküszöbölve a mutexek miatti lassulást.

- **Implementáció (A CAS működése a kódodban):** A `cheapest` tömb elemeit atomi változókként (`std::atomic`) reprezentáljuk (gyakran egy 64 bites egész számba "csomagolva" az él súlyát és a célcsúcs azonosítóját, hogy egyetlen utasítással lehessen írni-olvasni). Amikor egy szál egy új, potenciálisan legolcsóbb élt talál egy komponenshez, a folyamat a következőképpen zajlik:
    
    1. A szál kiolvassa a `cheapest` tömb jelenlegi értékét (régi él).
        
    2. Ha a saját éle olcsóbb, megpróbálja felülírni a régit a hardveres **Compare-And-Swap (CAS)** (pl. `std::atomic_compare_exchange_weak`) utasítással.
        
    3. A CAS utasítás az operációs rendszert megkerülve, a processzor szintjén ellenőrzi, hogy a memória tartalma megváltozott-e az olvasás óta.
        
    4. Ha nem változott, a felülírás sikeres (az él bekerült). Ha közben egy másik szál már beírt egy másik élt, a CAS "elbukik". Ekkor a szál újraolvassa az új értéket, és ha a saját éle _még mindig_ olcsóbb, újra megpróbálja a cserét.
        
- **Gyenge írásvédelem (Weak Write Protection) logikája:** Hagyományos párhuzamos programozásnál (például egy közös számláló növelésénél: `x = x + 1`) a változó új értéke szigorúan függ a korábbi állapottól, ezért az adatvesztés elkerülésére erős zárolás (mutex) kell. Ezzel szemben az MST keresésnél a feltétel **abszolút**: minket kizárólag a _legkisebb súlyú_ él érdekel. Emiatt alkalmazható a gyenge írásvédelem:
    
    - Nem probléma, ha egy szál a feldolgozás pillanatában egy elavult (régi) értéket lát.
        
    - Ha a szál elavult adat alapján próbál írni, a CAS utasítás megvédi a memóriát: nem engedi, hogy felülírjon egy olyan élt, amit a háttérben egy másik szál már egy még kisebbre cserélt.
        
    - Ugyanez a gyenge írásvédelmi logika és abszolút feltételrendszer teszi lehetővé a zárolásmentes Párhuzamos DSU `find` műveletét is (Útvonal-felezés / Path Halving), ahol a fák mutatói folyamatosan változhatnak a háttérben, a struktúra mégis érvényes marad.
        
- **Előnyök:** Mivel nincsenek kritikus szekciók (`#pragma omp critical`) és mutexek, a szálak sosem kényszerülnek operációs rendszer szintű várakozásra vagy kontextusváltásra (Context Switch). Még extrém sűrű gráfok esetén is (ahol rengeteg szál próbálná ugyanazt a komponenst frissíteni) masszív skálázódást és sebességet biztosít. A mérési eredmények alapján jelenleg ez tekinthető a leggyorsabb többmagos MST algoritmusnak.
    

## Összegzés és összehasonlítás

| **Algoritmus**       | **Adatszerkezet**                     | **Kivitel** | **Feltételezett legjobb felhasználási eset**                          |
| -------------------- | ------------------------------------- | ----------- | --------------------------------------------------------------------- |
| **Kruskal**          | Éllista + DSU                         | Soros       | Ritka gráfok, egyszerű élkezelés esetén.                              |
| **Prim (Matrix)**    | Szomsz. mátrix                        | Soros       | Nagyon sűrű gráfok ($E \approx V^2$).                                 |
| **Prim (List)**      | Szomsz. lista + PQ                    | Soros       | Ritka gráfok, általános célú felhasználás.                            |
| **Parallel Prim**    | Szomsz. lista + PQ + Mutex            | Párhuzamos  |                                                                       |
| **Buffered Borůvka** | Szomsz. lista + Szál-lokális pufferek | Párhuzamos  | Ritka/Közepes gráfok, memóriaigényesebb, de nagyon gyors.             |
| **CAS Borůvka**      | Szomsz. lista + Atomi változók        | Párhuzamos  | A leggyorsabb általános párhuzamos megoldás, sűrű gráfokon is stabil. |
## Mérési eredmények és következtetések
## Mérési eredmények és grafikonok
### **Ritka gráfok** (Összes lehetséges él 5%-a)
![mst_log_sparse](pictures/mst_benchmark_plot_full_log_sparse.png)
![mst_lin_sparse](pictures/mst_benchmark_plot_full_lin_sparse.png)
![mst_top4_sparse](pictures/mst_benchmark_plot_top4_log_sparse.png)

### **Sűrű gráfok** (Összes lehetséges él 80%-a)
![mst_log_dense](pictures/mst_benchmark_plot_full_log_dense.png)
![mst_lin_dense](pictures/mst_benchmark_plot_full_lin_dense.png)
![mst_top4_dense](pictures/mst_benchmark_plot_top4_log_dense.png)
### Következtetések
- A **párhuzamos prim** algoritmusok hatékonysága a vártnál jelentősen rosszabb 
	-  Ennek okozója lehet az  implementáció nem megfelelő optimalizáltsága is
	- A forrás arra enged következtetni, hogy ennél jelentősen jobb hatékonyságot el lehet érni
	- Magas párhuzamosítási overhead: A szekvenciális algoritmusok (mint a Kruskal vagy a Prim List) annyira gyorsak és memóriabarátok ritka gráfokon (gyakran a tizedmásodperc töredéke alatt lefutnak), hogy a párhuzamosítás "adminisztrációs" költsége (a szálak indítása, a mutexek lefoglalása és elengedése) egyszerűen felemészti, sőt meghaladja a hasznos számítási időt (Amdahl törvényének hatása).
- A **Borůvka algoritmus** különböző implementációi a vártank megfelelően teljesítményt értek el, a CAS változat bizonyult a leghatékonyabbnak
	- A mérések egyértelműen bizonyítják a zárolásmentes (Lock-Free) párhuzamosítás fölényét. A CAS Borůvka azért tudja sűrű és ritka gráfokon is jobban teljesít minden másik algoritmusnál, mert a hardveres "Compare-And-Swap" atomi utasításokat használja a legkisebb élek frissítésére. Itt egyáltalán nincsenek mutexek, így nincsenek várakozó szálak és holtpontok sem; a modern többmagos processzorok teljes kapacitásukat a keresésre tudják fordítani.
- **Adatszerkezet és gráfsűrűség összefüggése:** A mérésekből tisztán látszik az elmélet gyakorlati igazolása: sűrű gráfok esetén a `Prim Matrix` algoritmus rendkívül gyors (hiszen egy mátrixban/tömbben $O(1)$ idő az élek vizsgálata), míg ritka gráfoknál ugyanez az algoritmus a felesleges nullák vizsgálata miatt veszít a hatékonyságából, és a `Prim List` veszi át a vezetést.