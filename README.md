# Screen Worms (description in Polish)
<div id="intro" class="box py-3 generalbox boxaligncenter"><div class="no-overflow"><p dir="ltr" style="text-align: left;"></p><h2 id="gra-robaki-ekranowe">1. Gra robaki ekranowe</h2>
<h3 id="zasady-gry">1.1. Zasady gry</h3>
<p>Tegoroczne duże zadanie zaliczeniowe polega na napisaniu gry 
sieciowej. Gra rozgrywa się na prostokątnym ekranie. Uczestniczy w niej 
co najmniej dwóch graczy. Każdy z graczy steruje ruchem robaka. Robak je
 piksel, na którym się znajduje. Gra rozgrywa się w turach. W każdej 
turze robak może się przesunąć na inny piksel, pozostawiając ten, na 
którym był, całkowicie zjedzony. Robak porusza się w kierunku ustalonym 
przez gracza. Jeśli robak wejdzie na piksel właśnie jedzony lub już 
zjedzony albo wyjdzie poza ekran, to spada z ekranu, a gracz nim 
kierujący odpada z gry. Wygrywa ten gracz, którego robak pozostanie jako
 ostatni na ekranie. Szczegółowy algorytm robaka jest opisany poniżej.</p>
<h3 id="architektura-rozwiązania">1.2. Architektura rozwiązania</h3>
<p>Na grę składają się trzy komponenty: serwer, klient, serwer 
obsługujący interfejs użytkownika. Należy zaimplementować serwer i 
klient. Aplikację implementującą serwer obsługujący graficzny interfejs 
użytkownika (ang. <em>GUI</em>) dostarczamy.</p>
<p>Serwer komunikuje się z klientami, zarządza stanem gry, odbiera od 
klientów informacje o wykonywanych ruchach oraz rozsyła klientom zmiany 
stanu gry. Serwer pamięta wszystkie zdarzenia dla bieżącej partii i 
przesyła je w razie potrzeby klientom.</p>
<p>Klient komunikuje się&nbsp;z serwerem gry oraz interfejsem użytkownika. 
Klient dba także o to, żeby interfejs użytkownika otrzymywał polecenia w
 kolejności zgodnej z przebiegiem partii oraz bez duplikatów.</p>
<p>Specyfikacje protokołów komunikacyjnych, rodzaje zdarzeń oraz formaty komunikatów i poleceń są opisane poniżej.</p>
<h3 id="parametry-wywołania-programów">1.3. Parametry wywołania programów</h3>
<p>Serwer:</p>
<pre><code>./screen-worms-server [-p n] [-s n] [-t n] [-v n] [-w n] [-h n]</code></pre>

  * `-p n` – numer portu (domyślnie `2021`)
  * `-s n` – ziarno generatora liczb losowych (opisanego poniżej, domyślnie
    wartość uzyskana przez wywołanie `time(NULL)`)
  * `-t n` – liczba całkowita wyznaczająca szybkość skrętu
    (parametr `TURNING_SPEED`, domyślnie `6`)
  * `-v n` – liczba całkowita wyznaczająca szybkość gry
    (parametr `ROUNDS_PER_SEC` w opisie protokołu, domyślnie `50`)
  * `-w n` – szerokość planszy w pikselach (domyślnie `640`)
  * `-h n` – wysokość planszy w pikselach (domyślnie `480`)

  <p>Klient:</p>
