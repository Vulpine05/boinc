#Czech language.ini.cs file
#Version 1.09a by Spok (2003/07/31 - 09:32:48 CET :)
#Character set: "charset=Windows-1250"
 
# Use & for menu keys

#PROJECT_ID
[HEADER-Projects]
Title=Projekty
Project=Projekt
Account=��et
Total Credit=Celkov� kredit
Avg. Credit=Pr�m�rn� kredit
Resource Share=Rozd�len� pr�ce

#RESULT_ID
[HEADER-Work]
Title=�koly
Project=Projekt
Application=Aplikace
Name=Jm�no
CPU time=Procesorov� �as
Progress=Pr�b�h
To Completion=Dokon�� za
Status=Stav

#XFER_ID
[HEADER-Transfers]
Title=P�enos
Project=Projekt
File=Soubor
Progress=Pr�b�h
Size=Velikost
Time=�as
Direction=Sm�r
Speed=Rychlost
Status=Stav
Type=Typ

#MESSAGE_ID
[HEADER-Messages]
Title=Zpr�vy
Project=Projekt
Time=�as
Message=Zpr�va

#USAGE_ID
[HEADER-Disk]
Title=Disk
#Free space: not available for use=Voln� m�sto: Nevyu�iteln�
#Free space: available for use=Voln� m�sto: Vyu�iteln�
#Used space: other than BOINC=Obsazen� m�sto: Mimo BOINC
#Used space:=Obsazen� m�sto:
Free space=Voln� m�sto
Used space: non-BOINC=Obsazen� m�sto: mimo BOINC
Used space: BOINC=Obsazen� m�sto: BOINC

#miscellaneous text
[HEADER-MISC]
New=Nov�
Running=Prob�haj�c�
Ready to run=P�ipraven�
Computation done=V�po�et ukon�en
Results uploaded=Odesl�no
Acknowledged=Potvrzeno
Error: invalid state=Chyba: Neplatn� stav
Completed=Dokon�eno
Upload=Odes�l� se
Download=P�ij�m� se
Retry in=Znovu zkus�m za
Upload failed=Chyba p�i odes�l�n�
Download failed=Chyba p�i stahov�n�
Uploading=Odes�l� se (*)
Downloading=P�ij�m� se (*)


#menu items
# NOTE: add an & (ampersand) to the letter to be used as mnemonic
#       i.e. Show Graphics=Show &Graphics
#                               ^^ the "G" will trigger the menu item
#       you can compare it with a saved language.ini.XX file
[MENU-File]
Title=&Soubor
Show Graphics=Zobraz &Grafiku
Force run=Spus� &V�po�et
Run based on preferences=Po��tej podle &Nastaven�
Pause=&Pauza
Run Benchmarks=Spus� &Testy
Hide=&Skryj
Exit=&Konec
Suspend=Pozastavit (*)
Resume=Obnovit (*)


[MENU-Settings]
Title=&Nastaven�
Login to Project...=&P�ihl�sit se k projektu...
Proxy Server...=Proxy &Server...

[MENU-Help]
Title=&Pomoc
About...=&O programu...

[MENU-StatusIcon]
Suspend=Pozastavit
Resume=Obnovit
Exit=Konec

[MENU-Project]
Relogin...=Znovu p�ihl�sit...
Get preferences=Z�skej nastaven�
Retry now=Zkus te�
Detach...=Opustit projekt...
Reset project...=Restartovat projekt...
Quit Project...=Ukon�it projekt...

[MENU-Work]
Show Graphics=Zobraz &grafiku

[MENU-Transfers]
Retry now=Zkus te�

[MENU-Messages]
Copy to clipboard=Zkop�ruj do schr�nky

[DIALOG-LOGIN]
Title=P�ihl�sit se k projektu
URL:=URL:
Account Key:=Kl�� (Account Key):
OK=OK
Cancel=Zru�it
The URL for the website of the project.=URL projektu.
The authorization code recieved in your confirmation email.=Autoriza�n� kl��
obdr�en� v potvrzovac�m e-mailu.

[DIALOG-QUIT]
Title=Ukon�it
URL:=URL:
Account Key:=Kl�� (Account Key):
OK=OK
Cancel=Zru�it
Select the project you wish to quit.=Vyberte projekt, kter� chcete ukon�it.

[DIALOG-CONNECT]
Title=P�ipojit
BOINC needs to connect to the network.  May it do so now?=BOINC vy�aduje
p�ipojen� na internet. P�ipojit?
Don't ask this again (connect automatically)=P��t� se nedotazovat
(p�ipojovat automaticky).
OK=OK
Cancel=Zru�it

[DIALOG-ABOUT]
Title=BOINC - Beta verze
Berkeley Open Infrastructure for Network Computing=Berkeley Open
Infrastructure for Network Computing
Open Beta=Open Beta
OK=OK

[DIALOG-PROXY]
Title=Nastaven� proxy serveru
Some organizations use an "HTTP proxy" or a "SOCKS proxy" (or both) for
increased security.  If you need to use a proxy, fill in the information
below.  If you need help, ask your System Administrator or Internet Service
Provider.=N�kter� organizace pou��vaj� HTTP nebo SOCKS proxy server
(p��padn� oba), kv�li zv��en� bezpe�nosti. Pokud se p�ipojujete p�es proxy
server, vypl�te tento formul��. Pot�ebujete-li pomoc, kontaktujte va�eho
spr�vce s�t� nebo va�eho poskytovatele p�ipojen�.
HTTP Proxy=HTTP proxy server
Connect via HTTP Proxy Server=P�ipojit p�es HTTP proxy server
http://=http://
Port Number:=��slo portu:
SOCKS Proxy=SOCKS proxy server
Connect via SOCKS Proxy Server=P�ipojit p�es SOCKS proxy server
#SOCKS Host:=Jm�no/IP-adresa SOCKS serveru:
SOCKS Host:=Jm�no/IP:
Port Number:=��slo portu:
Leave these blank if not needed=Nevypl�ujte, pokud nen� vy�adov�no
SOCKS User Name:=U�ivatelsk� jm�no:
SOCKS Password:=Heslo:
OK=OK
Cancel=Zru�it
