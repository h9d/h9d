# h9d JSON-RPC API

h9d udostępnia API w postaci JSON-RPC 2.0 po TCP. Klient łączy się na skonfigurowany port TCP i wymienia wiadomości JSON zakończone znakiem nowej linii (`\n`). Każde żądanie musi być zgodne ze specyfikacją JSON-RPC 2.0.

## Protokół komunikacji

### Format żądania

```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "nazwa_metody",
  "params": { ... }
}
```

### Format odpowiedzi (sukces)

```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": { ... }
}
```

### Format odpowiedzi (błąd)

```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "error": {
    "code": -11,
    "message": "Node 5 does not exist."
  }
}
```

### Notyfikacje (push od serwera)

Po zasubskrybowaniu zdarzeń serwer wysyła asynchroniczne notyfikacje (bez `id`, bez `result`):

```json
{
  "jsonrpc": "2.0",
  "method": "on_frame",
  "params": { ... }
}
```

### Uwierzytelnienie

Przed wywołaniem jakiejkolwiek metody (z wyjątkiem `authenticate`) klient **musi** najpierw wywołać `authenticate`. Próba wywołania innej metody bez uwierzytelnienia zwróci błąd `Method not found` (-32601).

---

## Kody błędów

### Błędy aplikacji h9d

| Kod | Stała | Opis |
|-----|-------|------|
| `-10` | `EXECUTION_TIMEOUT` | Operacja na węźle przekroczyła limit czasu oczekiwania na odpowiedź |
| `-11` | `NODE_IS_NOT_EXIST` | Węzeł o podanym `node_id` nie istnieje w systemie |
| `-12` | `DEV_IS_NOT_EXIST` | Urządzenie wirtualne o podanej nazwie nie istnieje |
| `-13` | `MALFORMED_FRAME` | Ramka CAN ma niepoprawną strukturę |
| `-14` | `FRAME_SIZE_MISMATCH` | Rozmiar danych ramki nie zgadza się z oczekiwanym |
| `-15` | `REGISTER_IS_NOT_EXIST` | Rejestr o podanym numerze nie istnieje w tym węźle |
| `-16` | `REGISTER_IS_NOT_WRITABLE` | Rejestr jest tylko do odczytu |
| `-17` | `REGISTER_IS_NOT_READABLE` | Rejestr jest tylko do zapisu |
| `-18` | `UNSUPPORTED_REGISTER_DATA_CONVERSION` | Nie można przekonwertować podanej wartości na typ rejestru |
| `-1000` do `-1255` | — | Kod błędu zwrócony bezpośrednio przez węzeł w ramce `COMMAND_ERROR` (numer węzła zakodowany w zakresie) |

### Standardowe błędy JSON-RPC

| Kod | Opis |
|-----|------|
| `-32700` | Parse error — nieprawidłowy JSON |
| `-32600` | Invalid request — brakuje wymaganego pola JSON-RPC |
| `-32601` | Method not found — metoda nie istnieje lub klient nie jest uwierzytelniony |
| `-32602` | Invalid params — brakuje wymaganego parametru lub ma zły typ |

---

## Obiekt H9Frame

Obiekt ramki CAN używany w metodach `send_frame` i notyfikacji `on_frame`.

### Pola

| Pole | Typ | Opis |
|------|-----|------|
| `type` | integer | Typ ramki (wartość 0–31, patrz tabela typów poniżej) |
| `source_id` | integer | ID węzła źródłowego (0–255) |
| `destination_id` | integer | ID węzła docelowego (0–255) — tylko w ramkach unicast |
| `broadcast_group` | integer | Numer grupy broadcastowej — tylko w ramkach broadcast; `65535` (`0xFFFF`) = wszystkie węzły |
| `flags` | integer | Flagi ramki (0 = pojedyncza ramka, 1 = pierwsza z wielu, 2 = środkowa, 3 = ostatnia) |
| `seqnum` | integer | Numer sekwencyjny (0–31), przydzielany przez h9d przy wysyłaniu |
| `dlc` | integer | Data Length Code — liczba bajtów danych (0–8) |
| `data` | array of integers | Bajty danych, tablica liczb 0–255, długość = `dlc` |
| `origin` | string | Identyfikator klienta który wysłał ramkę (wypełniany przez h9d, ignorowany przy odbiorze) |