<pre><code>./screen-worms-client game_server [-n player_name] [-p n] [-i gui_server] [-r n]</code></pre>
<ul>
<li><code>game_server</code> – adres (IPv4 lub IPv6) lub nazwa serwera gry</li>
<li><code>-n player_name</code> – nazwa gracza, zgodna z opisanymi niżej wymaganiami</li>
<li><code>-p n</code> – port serwera gry (domyślne <code>2021</code>)</li>
<li><code>-i gui_server</code> – adres (IPv4 lub IPv6) lub nazwa serwera obsługującego interfejs użytkownika (domyślnie <code>localhost</code>)</li>
<li><code>-r n</code> – port serwera obsługującego interfejs użytkownika (domyślnie <code>20210</code>)</li>
</ul>
<p>Do parsowania parametrów linii komend można użyć funkcji <code>getopt</code> z biblioteki standardowej: <a href="https://linux.die.net/man/3/getopt">https://linux.die.net/man/3/getopt</a>.</p>
<h2 id="protokół-komunikacyjny-pomiędzy-klientem-a-serwerem">2. Protokół komunikacyjny pomiędzy klientem a serwerem</h2>
<p>Wymiana danych odbywa się po UDP. W datagramach przesyłane są dane 
binarne, zgodne z poniżej zdefiniowanymi formatami komunikatów. W 
komunikatach wszystkie liczby przesyłane są w sieciowej kolejności 
bajtów.</p>
<h3 id="komunikaty-od-klienta-do-serwera">2.1. Komunikaty od klienta do serwera</h3>
<p>Komunikat od klienta do serwera ma kolejno następujące pola:</p>
<ul>
<li><code>session_id</code>: 8 bajtów, liczba bez znaku</li>
<li><code>turn_direction</code>: 1 bajt, liczba bez znaku, wartość <code>0</code> → prosto, wartość <code>1</code> → w prawo, wartość <code>2</code> → w lewo</li>
<li><code>next_expected_event_no</code> – 4 bajty, liczba bez znaku</li>
<li><code>player_name</code>: 0–20 znaków ASCII o wartościach z przedziału 33–126, w szczególności spacje nie są dozwolone</li>
</ul>
<p>Klient wysyła taki datagram co 30 ms.</p>
<p>Komunikacja odbywa się zawsze, nawet jeśli partia się jeszcze nie rozpoczęła lub już się zakończyła.</p>
<p>Pole <code>turn_direction</code> wskazuje, czy gracz chce skręcać (czy ma wciśniętą którąś ze strzałek w lewo lub prawo na klawiaturze).</p>
<p>Puste pole <code>player_name</code> oznacza, że klient nie ma zamiaru włączać&nbsp;się do gry, jednakże chętnie poobserwuje, co się dzieje na planszy.</p>
<p>Pole <code>session_id</code> jest takie samo dla wszystkich datagramów wysyłanych przez danego klienta. Klient przy uruchomieniu ustala <code>session_id</code> na bieżący czas wyrażony w mikrosekundach od <code>1970-01-01 00:00:00 +0000 (UTC)</code>.</p>
<h3 id="komunikaty-od-serwera-do-klienta">2.2. Komunikaty od serwera do klienta</h3>
<p>Komunikat od serwera do klienta ma kolejno następujące pola:</p>
<ul>
<li><code>game_id</code>: 4 bajty, liczba bez znaku</li>
<li><code>events</code>: zmienna liczba rekordów, zgodnych z poniższą specyfikacją</li>
</ul>
<p>Serwer wysyła taki komunikat natychmiast po odebraniu komunikatu od 
klienta. Wysyła zdarzenia o numerach, począwszy od odebranego <code>next_expected_event_no</code>
 aż do ostatniego dostępnego. Jeśli takich zdarzeń nie ma, serwer nic 
nie wysyła w odpowiedzi. Serwer także wysyła taki komunikat do 
wszystkich klientów po pojawieniu się nowego zdarzenia.</p>
<p>Maksymalny rozmiar pola danych datagramu UDP wysyłanego przez serwer 
wynosi 548 bajtów. Jeśli serwer potrzebuje wysłać więcej zdarzeń, niż 
może zmieścić w jednym datagramie UDP, wysyła je w kolejnych 
komunikatach. W tym przypadku wszystkie oprócz ostatniego muszą zawierać
 maksymalną liczbę zdarzeń możliwą do umieszczenia w pojedynczym 
