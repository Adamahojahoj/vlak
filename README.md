# 🚂 ESP32 Train Web Controller

Tento projekt přeměňuje modul **ESP32** na výkonný a plynulý řídicí mozek pro váš model vlaku. Pomocí vlastního Wi-Fi rozhraní a H-Můstku (L298N) můžete z mobilního telefonu přesně regulovat rychlost oběma směry a spínat osvětlení.

---

## 📑 Obsah
1. [Hlavní Funkce](#-hlavní-funkce)
2. [Hardwarové Zapojení (Wiring)](#-hardwarové-zapojení-wiring)
3. [Síť a Wi-Fi Nastavení](#-síť-a-wi-fi-nastavení)
4. [Instalace a Nahrání](#-instalace-a-nahrání)
5. [Jak Ovládat](#-jak-ovládat)

---

## 🌟 Hlavní Funkce

- **Nezávislé Wi-Fi (SoftAP):** Nevyžaduje domácí Wi-Fi router; ESP32 si vytvoří vlastní síť.
- **Rampování Motoru:** Speciální algoritmus (*Ramping*) zabraňuje ničení převodů plynulým vyrovnáváním rychlosti.
- **Webové Ovládání v reálném čase:** Interaktivní responzivní posuvník, který reaguje okamžitě na pohyb prstu s rychlou odezvou přes asynchronní requesty.
- **Podpora světel:** Ovládání předního i zadního světlometu jedním tlačítkem.

---

## 🔌 Hardwarové Zapojení (Wiring)

> **⚠️ KRITICKÉ: Společná Zem (GND)**  
> Pokud napájíte ESP32 z USB (5V) a Motory z baterie (např. 9V-12V přes L298N), **musíte propojit GND na ESP32 s GND napájecí soustavy na L298N.** Bez společné země nebudou řídicí signály fungovat.

### L298N H-Můstek (Připojení k Motorům)
*Z L298N desky sundejte malé plastové propojky (jumpery) na pinech ENA a ENB, ty zrovna použijeme pro posílání pulzů z ESP32.*

| Modul L298N | Pin k ESP32 | K čemu slouží? |
|:---:|:---:|---|
| **ENA** | `14` | Povolení a rychlost (PWM) pro levý okruh **(Motor A)** |
| **IN1** | `27` | Směr otáčení pro Motor A (vpřed/vzad) |
| **IN2** | `26` | Směr otáčení pro Motor A (vpřed/vzad) |
| **ENB** | `32` | Povolení a rychlost (PWM) pro pravý okruh **(Motor B)** |
| **IN3** | `33` | Směr otáčení pro Motor B (vpřed/vzad) |
| **IN4** | `25` | Směr otáčení pro Motor B (vpřed/vzad) |

### Světlomety (LED)
*Pro bezpečný chod u 3.3V logiky ESP32 použijte **rezistor** (150Ω - 330Ω) zapájený do série k LED, aby dioda nebo pin neshořely.*

| Dioda (LED) | Pin (+ Anoda) | Mínus (- Katoda) | Popis |
|---|:---:|:---:|---|
| **LED 1 (Přední)** | `12` | Přes rezistor do `GND` | Hlavní světlomet po směru jízdy |
| **LED 2 (Zadní)** | `13` | Přes rezistor do `GND` | Koncové obrysové/doplňkové světlo |

---

## 🛜 Síť a Wi-Fi Nastavení

- **Název vysílané sítě (SSID):** `Vlak_Ovladani`
- **Heslo:** `12345678`
- **IP adresa ovládacího panelu:** `192.168.4.1`

Pokud plánujete zapojit hračku z domu a ovládat skrze běžný domácí router, změňte ve složce `vlak_esp32.ino` konfiguraci:
```cpp
// Nahradit WiFi.softAP(jmenoWifi, hesloWifi); tímto:
WiFi.begin("TvojeDomaciSít", "TajneHeslo");
```

---

## 🚀 Instalace a Nahrání

Vše je připraveno k okamžitému přesunu do ESP32 přes **Arduino IDE**. Není potřeba žádné složité přidávání externích knihoven.

1. Propojte ESP32 s počítačem přes USB kabel.
2. V Arduino IDE vyberte připojený port a desku (např. *DOIT ESP32 DEVKIT V1*).
3. Otevřete zdroják projektu `vlak_esp32.ino`.
4. Klikněte vlevo nahoře na tlačítko **Nahrát / Upload**.
5. Otevřete *Sériový Monitor* (nastavte baud rate dole na `115200`). Až uvidíte hlášku `Vlak je pripraven!`, vše úspěšně naběhlo. 

---

## 📱 Jak Ovládat

1. Připojte z mobilu Vaši Wi-Fi k vytvořené síti **`Vlak_Ovladani`**.
2. Zapněte internetový prohlížeč a vyhledejte adresu **`192.168.4.1`**.
3. Otevře se Vám tmavý palubní displej. 
    - Táhněte posuvníkem nad `0%` pro **Jízdu vpřed**.
    - Táhněte pod `0%` pro opačný chod – **Jízdu vzad**.
    - Tlačítkem 🛑 okamžitě nastavíte nulu, ale motor fyzicky brzdí plynulou "Rampou". Typická rychlost pro Rampu lze upravit v kódu.
    - Osvětlení 💡 přepíná logiku LED 1 a 2 nezávisle na motorech.