Ramka jest albo **unicast** (ma `destination_id`) albo **broadcast** (ma `broadcast_group`) — nigdy oba naraz.

### Typy ramek (`type`)

| Wartość | Nazwa | Opis |
|---------|-------|------|
| 8 | `COMMAND_ERROR` | Odpowiedź węzła — błąd wykonania polecenia |
| 9 | `REG_VALUE` | Odpowiedź węzła — wartość rejestru (unicast) |
| 10 | `SET_REG` | Żądanie ustawienia wartości rejestru |
| 11 | `GET_REG` | Żądanie odczytania wartości rejestru |
| 12 | `SET_BIT` | Żądanie ustawienia bitu w rejestrze |
| 13 | `CLEAR_BIT` | Żądanie wyzerowania bitu w rejestrze |
| 14 | `NODE_UPGRADE` | Polecenie aktualizacji firmware |
| 15 | `NODE_RESET` | Polecenie resetu węzła |
| 16 | `DISCOVER` | Broadcast — wykrywanie węzłów |
| 17 | `GROUP_RESET` | Broadcast — reset grupy węzłów |
| 18 | `NODE_FAULT` | Broadcast — węzeł zgłasza błąd |
| 19 | `REG_VALUE_BROADCAST` | Broadcast — wartość rejestru (np. heartbeat z wartością) |
| 20 | `NODE_HEARTBEAT` | Broadcast — sygnał życia węzła |
| 21 | `NODE_INFO` | Broadcast — informacje o węźle (odpowiedź na DISCOVER) |
| 22 | `NODE_TURNED_ON` | Broadcast — węzeł właśnie się uruchomił |
| 23 | `BOOTLOADER_TURNED_ON` | Broadcast — bootloader węzła aktywny |
| 24–31 | `NODE_SPECIFIC_BROADCAST0`–`7` | Broadcasty specyficzne dla urządzenia |
| 1–7 | Bootloader | Typy używane podczas aktualizacji firmware |

### Przykład — ramka unicast

```json
{
  "type": 11,
  "source_id": 0,
  "destination_id": 5,
  "flags": 0,
  "seqnum": 0,
  "dlc": 1,
  "data": [10]
}
```
Powyższy przykład to `GET_REG` wysłany do węzła 5, prośba o odczyt rejestru nr 10.

### Przykład — ramka broadcast

```json
{
  "type": 16,
  "source_id": 0,
  "broadcast_group": 65535,
  "dlc": 0,
  "data": []
}
```
Powyższy przykład to `DISCOVER` wysłany do wszystkich węzłów.

---

## Metody

### `authenticate`

Uwierzytelnia klienta i rejestruje go pod podaną nazwą encji. Musi być pierwszym wywołaniem po połączeniu.

**Parametry:**

| Nazwa | Typ | Wymagany | Opis |
|-------|-----|----------|------|
| `entity` | string | tak | Nazwa identyfikująca klienta, widoczna w liście połączonych klientów (`get_tcp_clients`). Może być dowolnym ciągiem — np. `"my-app"`, `"h9console"`. |

**Wynik:**

```json
{ "authentication": true }
```

**Przykład:**

```json
→ {"jsonrpc":"2.0","id":1,"method":"authenticate","params":{"entity":"my-app"}}
← {"jsonrpc":"2.0","id":1,"result":{"authentication":true}}
```

---

### `get_version`

Zwraca wersję działającego demona h9d.

**Parametry:** brak

**Wynik:**

| Pole | Typ | Opis |
|------|-----|------|
| `version` | string | Pełna wersja jako string, np. `"1.2.3"` |
| `major` | integer | Wersja major |
| `minor` | integer | Wersja minor |
| `patch` | integer | Wersja patch |

**Przykład:**

```json
→ {"jsonrpc":"2.0","id":2,"method":"get_version","params":{}}
← {"jsonrpc":"2.0","id":2,"result":{"version":"1.2.3","major":1,"minor":2,"patch":3}}
```

