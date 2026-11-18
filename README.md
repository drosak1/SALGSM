## Repozytorium Gitea

<p align="center">
  <img src="GSMSALv1_.png" width="400"/>
</p>


## Wykorzystanie pamięci ATmega328PB

### Aktualne wykorzystanie pamięci

| Pamięć | Wykorzystane | Zajętość | Wolne |
|---|---:|---:|---:|
| Flash | 12 028 / 32 768 B | 36,7% | 20 740 B |
| SRAM | 953 / 2 048 B | 46,5% | 1 095 B |

**Flash — pamięć programu**

![Flash](https://img.shields.io/badge/Flash-36.7%25-brightgreen)

**SRAM — pamięć operacyjna**

![SRAM](https://img.shields.io/badge/SRAM-46.5%25-brightgreen)

### Porównanie trzech kompilacji

| Parametr | Pierwsza | Druga | Aktualna |
|---|---:|---:|---:|
| Flash | 11 668 B | 10 148 B | 12 028 B |
| SRAM | 1 517 B | 843 B | 953 B |
| Wolny SRAM | 531 B | 1 205 B | 1 095 B |

### Zmiany względem poprzedniej wersji

- Flash zwiększył się o **1 880 B**.
- SRAM zwiększył się o **110 B**.
- Nadal pozostaje ponad **1 KB wolnej pamięci SRAM**.

### Ocena wykorzystania zasobów

Program zajmuje mniej niż 40% pamięci Flash i mniej
niż 50% pamięci SRAM mikrokontrolera ATmega328PB.

Pozostała pamięć SRAM (1 095 B) jest dostępna dla
stosu, zmiennych lokalnych i operacji wykonywanych
podczas pracy programu.

**Status:** Wykorzystanie pamięci jest odpowiednie
do dalszego rozwoju i testowania biblioteki SALGSMv1.

### warto znac

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
