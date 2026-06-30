# H9 Frames

## CAN 2.0B Identifier (29-bit extended)

### Ramka unicast (type 0–15)

| Bity    | 28 – 24 | 23 – 16   | 15 – 13 | 12 – 05        | 04 – 00 |
|---------|:-------:|:---------:|:-------:|:--------------:|:-------:|
|         | type    | source_id | flags   | destination_id | seqnum  |
| Rozmiar | 5 bitów | 8 bitów   | 3 bity  | 8 bitów        | 5 bitów |

### Ramka broadcast (type 16–31)

| Bity    | 28 – 24 | 23 – 16   | 15 – 00         |
|---------|:-------:|:---------:|:---------------:|
|         | type    | source_id | broadcast_group |
| Rozmiar | 5 bitów | 8 bitów   | 16 bitów        |

`broadcast_group = 0xFFFF` oznacza broadcast do wszystkich węzłów.

### Pole flags (tylko unicast)

| Wartość | Nazwa                | Opis |
|---------|----------------------|------|
| 0       | `SINGLE_FRAME`       | Jednoramkowa wiadomość |
| 1       | `MULTI_FRAME_FIRST`  | Pierwsza ramka wiadomości wieloramkowej |
| 2       | `MULTI_FRAME_MIDDLE` | Środkowa ramka wiadomości wieloramkowej |
| 3       | `MULTI_FRAME_LAST`   | Ostatnia ramka wiadomości wieloramkowej |

---

## Typy ramek

| ID  | Typ                       | Unicast/Broadcast  | Seqnum                | Odpowiedź                         |
|-----|---------------------------|--------------------|-----------------------|-----------------------------------|
| 0   | RES1 *(reserved)*         | unicast            | —                     | —                                 |
| 1   | PAGE_START                | unicast            | kolejny numer         | PAGE_FILL_NEXT                    |
| 2   | QUIT_BOOTLOADER           | unicast            | kolejny numer         | NODE_TURNED_ON                    |
| 3   | PAGE_FILL                 | unicast            | kolejny numer         | PAGE_WRITED / PAGE_FILL_NEXT      |
| 4   | RES2 *(reserved)*         | unicast            | —                     | —                                 |
| 5   | PAGE_FILL_NEXT            | unicast            | taki sam jak zapytanie | PAGE_FILL                        |
| 6   | PAGE_WRITED               | unicast            | taki sam jak zapytanie | —                                |
| 7   | PAGE_FILL_BREAK           | unicast            | taki sam jak zapytanie | —                                |
| 8   | COMMAND_ERROR             | unicast            | taki sam jak zapytanie | —                                |
| 9   | REG_VALUE                 | unicast            | taki sam jak zapytanie | —                                |
| 10  | SET_REG                   | unicast            | kolejny numer         | REG_VALUE                         |
| 11  | GET_REG                   | unicast            | kolejny numer         | REG_VALUE                         |
| 12  | SET_BIT                   | unicast            | kolejny numer         | REG_VALUE                         |
| 13  | CLEAR_BIT                 | unicast            | kolejny numer         | REG_VALUE                         |
| 14  | NODE_UPGRADE              | unicast            | kolejny numer         | BOOTLOADER_TURNED_ON              |
| 15  | NODE_RESET                | unicast/broadcast  | kolejny numer         | NODE_TURNED_ON                    |
| 16  | DISCOVER                  | broadcast specjalny | kolejny numer        | NODE_INFO                         |
| 17  | GROUP_RESET               | broadcast specjalny | kolejny numer        | —                                 |
| 18  | NODE_FAULT                | broadcast          | zawsze 0              | —                                 |
| 19  | REG_VALUE_BROADCAST       | broadcast          | taki sam jak zapytanie | —                                |
| 20  | NODE_HEARTBEAT            | broadcast          | zawsze 0              | —                                 |
| 21  | NODE_INFO                 | broadcast          | taki sam jak zapytanie | —                                |
| 22  | NODE_TURNED_ON            | broadcast          | zawsze 0              | —                                 |
| 23  | BOOTLOADER_TURNED_ON      | broadcast          | zawsze 0              | PAGE_START                        |
| 24  | NODE_SPECIFIC_BROADCAST0  | broadcast          | kolejny numer         | —                                 |
| 25  | NODE_SPECIFIC_BROADCAST1  | broadcast          | kolejny numer         | —                                 |
| 26  | NODE_SPECIFIC_BROADCAST2  | broadcast          | kolejny numer         | —                                 |
| 27  | NODE_SPECIFIC_BROADCAST3  | broadcast          | kolejny numer         | —                                 |
| 28  | NODE_SPECIFIC_BROADCAST4  | broadcast          | kolejny numer         | —                                 |
| 29  | NODE_SPECIFIC_BROADCAST5  | broadcast          | kolejny numer         | —                                 |
| 30  | NODE_SPECIFIC_BROADCAST6  | broadcast          | kolejny numer         | —                                 |
| 31  | NODE_SPECIFIC_BROADCAST7  | broadcast          | kolejny numer         | —                                 |

