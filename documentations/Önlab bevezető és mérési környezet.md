# Párhuzamos algoritmusok implementációjának vizsgálata
## Bevezetés
Az önálló laboratóriumom témájának címe a párhuzamos algoritmusok implemetálásának vizsgálata. Azzal a témával foglalkozok, hogy soros algoritmusokat, hogyan lehet párhuzamosítani és matematikai párhuzamos algoritmusokat, hogy lehet hatékonyan implementálni. 
A matematikai algoritmusok sok esetben nincsenek figyelemmel a technikai korlátokra, például a maximum lehetséges szálak számára vagy a szálak létrehozásának többletköltségére. Ennek a önálló laboratóriumnak a célja, hogy ezeket az aspektusokat vizsgálja. 
Három algoritmus családot vizsgálok, a minimum keresést listában, rendezést listában és minimumfa keresést gráfokon.

## Fejlesztői és mérési környezet
Az algoritmusokat c++-ban implementáltam és kerestem hozzájuk implementációt. A minimum keresésnél összehaszonlítottam a c++ thread könyvtárát egy threadpool osztály használatával és a openMP könyvtár használatával. A további teszteléshez a konzisztencia és a hatékonyság miatt a openMP-t használtam kizárólag.
### A mérési laptop specifikációja 
#### Hardware Information:
- **Hardware Model:**                              ASUSTeK COMPUTER INC. ZenBook UX431FL_UX431FL
- **Memory:**                                      8.0 GiB
- **Processor:**                                   Intel® Core™ i5-8265U × 8
- **Graphics:**                                    Intel® UHD Graphics 620 (WHL GT2)
- **Graphics 1:**                                  NVIDIA GeForce MX250
- **Disk Capacity:**                               768.2 GB

#### Software Information:
- **Firmware Version:**                            UX431FL.304
- **OS Name:**                                     Ubuntu 24.04.4 LTS
- **OS Build:**                                    (null)
- **OS Type:**                                     64-bit
- **GNOME Version:**                               46
- **Windowing System:**                            X11
- **Kernel Version:**                              Linux 6.17.0-23-generic

#### Mérési környezet
Ubuntu operációs rendszeren végeztem a mérést. Az energia módot legjobb teljesítményre állítva  teszteltem. A teszek futása alatt nem volt megnyitva semmilyen alkalmazás a vscode-on kívül, amiben a tesztelést végeztem.