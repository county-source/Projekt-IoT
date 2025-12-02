# Chytrý IoT Květináč

**Autor:** Jakub Vana 3F  
**Datum:** 2025-02-17


## Funkce
- **Měření:** ESP32 pravidelně sbírá data ze senzorů – vlhkost půdy, teplotu vzduchu, vlhkost vzduchu a hladinu vody.
- **Zobrazení:** Naměřená data se zobrazují na OLED displeji.


## Sestava

- **ESP32 Development Board** – hlavní mikrokontrolér, který řídí všechny funkce.
- **Kapacitní senzor vlhkosti půdy** – měří aktuální vlhkost půdy.
- **Teplotní senzor** – monitoruje teplotu vzduchu.
- **OLED displej (I2C)** – zobrazuje naměřená data.
- **Senzor hladiny vody** – kontroluje stav vodní hladiny v rezervoáru.
- **Vodní rezervoár** – zásobník pro zavlažování, který je vybaven odtokovým systémem.
- **3D tisknutý květináč** – navržený s dutinou pro umístění elektroniky a skrytí FSR.