datagramie.</p>
<p>Pole <code>game_id</code> służy do identyfikacji bieżącej partii w 
sytuacji, gdy do klienta mogą dochodzić opóźnione datagramy z uprzednio 
zakończonej partii.</p>
<h3 id="rekordy-opisujące-zdarzenia">2.3. Rekordy opisujące zdarzenia</h3>
<p>Rekord opisujący zdarzenie ma następujący format:</p>
<ul>
<li><code>len</code>: 4 bajty, liczba bez znaku, sumaryczna długość pól <code>event_*</code></li>
<li><code>event_no</code>: 4 bajty, liczba bez znaku, dla każdej partii kolejne wartości, począwszy od zera</li>
<li><code>event_type</code>: 1 bajt</li>
<li><code>event_data</code>: zależy od typu, patrz opis poniżej</li>
<li><code>crc32</code>: 4 bajty, liczba bez znaku, suma kontrolna obejmująca pola od pola <code>len</code> do <code>event_data</code> włącznie, obliczona standardowym algorytmem CRC-32-IEEE</li>
</ul>
<p>Możliwe rodzaje zdarzeń:</p>
<ul>
<li><code>NEW_GAME</code>
<ul>
<li><code>event_type</code>: 0</li>
<li><code>event_data</code>:
<ul>
<li><code>maxx</code>: 4 bajty, szerokość planszy w pikselach, liczba bez znaku</li>
<li><code>maxy</code>: 4 bajty, wysokość planszy w pikselach, liczba bez znaku</li>
<li>następnie lista nazw graczy zawierająca dla każdego z graczy <code>player_name</code>, jak w punkcie „2.1. Komunikaty od klienta do serwera”, oraz znak <code>'\0'</code></li>
</ul></li>
</ul></li>
<li><code>PIXEL</code>
<ul>
<li><code>event_type</code>: 1</li>
<li><code>event_data</code>:
<ul>
<li><code>player_number</code>: 1 bajt</li>
<li><code>x</code>: 4 bajty, odcięta, liczba bez znaku</li>
<li><code>y</code>: 4 bajty, rzędna, liczba bez znaku</li>
</ul></li>
</ul></li>
<li><code>PLAYER_ELIMINATED</code>
<ul>
<li><code>event_type</code>: 2</li>
<li><code>event_data</code>:
<ul>
<li><code>player_number</code>: 1 bajt</li>
</ul></li>
</ul></li>
<li><code>GAME_OVER</code>
<ul>
<li><code>event_type</code>: 3</li>
<li><code>event_data</code>: brak</li>
</ul></li>
</ul>
<p>Wśród nazw graczy w zdarzeniu <code>NEW_GAME</code> nie umieszcza się pustych nazw obserwatorów.</p>
<p>Kolejność graczy w zdarzeniu <code>NEW_GAME</code> oraz ich numerację w zdarzeniach <code>PIXEL</code> i <code>NEW_GAME</code> ustala się, ustawiając alfabetycznie ich nazwy. Graczy numeruje się od zera.</p>
<h3 id="generator-liczb-losowych">2.4. Generator liczb losowych</h3>
<p>Do wytwarzania wartości losowych należy użyć poniższego 
deterministycznego generatora liczb 32-bitowych. Kolejne wartości 
zwracane przez ten generator wyrażone są&nbsp;wzorem:</p>
<pre><code>r_0 = seed
r_i = (r_{i-1} * 279410273) mod 4294967291</code></pre>
<p>gdzie wartość <code>seed</code> jest 32-bitowa i jest przekazywana do serwera za pomocą parametru <code>-s</code> (domyślnie są to 32 młodsze bity wartości zwracanej przez wywołanie <code>time(NULL)</code>). W pierwszym wywołaniu generatora powinna zostać zwrócona wartość <code>r_0 == seed</code>.</p>
<p>Należy użyć dokładnie takiego generatora, żeby umożliwić automatyczne
 testowanie rozwiązania (uwaga na konieczność wykonywania pośrednich 
obliczeń na typie 64-bitowym).</p>
<h3 id="stan-gry">2.5. Stan gry</h3>
<p>Podczas partii serwer utrzymuje stan gry, w skład którego wchodzą m.in.:</p>
<ul>
<li>numer partii (<code>game_id</code>), wysyłany w każdym wychodzącym datagramie</li>
<li>bieżące współrzędne robaka każdego z graczy (jako liczby 
zmiennoprzecinkowe o co najmniej podwójnej precyzji) oraz kierunek ruchu
 robaka</li>
<li>zdarzenia wygenerowane od początku gry (patrz punkt „2.3. Rekordy opisujące zdarzenia” oraz dalej)</li>
<li>zjedzone piksele planszy</li>
</ul>
<p>Lewy górny róg planszy ma współrzędne <code>(0, 0)</code>, odcięte rosną w prawo, a rzędne w dół. Kierunek ruchu jest wyrażony w stopniach, zgodnie z ruchem wskazówek zegara, a <code>0</code> oznacza kierunek w prawo.</p>
<p>Warto tu podkreślić, że bieżąca pozycja robaka jest obliczana i 
przechowywana w formacie zmiennoprzecinkowym. Przy konwersji pozycji 
zmiennoprzecinkowej na współrzędne piksela stosuje się zaokrąglanie w 
dół. Uznajemy, że pozycja po zaokrągleniu znajduje się na planszy, jeśli
 rzędna znajduje się w przedziale domkniętym <code>[0, maxy - 1]</code>, a odcięta w przedziale domkniętym <code>[0, maxx - 1]</code>.</p>