---

## Opis typów ramek

### RES1, RES2
Typy zarezerwowane, niezdefiniowane. Nie należy ich używać.

### PAGE_START
Rozpoczęcie procedury programowania strony flash. Wysyłane przez host do węzła w trybie bootloadera.

**Dane:** adres strony (2 bajty big-endian).

### QUIT_BOOTLOADER
Polecenie wyjścia z bootloadera i uruchomienia aplikacji.

**Dane:** brak.

### PAGE_FILL
Wypełnienie bufora strony danymi. Wysyłane przez host po otrzymaniu PAGE_FILL_NEXT.

**Dane:** do 8 bajtów danych firmware.

### PAGE_FILL_NEXT
Żądanie kolejnej porcji danych strony. Odpowiedź bootloadera na PAGE_FILL lub PAGE_START.

**Dane:** brak.

### PAGE_WRITED
Potwierdzenie zapisania strony flash. Odpowiedź bootloadera po zaprogramowaniu pełnej strony.

**Dane:** brak.

### PAGE_FILL_BREAK
Przerwanie wypełniania strony. Wysyłane przez bootloader w przypadku błędu.

**Dane:** brak.

### COMMAND_ERROR
Odpowiedź węzła informująca o błędzie wykonania polecenia. Seqnum równy seqnum ramki wywołującej błąd.

**Dane:**

| Bajt | Opis |
|------|------|
| 0 | Kod błędu (patrz tabela poniżej) |

**Kody błędów węzła:**

| Wartość | Nazwa | Opis |
|---------|-------|------|
| 1 | `INVALID_FRAME` | Nieprawidłowa ramka |
| 2 | `BOOTLOADER_UNSUPPORTED` | Operacja nieobsługiwana przez bootloader |
| 3 | `UNSUPPORTED_OPERATION` | Operacja nieobsługiwana |
| 4 | `UNSUPPORTED_REGISTER` | Rejestr nieobsługiwany przez węzeł |
| 5 | `INVALID_REGISTER` | Nieprawidłowy numer rejestru |
| 6 | `READ_ONLY_REGISTER` | Próba zapisu do rejestru tylko do odczytu |
| 7 | `WRITE_ONLY_REGISTER` | Próba odczytu z rejestru tylko do zapisu |
| 8 | `REGISTER_SIZE_MISMATCH` | Rozmiar danych nie zgadza się z rozmiarem rejestru |

### REG_VALUE
Odpowiedź węzła z wartością rejestru. Wysyłana w odpowiedzi na GET_REG, SET_REG, SET_BIT lub CLEAR_BIT.

**Dane:**

| Bajt | Opis |
|------|------|
| 0 | Numer rejestru |
| 1–N | Wartość rejestru (big-endian, liczba bajtów wynika z rozmiaru rejestru) |

### SET_REG
Żądanie ustawienia wartości rejestru.

**Dane:**

| Bajt | Opis |
|------|------|
| 0 | Numer rejestru |
| 1–N | Nowa wartość rejestru (big-endian) |

### GET_REG
Żądanie odczytu wartości rejestru.

**Dane:**

| Bajt | Opis |
|------|------|
| 0 | Numer rejestru |

### SET_BIT
Żądanie ustawienia jednego bitu w rejestrze na `1` (pozostałe bity bez zmian).