---

### `get_methods_list`

Zwraca listę nazw wszystkich dostępnych metod API.

**Parametry:** brak

**Wynik:**

| Pole | Typ | Opis |
|------|-----|------|
| `methods_list` | array of strings | Lista nazw metod |

**Przykład:**

```json
→ {"jsonrpc":"2.0","id":3,"method":"get_methods_list","params":{}}
← {"jsonrpc":"2.0","id":3,"result":{"methods_list":["get_version","authenticate",...]}}
```

---

### `subscribe`

Subskrybuje klienta do strumienia zdarzeń. Po subskrypcji serwer wysyła notyfikacje push bez żądania klienta.

**Parametry:**

| Nazwa | Typ | Wymagany | Opis |
|-------|-----|----------|------|
| `event` | string | tak | Nazwa zdarzenia: `"frame"` lub `"dev_status"` |

- `"frame"` — klient będzie otrzymywał notyfikację `on_frame` dla każdej ramki CAN pojawiającej się na magistrali
- `"dev_status"` — klient będzie otrzymywał notyfikację o zmianie stanu urządzeń wirtualnych (devs)

**Wynik:**

```json
{ "return": "ok" }
```

**Przykład:**

```json
→ {"jsonrpc":"2.0","id":4,"method":"subscribe","params":{"event":"frame"}}
← {"jsonrpc":"2.0","id":4,"result":{"return":"ok"}}
```