<h3 id="podłączanie-i-odłączanie-graczy">2.6. Podłączanie i odłączanie graczy</h3>
<p>Podłączenie nowego gracza może odbyć się w dowolnym momencie. 
Wystarczy, że serwer odbierze prawidłowy komunikat od nowego klienta. 
Jeśli nowy gracz podłączy się podczas partii, staje się jej 
obserwatorem, otrzymuje informacje o wszystkich zdarzeniach, które miały
 miejsce od początku partii. Do walki dołącza w kolejnej partii.</p>
<p>Jeśli podłączy się gracz, który w komunikatach przesyła puste pole <code>player_name</code>, to taki gracz nie walczy, ale może obserwować rozgrywane partie.</p>
<p>Brak komunikacji od gracza przez 2 sekundy skutkuje jego odłączeniem.
 Jeśli gra się już rozpoczęła, robak takiego gracza nie znika i nadal 
porusza się wg algorytmu z punktu „2.8. Przebieg partii”.</p>
<p>Klienty są identyfikowane za pomocą par (<code>gniazdo</code>, <code>session_id</code>), jednakże otrzymanie komunikatu z gniazda istniejącego klienta, aczkolwiek z większym niż dotychczasowe <code>session_id</code>, jest równoznaczne z odłączeniem istniejącego klienta i podłączeniem nowego. Komunikaty z mniejszym niż dotychczasowe <code>session_id</code> należy ignorować.</p>
<p>Pakiety otrzymane z nieznanego dotychczas gniazda, jednakże z nazwą podłączonego już klienta, są ignorowane.</p>
<h3 id="rozpoczęcie-partii-i-zarządzanie-podłączonymi-klientami">2.7. Rozpoczęcie partii i zarządzanie podłączonymi klientami</h3>
<p>Do rozpoczęcia partii potrzeba, aby wszyscy podłączeni gracze (o niepustej nazwie) nacisnęli strzałkę (przysłali wartość <code>turn_direction</code> różną od zera) oraz żeby tych graczy było co najmniej dwóch.</p>
<p>Stan gry jest inicjowany w następujący sposób (kolejność wywołań <code>rand()</code>
 ma znaczenie i należy użyć generatora z punktu „2.4. Generator liczb 
losowych”). Graczy inicjuje się w kolejności alfabetycznej ich nazw.</p>
<pre><code>game_id = rand()
wygeneruj zdarzenie NEW_GAME
dla kolejnych graczy zainicjuj pozycję i kierunek ruchu ich robaków
  x_robaka_gracza = (rand() mod maxx) + 0.5
  y_robaka_gracza = (rand() mod maxy) + 0.5
  kierunek_robaka_gracza = rand() mod 360
  jeśli piksel zajmowany przez robaka jest jedzony, to
    wygeneruj zdarzenie PLAYER_ELIMINATED
  w przeciwnym razie
    wygeneruj zdarzenie PIXEL</code></pre>
<p>A zatem z gry można odpaść już na starcie.</p>
<h3 id="przebieg-partii">2.8. Przebieg partii</h3>
<p>Partia składa się z tur. Tura trwa <code>1/ROUNDS_PER_SEC</code> sekundy. Ruchy graczy wyznacza się w kolejności alfabetycznej ich nazw.</p>
<pre><code>dla kolejnych graczy
  jeśli ostatni turn_direction == 1, to
    kierunek_robaka_gracza += TURNING_SPEED
  jeśli ostatni turn_direction == 2, to
    kierunek_robaka_gracza −= TURNING_SPEED
  przesuń robaka o 1 w bieżącym kierunku
  jeśli w wyniku przesunięcia robak nie zmienił piksela, to
    continue
  jeśli robak zmienił piksel na piksel jedzony lub już zjedzony, albo wyszedł poza planszę, to
    wygeneruj zdarzenie PLAYER_ELIMINATED
  w przeciwnym razie
    wygeneruj zdarzenie PIXEL</code></pre>
<h3 id="zakończenie-partii">2.9. Zakończenie partii</h3>
<p>Gdy na planszy zostanie tylko jeden robak, gra się kończy. Generowane jest zdarzenie <code>GAME_OVER</code>.
 Po zakończeniu partii serwer wciąż obsługuje komunikację z klientami. 
Jeśli w takiej sytuacji serwer otrzyma od każdego podłączonego klienta 
(o niepustej nazwie) co najmniej jeden komunikat z <code>turn_direction</code>
 różnym od zera, rozpoczyna kolejną partię. Klienty muszą radzić sobie z
 sytuacją, gdy po rozpoczęciu nowej gry będą dostawać jeszcze stare, 