**Dane:**

| Bajt | Opis |
|------|------|
| 0 | Numer rejestru |
| 1 | Numer bitu do ustawienia (0 = LSB) |

### CLEAR_BIT
Żądanie wyzerowania jednego bitu w rejestrze (pozostałe bity bez zmian).

**Dane:**

| Bajt | Opis |
|------|------|
| 0 | Numer rejestru |
| 1 | Numer bitu do wyzerowania (0 = LSB) |

### NODE_UPGRADE
Polecenie wejścia w tryb bootloadera w celu aktualizacji firmware.

**Dane:** brak.

### NODE_RESET
Polecenie restartu węzła. Może być wysłane jako unicast (do konkretnego węzła) lub broadcast (do wszystkich węzłów w grupie).

**Dane:** brak.

### DISCOVER
Broadcast specjalny — żądanie zgłoszenia się wszystkich węzłów. Każdy węzeł odpowiada ramką NODE_INFO.

**Dane:** brak.

### GROUP_RESET
Broadcast specjalny — polecenie restartu wszystkich węzłów w grupie broadcastowej.

**Dane:** brak.

### NODE_FAULT
Broadcast — węzeł zgłasza wystąpienie błędu krytycznego.

**Dane:** zależne od węzła.

### REG_VALUE_BROADCAST
Broadcast — węzeł publikuje wartość rejestru bez żądania (np. zmiana wewnętrzna, pomiar).

**Dane:** takie same jak REG_VALUE (bajt 0 = numer rejestru, bajty 1–N = wartość).

### NODE_HEARTBEAT
Broadcast — sygnał życia węzła, wysyłany cyklicznie.

**Dane:** brak lub zależne od węzła.

### NODE_INFO
Broadcast — węzeł przedstawia się z informacjami o sobie. Wysyłany w odpowiedzi na DISCOVER lub po uruchomieniu.

**Dane:**

| Bajt | Opis |
|------|------|
| 0–1  | Typ węzła (big-endian, uint16) |
| 2–3  | Wersja firmware — major (big-endian, uint16) |
| 4–5  | Wersja firmware — minor (big-endian, uint16) |
| 6    | Litera rewizji sprzętowej (ASCII) |
| 7    | Powód ostatniego resetu (patrz tabela) |

**Kody powodu resetu:**

| Wartość | Opis |
|---------|------|
| 0 | Nieznany |
| 1 | Power-on reset |
| 2 | Watchdog reset |
| 3 | Brown-out reset |
| 4 | Reset zewnętrzny |

### NODE_TURNED_ON
Broadcast — węzeł właśnie uruchomił aplikację (po resecie lub wyjściu z bootloadera). Dane identyczne jak NODE_INFO.

### BOOTLOADER_TURNED_ON
Broadcast — bootloader węzła aktywny i gotowy na programowanie.

**Dane:**

| Bajt | Opis |
|------|------|
| 0–1  | Typ węzła (big-endian, uint16) |
| 2–3  | Wersja bootloadera — major (big-endian, uint16) |
| 4–5  | Wersja bootloadera — minor (big-endian, uint16) |
| 6    | Typ mikrokontrolera |
| 7    | Prędkość zegara MCU |

**Typy mikrokontrolerów:**

| Wartość | MCU |
|---------|-----|
| 1 | ATmega16M1 |
| 2 | ATmega32M1 |
| 3 | ATmega64M1 |
| 4 | ATmega16C1 |
| 5 | ATmega32C1 |
| 6 | ATmega64C1 |
| 7 | AT90CAN128 |
| 8 | PIC18F46K80 |

**Prędkości zegara:**

| Wartość | Częstotliwość |
|---------|---------------|
| 1 | 2 MHz |
| 2 | 4 MHz |
| 3 | 6 MHz |
| 4 | 8 MHz |
| 5 | 12 MHz |
| 6 | 16 MHz |

### NODE_SPECIFIC_BROADCAST0 – NODE_SPECIFIC_BROADCAST7
Broadcasty specyficzne dla aplikacji i typu urządzenia. Ich semantyka jest definiowana przez producenta węzła.