Po tym serwer zacznie wysyłać notyfikacje (patrz sekcja [Notyfikacje](#notyfikacje)).

---

### `get_tcp_clients`

Zwraca listę aktualnie połączonych klientów TCP.

**Parametry:** brak

**Wynik:** tablica obiektów, każdy opisuje jedno połączenie:

| Pole | Typ | Opis |
|------|-----|------|
| `id` | string | Unikalny identyfikator połączenia nadany przez h9d |
| `entity` | string | Nazwa encji podana podczas `authenticate` (pusty string jeśli niezautentykowany) |
| `connection_time` | string | Czas nawiązania połączenia w formacie ISO 8601 UTC, np. `"2024-01-15T12:00:00Z"` |
| `authenticated` | boolean | Czy klient wywołał `authenticate` |
| `remote_address` | string | Adres IP klienta |
| `remote_port` | integer | Port TCP klienta |
| `frame_subscription` | boolean | Czy klient subskrybuje zdarzenia `frame` |
| `dev_subscription` | boolean | Czy klient subskrybuje zdarzenia `dev_status` |

**Przykład:**

```json
← {
  "result": [
    {
      "id": "h9console@192.168.1.10:54321",
      "entity": "h9console",
      "connection_time": "2024-01-15T12:00:00Z",
      "authenticated": true,
      "remote_address": "192.168.1.10",
      "remote_port": 54321,
      "frame_subscription": true,
      "dev_subscription": false
    }
  ]
}
```

---

### `send_frame`

Wysyła ramkę CAN na magistralę. Pole `origin` ramki jest zawsze ustawiane przez h9d na identyfikator klienta.

**Parametry:**

| Nazwa | Typ | Wymagany | Opis |
|-------|-----|----------|------|
| `frame` | object | tak | Obiekt H9Frame do wysłania (patrz sekcja [Obiekt H9Frame](#obiekt-h9frame)) |
| `raw` | boolean | nie | `true` = wyślij ramkę dokładnie jak podano, bez dodatkowej walidacji; `false` (domyślnie) = h9d może zmodyfikować niektóre pola |

**Wynik:**

| Pole | Typ | Opis |
|------|-----|------|
| `seqnum` | integer | Numer sekwencyjny przydzielony wysłanej ramce (0–31) |

**Przykład — wysłanie GET_REG do węzła 5, rejestr 10:**

```json
→ {
  "jsonrpc": "2.0",
  "id": 5,
  "method": "send_frame",
  "params": {
    "frame": {
      "type": 11,
      "source_id": 0,
      "destination_id": 5,
      "flags": 0,
      "dlc": 1,
      "data": [10]
    }
  }
}
← {"jsonrpc":"2.0","id":5,"result":{"seqnum":3}}
```

---

### `get_stats`

Zwraca statystyki i metryki demona h9d (liczniki ramek, błędów itp.).

**Parametry:** brak

**Wynik:** obiekt z metrykami (zawartość zależy od konfiguracji zbieranych metryk).

---

### `reload_nodes_description`

Przeładowuje pliki opisów węzłów (`.conf`) z dysku bez restartu demona. Przydatne po dodaniu/zmianie opisu węzła.

**Parametry:** brak

**Wynik:** `true`

---

### `get_nodes_list`

Zwraca listę węzłów znanych h9d (które zgłosiły swoją obecność na magistrali).

**Parametry:** brak

**Wynik:** tablica obiektów:

| Pole | Typ | Opis |
|------|-----|------|
| `id` | integer | ID węzła (0–255) |
| `type` | integer | Typ węzła (numer z pliku opisów) |
| `name` | string | Nazwa węzła z pliku opisów (pusty string jeśli brak opisu) |

**Przykład:**

```json
← {"result": [
  {"id": 5, "type": 1234, "name": "ATU controller"},
  {"id": 7, "type": 1235, "name": "Power switch"}
]}
```

---

### `get_node_info`

Zwraca szczegółowe informacje o konkretnym węźle.

**Parametry:**

| Nazwa | Typ | Wymagany | Opis |
|-------|-----|----------|------|
| `node_id` | integer | tak | ID węzła (0–255) |

**Wynik:**

| Pole | Typ | Opis |
|------|-----|------|
| `id` | integer | ID węzła |
| `type` | integer | Typ węzła |
| `version_major` | integer | Wersja firmware — major |
| `version_minor` | integer | Wersja firmware — minor |
| `hardware_revision` | string | Rewizja sprzętowa |
| `reset_reason` | integer | Powód ostatniego resetu |
| `name` | string | Nazwa węzła z pliku opisów |
| `description` | string | Opis węzła z pliku opisów |
| `created_time` | string | Czas pierwszego wykrycia węzła (ISO 8601 UTC) |
| `last_seen_time` | string | Czas ostatniej aktywności węzła (ISO 8601 UTC) |

**Błędy:** `NODE_IS_NOT_EXIST` (-11)

**Przykład:**

```json
→ {"jsonrpc":"2.0","id":6,"method":"get_node_info","params":{"node_id":5}}
← {
  "result": {
    "id": 5,
    "type": 1234,
    "version_major": 1,
    "version_minor": 3,
    "hardware_revision": "B",
    "reset_reason": 0,
    "name": "ATU controller",
    "description": "Automatic antenna tuner",
    "created_time": "2024-01-10T08:00:00Z",
    "last_seen_time": "2024-01-15T12:30:00Z"
  }
}
```

---

### `discover_nodes`

Wysyła ramkę `DISCOVER` (broadcast do wszystkich węzłów). Węzły odpowiadają ramkami `NODE_INFO`, które h9d przechwytuje i aktualizuje wewnętrzną listę węzłów. Wyniki można następnie pobrać metodą `get_nodes_list`.

**Parametry:** brak

**Wynik:** `true`

---

### `node_reset`

Wysyła polecenie resetu do węzła.

**Parametry:**

| Nazwa | Typ | Wymagany | Opis |
|-------|-----|----------|------|
| `node_id` | integer | tak | ID węzła (0–255) |

**Wynik:** `true`

**Błędy:** `NODE_IS_NOT_EXIST` (-11)

---

### `get_registers_list`

Zwraca listę wszystkich rejestrów węzła z ich opisami (z pliku `.conf` opisów węzłów).

**Parametry:**

| Nazwa | Typ | Wymagany | Opis |
|-------|-----|----------|------|
| `node_id` | integer | tak | ID węzła (0–255) |

**Wynik:** tablica obiektów opisujących rejestry:

| Pole | Typ | Opis |
|------|-----|------|
| `number` | integer | Numer rejestru (0–255). Rejestry 0–9 to rejestry standardowe obecne w każdym węźle H9. |
| `name` | string | Nazwa rejestru |
| `type` | string | Typ danych: `"uint"`, `"int"`, `"bool"`, `"char"`, `"str"`, `"float"` |
| `size` | integer | Rozmiar w **bitach** |
| `readable` | boolean | Czy rejestr można odczytać |
| `writable` | boolean | Czy rejestr można zapisać |
| `bits_names` | array of strings | Nazwy poszczególnych bitów (tylko dla `type="bool"` z `size>1`). `bits_names[0]` = bit 0 (LSB), `bits_names[n-1]` = bit n-1 (MSB). |
| `description` | string | Opcjonalny opis rejestru |

**Typy rejestrów:**

| Typ | Opis | Przykładowy rozmiar |
|-----|------|---------------------|
| `uint` | Liczba całkowita bez znaku | 8, 16, 32 bitów |
| `int` | Liczba całkowita ze znakiem | 8, 16, 32 bitów |
| `bool` | Wartość logiczna (1 bit) lub pole bitowe (>1 bit) | 1–48 bitów |
| `char` | Pojedynczy znak ASCII | 8 bitów |
| `str` | Ciąg znaków ASCII | 16–48 bitów |
| `float` | Liczba zmiennoprzecinkowa IEEE 754 | 32 bity |

**Błędy:** `NODE_IS_NOT_EXIST` (-11)

**Przykład:**

```json
→ {"jsonrpc":"2.0","id":7,"method":"get_registers_list","params":{"node_id":5}}
← {
  "result": [
    {
      "number": 0,
      "name": "Node type",
      "type": "uint",
      "size": 16,
      "readable": true,
      "writable": false,
      "bits_names": [],
      "description": ""
    },
    {
      "number": 10,
      "name": "STATUS",
      "type": "bool",
      "size": 10,
      "readable": true,
      "writable": false,
      "bits_names": [
        "DEBUG", "FULL SEARCH", "TUNE", "OLED ENABLE", "OLED DEBUG",
        "SWR SHORTCUT", "MEMORY SHORTCUT", "EXTEND MEMORY SHORTCUT",
        "UNDERPOWER ERROR", "OVERPOWER ERROR"
      ],
      "description": ""
    },
    {
      "number": 11,
      "name": "FREQUENCY",
      "type": "uint",
      "size": 16,
      "readable": true,
      "writable": true,
      "bits_names": [],
      "description": "Operating frequency in kHz"
    }
  ]
}
```

---

### `get_register_value`

Odczytuje aktualną wartość rejestru węzła. h9d wysyła ramkę `GET_REG` do węzła i czeka na odpowiedź `REG_VALUE`.

**Parametry:**

| Nazwa | Typ | Wymagany | Opis |
|-------|-----|----------|------|
| `node_id` | integer | tak | ID węzła (0–255) |
| `reg` | integer | tak | Numer rejestru (0–255) |

**Wynik:** wartość rejestru — typ JSON zależy od typu rejestru:

| Typ rejestru | Typ w JSON | Przykład |
|--------------|------------|---------|
| `uint`, `int`, `bool`, `char` | integer | `42` |
| `float` | number | `3.14` |
| `str` | string | `"hello"` |
| (raw, nieznany typ) | array of integers | `[0, 42, 255]` |

**Błędy:** `NODE_IS_NOT_EXIST` (-11), `REGISTER_IS_NOT_EXIST` (-15), `REGISTER_IS_NOT_READABLE` (-17), `EXECUTION_TIMEOUT` (-10)

**Przykład:**

```json
→ {"jsonrpc":"2.0","id":8,"method":"get_register_value","params":{"node_id":5,"reg":11}}
← {"jsonrpc":"2.0","id":8,"result":14074}
```

```json
→ {"jsonrpc":"2.0","id":9,"method":"get_register_value","params":{"node_id":5,"reg":10}}
← {"jsonrpc":"2.0","id":9,"result":8}
```
(wartość `8` = bity `0b0000001000` = bit 3 ustawiony = `OLED ENABLE`)

---

### `set_register_value`

Zapisuje wartość do rejestru węzła. h9d wysyła ramkę `SET_REG` i czeka na potwierdzenie `REG_VALUE`.

**Parametry:**

| Nazwa | Typ | Wymagany | Opis |
|-------|-----|----------|------|
| `node_id` | integer | tak | ID węzła (0–255) |
| `reg` | integer | tak | Numer rejestru (0–255) |
| `value` | integer \| float \| string \| array | tak | Wartość do zapisania. Typ musi pasować do typu rejestru. |

Obsługiwane typy `value`:
- `integer` lub `unsigned integer` → dla rejestrów `uint`, `int`, `bool`, `char`
- `float` → dla rejestrów `float`
- `string` → dla rejestrów `str`, `char`
- `array of integers` → zapis surowych bajtów

**Wynik:** wartość potwierdzona przez węzeł (ten sam format co `get_register_value`).

**Błędy:** `NODE_IS_NOT_EXIST` (-11), `REGISTER_IS_NOT_EXIST` (-15), `REGISTER_IS_NOT_WRITABLE` (-16), `EXECUTION_TIMEOUT` (-10), `UNSUPPORTED_REGISTER_DATA_CONVERSION` (-18)

**Przykład:**

```json
→ {"jsonrpc":"2.0","id":10,"method":"set_register_value","params":{"node_id":5,"reg":11,"value":14200}}
← {"jsonrpc":"2.0","id":10,"result":14200}
```

---

### `set_register_bit`

Ustawia jeden bit rejestru na `1` bez zmiany pozostałych bitów. h9d wysyła ramkę `SET_BIT`.

**Parametry:**

| Nazwa | Typ | Wymagany | Opis |
|-------|-----|----------|------|
| `node_id` | integer | tak | ID węzła (0–255) |
| `reg` | integer | tak | Numer rejestru (0–255) |
| `bit_num` | integer | tak | Numer bitu do ustawienia. Bit 0 = LSB (odpowiada `bits_names[0]`). |

**Wynik:** zaktualizowana wartość rejestru (ten sam format co `get_register_value`).

**Błędy:** `NODE_IS_NOT_EXIST` (-11), `REGISTER_IS_NOT_EXIST` (-15), `REGISTER_IS_NOT_WRITABLE` (-16), `EXECUTION_TIMEOUT` (-10)

**Przykład — ustawienie bitu `OLED ENABLE` (bit 3) w rejestrze STATUS (10):**

```json
→ {"jsonrpc":"2.0","id":11,"method":"set_register_bit","params":{"node_id":5,"reg":10,"bit_num":3}}
← {"jsonrpc":"2.0","id":11,"result":8}
```

---

### `clear_register_bit`

Zeruje jeden bit rejestru bez zmiany pozostałych bitów. h9d wysyła ramkę `CLEAR_BIT`.

**Parametry:**

| Nazwa | Typ | Wymagany | Opis |
|-------|-----|----------|------|
| `node_id` | integer | tak | ID węzła (0–255) |
| `reg` | integer | tak | Numer rejestru (0–255) |
| `bit_num` | integer | tak | Numer bitu do wyzerowania. Bit 0 = LSB (odpowiada `bits_names[0]`). |

**Wynik:** zaktualizowana wartość rejestru (ten sam format co `get_register_value`).

**Błędy:** `NODE_IS_NOT_EXIST` (-11), `REGISTER_IS_NOT_EXIST` (-15), `REGISTER_IS_NOT_WRITABLE` (-16), `EXECUTION_TIMEOUT` (-10)

**Przykład — wyzerowanie bitu `OLED ENABLE` (bit 3) w rejestrze STATUS (10):**

```json
→ {"jsonrpc":"2.0","id":12,"method":"clear_register_bit","params":{"node_id":5,"reg":10,"bit_num":3}}
← {"jsonrpc":"2.0","id":12,"result":0}
```

---

### `get_devs_list`

Zwraca listę urządzeń wirtualnych (devs) zarządzanych przez h9d. Urządzenia wirtualne to abstrakcje wyższego poziomu zbudowane nad węzłami H9.

**Parametry:** brak

**Wynik:** tablica obiektów:

| Pole | Typ | Opis |
|------|-----|------|
| `name` | string | Unikalna nazwa urządzenia |
| `type` | string | Typ urządzenia |

**Przykład:**

```json
← {"result": [
  {"name": "atu", "type": "atu_controller"},
  {"name": "pwr", "type": "power_switch"}
]}
```

---

### `get_dev_description`

Zwraca opis (metadane, możliwe operacje) urządzenia wirtualnego.

**Parametry:**

| Nazwa | Typ | Wymagany | Opis |
|-------|-----|----------|------|
| `dev_name` | string | tak | Nazwa urządzenia (z listy `get_devs_list`) |

**Wynik:** obiekt specyficzny dla typu urządzenia.

**Błędy:** `DEV_IS_NOT_EXIST` (-12)

---

### `get_dev_status`

Zwraca aktualny stan urządzenia wirtualnego.

**Parametry:**

| Nazwa | Typ | Wymagany | Opis |
|-------|-----|----------|------|
| `dev_name` | string | tak | Nazwa urządzenia (z listy `get_devs_list`) |

**Wynik:** obiekt specyficzny dla typu urządzenia opisujący aktualny stan.

**Błędy:** `DEV_IS_NOT_EXIST` (-12)

---

### `dev_method_call`

Wywołuje metodę na urządzeniu wirtualnym. Pozwala na wykonywanie operacji specyficznych dla urządzenia.

**Parametry:**

| Nazwa | Typ | Wymagany | Opis |
|-------|-----|----------|------|
| `dev_name` | string | tak | Nazwa urządzenia (z listy `get_devs_list`) |
| `method` | string | tak | Nazwa metody urządzenia do wywołania |
| `...` | dowolny | nie | Dodatkowe parametry specyficzne dla danej metody urządzenia |

**Wynik:** obiekt specyficzny dla wywołanej metody.

**Błędy:** `DEV_IS_NOT_EXIST` (-12)

---

## Notyfikacje

Po wywołaniu `subscribe` serwer wysyła asynchroniczne notyfikacje JSON-RPC (bez `id`). Klient powinien obsługiwać je równolegle z odpowiedziami na swoje żądania.

### `on_frame`

Wysyłana dla każdej ramki CAN odebranej z magistrali (wymaga subskrypcji `event: "frame"`).

```json
{
  "jsonrpc": "2.0",
  "method": "on_frame",
  "params": {
    "frame": {
      "origin": "h9console@192.168.1.10:54321",
      "type": 22,
      "source_id": 5,
      "broadcast_group": 65535,
      "dlc": 8,
      "data": [4, 210, 1, 0, 3, 65, 0]
    }
  }
}
```

Pole `origin` wskazuje kto wysłał ramkę: wartość `"bus"` oznacza że ramka pochodzi z fizycznej magistrali CAN; w przeciwnym wypadku jest to identyfikator klienta TCP który ją wysłał.

### `on_dev_status`

Wysyłana gdy zmienia się stan urządzenia wirtualnego (wymaga subskrypcji `event: "dev_status"`). Zawartość `params` jest specyficzna dla typu urządzenia.

---

## Standardowe rejestry H9 (rejestry 0–9)

Każdy węzeł H9 implementuje następujące rejestry standardowe:

| Nr | Nazwa | Typ | Rozmiar | R | W | Opis |
|----|-------|-----|---------|---|---|------|
| 0 | Node type | uint | 16 bit | ✓ | — | Typ węzła (identyfikator producenta/modelu) |
| 1 | Node hardware revision | char | 8 bit | ✓ | — | Rewizja sprzętowa (litera, np. `'B'`) |
| 2 | Node version | uint | 32 bit | ✓ | — | Wersja firmware zakodowana jako uint32 |
| 3 | Build metadata | str | 48 bit | ✓ | — | Metadane buildu (do 6 znaków) |
| 4 | MCU type | uint | 8 bit | ✓ | — | Identyfikator mikrokontrolera |
| 5 | MCU SN | uint | 32 bit | ✓ | — | Numer seryjny mikrokontrolera |
| 6 | Node reset reason | uint | 8 bit | ✓ | — | Powód ostatniego resetu |
| 9 | Node id | uint | 8 bit | ✓ | ✓ | Aktualny ID węzła (możliwy do zmiany) |