opóźnione datagramy z poprzedniej gry.</p>
<h2 id="protokół-komunikacyjny-pomiędzy-klientem-a-interfejsem-użytkownika">3. Protokół komunikacyjny pomiędzy klientem a interfejsem użytkownika</h2>
<p>Wymiana danych odbywa się po TCP. Komunikaty przesyłane są&nbsp;w formie 
tekstowej, każdy w osobnej linii. Liczby są reprezentowane dziesiętnie. 
Linia zakończona jest znakiem o kodzie ASCII 10. Jeśli linia zawiera 
kilka wartości, to wysyłający powinien te wartości oddzielić pojedynczą 
spacją i nie dołączać dodatkowych białych znaków na początku ani na 
końcu.</p>
<p>Odbiorca niepoprawnego komunikatu ignoruje go. Odebranie komunikatu nie jest potwierdzane żadną wiadomością zwrotną.</p>
<p>Serwer obsługujący interfejs użytkownika akceptuje następujące komunikaty:</p>
<ul>
<li><code>NEW_GAME maxx maxy player_name1 player_name2 …</code></li>
<li><code>PIXEL x y player_name</code></li>
<li><code>PLAYER_ELIMINATED player_name</code></li>
</ul>
<p>Nazwa gracza to ciąg 1–20 znaków ASCII o wartościach z przedziału 33–126. Współrzędne <code>x</code>, <code>y</code> to liczby całkowite, odpowiednio od <code>0</code> do <code>maxx − 1</code> lub <code>maxy − 1</code>. Lewy górny róg planszy ma współrzędne <code>(0, 0)</code>, odcięte rosną w prawo, a rzędne w dół.</p>
<p>Serwer obsługujący interfejs użytkownika wysyła następujące komunikaty:</p>
<ul>
<li><code>LEFT_KEY_DOWN</code></li>
<li><code>LEFT_KEY_UP</code></li>
<li><code>RIGHT_KEY_DOWN</code></li>
<li><code>RIGHT_KEY_UP</code></li>
</ul>
<h2 id="ustalenia-dodatkowe">4. Ustalenia dodatkowe</h2>
<p>Programy powinny umożliwiać komunikację&nbsp;zarówno przy użyciu IPv4, jak i IPv6.</p>
<p>W implementacji programów duże kolejki komunikatów, zdarzeń itp. powinny być alokowane dynamicznie.</p>
<p>Przy parsowaniu ciągów zdarzeń z datagramów przez klienta:</p>
<ul>
<li>pierwsze zdarzenie z niepoprawną sumą kontrolną powoduje 
zaprzestanie przetwarzania kolejnych w tym datagramie, ale poprzednie 
pozostają w mocy;</li>
<li>rekord z poprawną sumą kontrolną, znanego typu, jednakże z 
bezsensownymi wartościami, powoduje zakończenie klienta z odpowiednim 
komunikatem i kodem wyjścia 1;</li>
<li>pomija się zdarzenia z poprawną sumą kontrolną oraz nieznanym typem.</li>
</ul>
<p>Program klienta w przypadku błędu połączenia z serwerem gry lub 
interfejsem użytkownika powinien się zakończyć z kodem wyjścia 1, 
uprzednio wypisawszy zrozumiały komunikat na standardowe wyjście błędów.</p>
<p>Program serwera powinien być odporny na sytuacje błędne, które dają 
szansę na kontynuowanie działania. Intencja jest taka, że serwer 
powinien móc być uruchomiony na stałe bez konieczności jego 
restartowania, np. w przypadku kłopotów komunikacyjnych, czasowej 
niedostępności sieci, zwykłych zmian jej konfiguracji itp.</p>
<p>Serwer nie musi obsługiwać&nbsp;więcej niż 25 podłączonych graczy 
jednocześnie. Dodatkowi gracze ponad limit nie mogą jednak przeszkadzać 
wcześniej podłączonym.</p>
<p>W serwerze opóźnienia w komunikacji z jakimś podzbiorem klientów nie 
mogą wpływać na jakość komunikacji z pozostałymi klientami. Analogicznie
 w kliencie opóźnienia w komunikacji z interfejsem użytkownika nie mogą 
