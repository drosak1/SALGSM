## Repozytorium Gitea

<p align="center">
  <img src="GSMSALv1_.png" width="400"/>
</p>

AT+IPR=9600 -> ustawia transmisje na GSM

Creating a new repository on the command line

Existing repositories

git remote remove origin

git config --global credential.helper store

git remote add origin https://dlb.blue/drosak1/SALGSM.git

git remote set-url origin https://[USER]:[TOKEN]@dlb.blue/drosak/SALGSM.git

git push --force origin main


## Github

git init & add files & commit

git remote add origin https://github.com/drosak1/SALGSM.git

git pull origin main --allow-unrelated-histories

https://github.com/settings/tokens -> “Personal access tokens (classic)”

 repo [x] workflow [x] read:org

Username for 'https://github.com': drosak1

Password for 'https://github.com':

git push --force origin main


## Zwiększenie buffora gita

git config --global core.sshCommand "ssh -o Compression=no -o TCPKeepAlive=yes -o ServerAliveInterval=30 -o ServerAliveCountMax=10"

git config --global http.postBuffer 524288000

## Autor
Dawid Rosak

## ATmega4809 / MegaCoreX

Kod na galezi `ATMEGA4809` jest przeznaczony dla ukladu U206 ze schematu
`doc/Schemat_elektryczny.pdf` i pinoutu `48 pin standard` w MegaCoreX.

Zalecane ustawienia Arduino IDE:

- Board: `ATmega4809`
- Pinout: `48 pin standard`
- Clock: `Internal 16 MHz`
- BOD: `2.9V`
- EEPROM: `EEPROM retained`
- Reset pin: `Reset`
- Bootloader: `No bootloader`
- Programowanie: programator zgodny z UPDI

Polaczenia wykorzystywane przez program:

| Funkcja | Peryferium/pin ATmega4809 | Oznaczenie na schemacie |
| --- | --- | --- |
| Konsola CH340 | USART1: PC0/PC1 | RXCH340/TX1ATMEGA, TXCH340/RX1ATMEGA |
| SIM800L | USART3: PB0/PB1 | RXGSM/TX3ATMEGA, TXGSM/RX3ATMEGA |
| Reset SIM800L | PB2 | RSTGSM |
| Zasilanie SIM800L | PC3 | ENABLEGSM |
| Licznik wody | PA3 | IMPWATER |
| Licznik gazu (rezerwa) | PA4 | IMPGAS |

Konsola i modem pracuja z predkoscia 9600 baud. Program korzysta ze
sprzetowego USART3, dlatego biblioteka `NeoSWSerial` nie jest potrzebna.



AT

OK

RDY

+CFUN: 1

+CPIN: READY

Call Ready

SMS Ready
AT

OK

AT+CIMI

901405180011350

OK


AT+CGATT=1

OK

AT+CSTT="internet"

OK
AT+SAPBR=3,1,"CONTYPE","GPRS"

OK
AT+SAPBR=3,1,"APN","sensor.net"

OK
AT+SAPBR=1,1

OK
AT+SAPBR=2,1

+SAPBR: 1,1,"10.0.0.1"

OK


AT+HTTPINIT

OK

AT+HTTPPARA="CID",1

OK

AT+HTTPPARA="URL","http://dlb.com.pl/api/v1/telemetry.php?ID=901405180011350&KEY=9999&payload=xxyy"

OK

AT+HTTPACTION=0

OK

+HTTPACTION: 0,200,65


AT+HTTPREAD=0,33

+HTTPREAD: 33
﻿25.11.22 22:50:00;ID;901405180
OK
AT+HTTPREAD=33,66

+HTTPREAD: 32
011350;KEY;;payload;xxyy;error;;
OK

AT+HTTPTERM

OK
AT+HTTPINIT

OK
AT+HTTPPARA="CID",1

OK
AT+HTTPPARA="URL","http://dlb.com.pl/api/v1/telemetry.php?ID=901405180011350&KEY=9999&payload=xxyy"

OK
AT+HTTPPARA="http://dlb.com.pl/api/v1/telemetry.php?ID=123456&KEY=9999&phone=48609105069&sms=GSMTEST"

ERROR
AT+HTTPPARA="URL","http://dlb.com.pl/api/v1/telemetry.php?ID=123456&KEY=9999&phone=48609105069&sms=GSMTEST"

OK
AT+HTTPACTION=0

OK

+HTTPACTION: 0,200,12






Twój przypadek (idealne użycie)

Masz:

for(uint8_t i = 0 ;i < head[1];i++){
  str += char(data[i]);
}

👉 tutaj powinieneś zrobić:

String str;
str.reserve(head[1]);  // 🔥 kluczowe

for(uint8_t i = 0 ;i < head[1];i++){
  str += char(data[i]);
}
⚠️ Dlaczego to ważne

Bez reserve():

wiele malloc i free
RAM się „dziurawi”
po czasie → dziwne błędy / reset MCU

Z reserve():

jedna alokacja
stabilne działanie
❗ Ograniczenia
jeśli przekroczysz rozmiar → i tak będzie realloc
dalej używasz heap → nie jest to 100% safe