wpływać na regularność wysyłania komunikatów do serwera gry. Patrz też: <a href="https://stackoverflow.com/questions/4165174/when-does-a-udp-sendto-block">https://stackoverflow.com/questions/4165174/when-does-a-udp-sendto-block</a>.</p>
<p>Czynności okresowe (tury oraz wysyłanie komunikatów od klienta do 
serwera) powinny być wykonywane w odstępach niezależnych od czasu 
przetwarzania danych, obciążenia komputera czy też obciążenia sieci. 
Implementacja, która np. robi <code>sleep(20ms)</code> nie spełnia tego 
warunku. Dopuszczalne są krótkofalowe odchyłki, ale długofalowo średni 
odstęp czynności musi być zgodny ze specyfikacją.</p>
<p>Na połączeniu klienta z serwerem obsługującym interfejs użytkownika 
powinien zostać wyłączony algorytm Nagle'a, aby zminimalizować 
opóźnienia transmisji.</p>
<p>W przypadku otrzymania niepoprawnych argumentów linii komend, 
programy powinny wypisywać stosowny komunikat na standardowe wyjście 
błędów i zwracać kod 1.</p>
<p>Należy przyjąć rozsądne ograniczenia na wartości parametrów, w 
szczególności rozsądne ograniczenie na maksymalny rozmiar planszy, 
rozsądne limity na wartości parametrów <code>ROUNDS_PER_SEC</code> i <code>TURNING_SPEED</code>, dopuszczać tylko dodatnie wartości parametru tam, gdzie zerowa lub ujemna wartość nie ma sensu.</p>
<p>Nazwa gracza <code>player_name</code> nie kończy się znakiem o kodzie zero.</p>
<p>W każdej nowej grze numerowanie zdarzeń rozpoczyna się od zera.</p>
<p>Jeśli na przykład odbierzemy komunikat <code>LEFT_KEY_DOWN</code> bezpośrednio po komunikacie <code>RIGHT_KEY_DOWN</code>, to skręcamy w lewo. Generalnie uwzględniamy zawsze najnowszy komunikat.</p>
<p>Protokół zabrania obsługi kilku graczy przez jednego klienta i 
nakazuje wysyłać w ramach jednej gry zawsze jednakową nazwę gracza.</p>
<p>Przy przetwarzaniu sieciowych danych binarnych należy używać typów o ustalonym rozmiarze: <a href="http://en.cppreference.com/w/c/types/integer">http://en.cppreference.com/w/c/types/integer</a>.</p>
<p>Patrz też: <a href="http://man7.org/linux/man-pages/man2/gettimeofday.2.html">http://man7.org/linux/man-pages/man2/gettimeofday.2.html</a>, <a href="https://stackoverflow.com/questions/809902/64-bit-ntohl-in-c">https://stackoverflow.com/questions/809902/64-bit-ntohl-in-c</a>.</p>
<h2 id="oddawanie-rozwiązania">5. Oddawanie rozwiązania</h2>
<p>Jako rozwiązanie można oddać tylko serwer (część A) lub tylko klienta (część B), albo obie części.</p>
<p>Rozwiązanie ma:</p>
<ul>
<li>działać w środowisku Linux;</li>
<li>być napisane w języku C lub C++ z wykorzystaniem interfejsu gniazd (nie wolno korzystać z <code>libevent</code> ani <code>boost::asio</code>);</li>
<li>kompilować się za pomocą GCC (polecenie <code>gcc</code> lub <code>g++</code>) – wśród parametrów kompilacji należy użyć <code>-Wall</code>, <code>-Wextra</code> i <code>-O2</code>, zalecamy korzystanie ze standardów <code>-std=c11</code> i <code>-std=c++17</code>.</li>
</ul>
<p>Jako rozwiązanie należy dostarczyć pliki źródłowe oraz plik <code>makefile</code>,
 które należy umieścić jako skompresowane archiwum w Moodle. Archiwum 
powinno zawierać tylko pliki niezbędne do zbudowania programów. Nie 
wolno w nim umieszczać plików binarnych ani pośrednich powstających 
podczas kompilowania programów.</p>
<p>Po rozpakowaniu dostarczonego archiwum, w wyniku wykonania w jego głównym katalogu polecenia <code>make</code>, dla części A zadania ma powstać w tym katalogu plik wykonywalny <code>screen-worms-server</code>, a dla części B zadania – plik wykonywalny <code>screen-worms-client</code>. Ponadto <code>makefile</code> powinien obsługiwać cel <code>clean</code>, który po wywołaniu kasuje wszystkie pliki powstałe podczas kompilowania.</p>
