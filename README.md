---
section: 42
x-masysma-name: dcf77vfd_raspi_clock
title: Ma_Sys.ma DCF77 VFD Raspi Clock
date: 2023/12/17 00:09:58
lang: de-DE
author: ["Linux-Fan, Ma_Sys.ma (Ma_Sys.ma@web.de)"]
keywords: ["dcf77", "uhr", "rp2040"]
x-masysma-version: 1.0.0
x-masysma-website: https://masysma.net/...
x-masysma-repository: https://www.github.com/m7a/bo-dcf77vfd-raspi-clock
x-masysma-owned: 1
x-masysma-copyright: (c) 2018-2023 Ma_Sys.ma <info@masysma.net>.
---
Übersicht
=========

Die _Ma_Sys.ma DCF77 VFD Raspi Clock_ ist eine selbst gebaute Digitalfunkuhr auf
Basis eines Vakuum-Floureszenzdisplays, welche durch einen Raspberry Pi Pico
(RP2040 Mikrocontroller) angesteuert wird und ihre Uhrzeit mittels eines
DCF77-Antennenmoduls empfängt.

Designziel der Uhr war es, möglichst viel Fehlerkorrektur bei der Auswertung der
DCF77-Signale einzubauen, sodass eine Uhrzeitsynchronisation trotz billiger
Antenne auch bei schlechten Empfangsbedingungen möglich ist. Weiterhin ist der
DCF77-Empfang kontinuierlich aktiv, sodass der Wechsel der Sekunden in der
Anzeige mit dem Empfang der Signalpulse synchron erfolgt.

_TODO PIC_

Hardwaredesign
==============

Im Laufe der Entwicklung wurden verschiedene Hardwarevarianten durchgespielt
und dann jeweils wieder verworfen.

## Genereller Aufbau

 * Externes 5V Steckernetzteil
 * Externe Antenne
 * 4 Knöpfe zur Steuerung -- 3 Taster (_Modus, Zurück, Weiter_) und ein
   Druckschalter (Wecker Ein/Aus)
 * 5V VFD SPI-Displaymodul 128x64px
 * Buzzer als Weckalarm
 * Lichtsensor zur automatischen Helligkeitsanpassung

## Komponentenwahl

Bei den Komponenten wurde zuerst das Display festgelegt. Aus ästhetischen
Gründen wurde ein VFD gewählt. Das Displaymodul ermöglicht eine freie
Aufteilung des Bildschirminhalts und kommt dank SPI-Interface mit einer
überschaubaren Anzahl an Leitungen zur Ansteuerung aus.

Anfänglich wurde ein Arduino Nano v3 eingesetzt, allerdings stellte sich
nach langer Entwicklungsarbeit an der Software heraus, dass diese zu groß für
den Chip (und für die AVR-Architektur) geworden war. Daher wurde das Projekt auf
Basis des Raspberry Pi PICO (RP2040) neugestartet. Dadurch wurde eine Umsetzung
von den 3.3V IO-Spannung des RP2040 auf die 5V fürs Displaymodul erforderlich.
Typische Wandlerchips dafür scheinen alle nur als SMD-Bauteile verfügbar zu
sein. Daher wurde ein „Hack” mittels eines unidirektionalen Wandlerchips
implementiert.
_TODO WHICH CHIP EXACTLY_

Beim Gehäuse wurde anfänglich eine quadratische Version genutzt, in deren
Mitte dann ein großer Drehknauf plaziert worden wäre, mittels dessen man
verschiedene Bildschirme „durchschalten” hätte können. Allerdings war die
mechanische Stabilität zwischen Drehknauf und Achse schlecht und insgesamt die
Drehknauflösung sehr klobig. Daher wurde für die neue Revision auf ein
rechteckiges Gehäuse und statt eines Drehknaufs auf die Drucktaster für die
Steuerung und den Druckschalter für den Wecker gesetzt.

Somit bleibt die Information “Wecker gestellt” auch bei Stromausfall erhalten
und die Uhr kann beim Wiedereinschalten zwar nicht mehr die Weckzeit erinnern,
aber mittels Buzzer signalisieren, dass die Weckfunktion ausgefallen ist.

Die Antenne sollte anfänglich im Gehäuse integriert werden, nachdem der Empfang
aber aufgrund der Abstrahlung (oder Schirmung) durch die anderen Komponenten
quasi unmöglich war, wurde die Möglichkeit des Anschlusses einer externen
Antenne geschaffen. Da sich die Uhr aufgrund ihrer Größe und Versorgungsspannung
vielleicht auch „portabel” einsetzen lassen könnte, wurde eine Steckerverbindung
zur Antenne vorgesehen.

## Schaltplan

_TODO ..._

Softwaredesign
==============

Die Implementierung erfolgte in Ada, nachdem die Einschränkung auf C oder C++
durch den Arduino nicht mehr vorhanden war. Die Vielseitigkeit des
Ada-Typsystems lässt sich bei diesem Projekt gewinnbringend einsetzen, um
das vorherige „Bit Fiddling” aus der C-Implementierung abzulösen. Dank
Unterstützung für Subprozeduren und Module (Packages) können auch einige vormals
sehr länglichen Identifier wieder etwas kürzer ausfallen. Beispiel: Der lange
Funktionsname `dcf77_secondlayer_check_bcd_correct_telegram_ignore_eom` ist
nicht mehr erforderlich, stattdessen ist es jetzt `Check_BCD_Correct_Telegram`
mit dem optionalen Parameter `Ignore_EOM` im Package `DCF77_Secondlayer`.

Entsprechend des Designziels beschäfigt sich der Großteil des Programmes mit der
DCF77-Decodierung. Die Hardwarenahen Teile sind zentral in einer einzigen Datei
untergebracht, sodass zum Test mit einer „virtuellen” Hardware nur diese Datei
ersetzt und anders implementiert werden muss.

## DCF77-Decodierung

Vom Antennenmodul werden Pulse mit Längen von 100ms bzw. 200ms geliefert, um
eine logische 0 bzw. 1 zu codieren. Als Endmarkierung (60. Puls) bleibt der
Puls aus. Intern werden im Programm daher die Ablesungen (Reading) `Bit_0` (0),
`Bit_1` (1) und `No_Signal` (3) unterschieden.

Die unterste Abstraktionsebene stellt der _Bitlayer_ dar: Er ist dafür
zuständig, die eingehenden Pulslängen digital zu Reading zu codieren. Dazu wird
eine Zuordnungstabelle von Intervalllängen zu Ablesungen verwendet.

Darüber befindet sich der _Secondlayer_. Die Idee hinter dieser Schicht ist es,
die Ablesungen der letzten 9 Minuten vorzuhalten und anhand der Muster (bspw.
Lage der Endmarkierung) die einzelnen DCF77-Telegramme (Telegram) zu erkennen.
Außerdem wird zur Verbesserung des Empfangs ausgenutzt, dass sich innerhalb von
10 Minuten maximal 1x der Minuten-Zehner ändert. Damit können mehrere
aufeinanderfolgende Telegramme zusammengelegt werden, um fehlende empfangene
Bits (No_Signal) zu eliminieren (X_Eliminate). Insgesamt werden daraus bis zu
zwei Telegramme synthetisiert: Telegram_1 enthält das zuletzt vollständig
empfangene (“aktuelle”) Telegramm und Telegram_2 alles, was zum vorherigen
Minuten-Zehnerstand zugeordnet werden konnte.

Der _Timelayer_ nutzt diese Daten, um die eigentliche Decodierung durchzuführen.
Im einfachsten Fall ist der Empfang (nahezu) perfekt und das Datum und die
Uhrzeit lassen sich aus dem Telegram_1 vollständig decodieren. In diesem Fall
wird „perfekte Empfangsqualität” mittels _Quality Of Service 1_ (QOS1)
signalisiert. Falls eine derart einfache Decodierung nicht möglich ist, werden
verschiedene Verfahren zum Einsatz gebracht, um die fehlenden Daten zu
ermitteln, wobei mit höherem QOS-Wert die Qualität der Widerherstellung abnimmt.
Kann das Programm überhaupt keinen Zusammenhang mehr zwischen den empfangenen
Telegrammen und der aktuellen Uhrzeit herstellen fällt es auf `QOS9_ASYNC`
zurück, was so viel heißt, wie dass die Uhr asynchron zum DCF77-Zeitsignal
läuft. In diesem Fall wird anhand einer integrierten Berechnungsvorschrift
die Uhrzeit berechnet. Schaltjahre werden auch in diesem Falle berücksichtigt,
die Zeitumstellung jedoch nicht.

## Hardwareabstraktion

Die Ansteuerung der Hardware efolgt mittels der _Low-Level_-Bibliothek
(`DCF77_Low_Level`). Darüber hinaus wird für die Abstraktion des
Displayprotokolls eine eigene Bibliothek (`DCF77_Display`) genutzt.

Der `DCF77_Ticker` gehört im weitesten Sinne auch zur Hardware und unterstüzt
das Programm beim Einhalten eines gleichmäßigen Verarbeitungstaktes. Intern
werden dadurch Berechnungen alle 100ms ausgeführt. Die vom Ticker eingefügte
Wartezeit richtet sich nach der gemessenen und für die Verarbeitung der Daten
benötigten Rechenzeit, passt sich also automatisch an, wenn das Programm
komplexer oder schneller wird.

Nachbau
=======

Die Software und Hardwarebeschreibung in diesem Repository sind
_Freie Software_. Daher ist u.A. ein Nachbau ohne Zahlung von Lizenzgebühren
möglich.

Grundsätzlich habe ich die Uhr bisher erst genau dieses eine Mal gebaut. Von
daher ist es nicht unwahrscheinlich, dass die Pläne noch den ein oder anderen
Fehler enthalten, der im Rahmen der Ausführung erst korrigiert wurde. Es ist
daher dringend geraten, Schaltplan, `DCF77_Low_Level` und Verdrahtungsplan
spätestens vor der 1. Inbetriebnahme abzugleichen.

## Software

Die Software kann mittels Alire-Buildtool kompiliert werden und
anschließend mit `elf2uf2` für den Raspberry konvertiert werden, sodass man
sie im Boot-Modus (BOOT-Taster gedrückt halten beim USB-Anschließen) auf den
am Hostsystem gemeldeten Massenspeicher kopieren kann.

Die genauen Kommandos zum Bauen der Software stehen im `build.xml`. Wenn
`ant`, `alr` und `elf2uf2` installiert sind, genügt ggfs. ein
`ant`-Aufruf, um das fertige Programm unter `bin/dcf77vfd.uf2` zu bauen.

## Stückliste

_TODO ..._

## Hardwarezusammenbau

Geeignete Werkzeuge für die Kunststoffbearbeitung und einen Lötkolben
vorausgesetzt, sollte der Zusammenbau weitgehend selbsterklärend sein.

Da das Antennenkabel relativ starr ist, gibt es ggfs. Sinn, es etwas länger
(bspw. 2m) zu wählen, damit man eine Chance hat, die Antenne auszurichten, ohne,
dass sie sich aufgrund der Starrheit des Kabels wieder in die falsche Position
dreht.

Tests
=====

Zur Software gehören diverse Test- und Hilfsprogramme, die bei der
DCF77-Entwicklung (ggfs. auch in anderen Projekten) helfen können.

## Telegramm-Editor

![Telegramm-Editor Screenshot](dcf77_vfd_raspi_clock_att/scrteledit.png)

Im Ordner `telegram_editor` befindet sich ein in Java geschriebener
DCF77-Telegrammeditor, der sowohl Telegramme decodieren, als auch ein
„virtuelles” Telegramm zu einer gegebenen Datums- und Uhrzeitangabe berechnen
kann. Er unterstützt darüber hinaus mehrere CSV-Formate, die teilweise bei
der Entwicklung des 1. Implementierungsversuchs in C hilfreich waren.

## Simulator

![Simulator-Screenshot mit einer Vorversion der Uhrensoftware](dcf77_vfd_raspi_clock_att/scrsimulator.png)

Unter `src_simulated` befinden sich diverse Symlinks und einige zusätzliche
Dateien, mit denen sich das Programm auf dem Hostsystem kompilieren und ohne
die eigentliche Hardware ausführen lässt. Das Interface ist dann ein
„interaktives” Konsolenprogramm (stdin/stdout). Um dieses zu steueren wurde eine
GUI in Java entwickelt (`src_simulator_gui`), die das Executable startet und
die Interaktion per Pipe übernimmt. Damit kann man bspw. interaktiv die GUI
oder testen oder live zusehen, wie die virtuelle Uhr das vorgefertigte Telegramm
aus der enthaltenen `testvector.csv` verarbeitet.

## Test-Framework

Unter `test_framework` liegt ein Ada-Programm, welches zusammen mit dem Code
aus `src_simulated` zu einem `dcf77_test_runner` kompiliert werden kann.

Mit diesem Testprogramm lassen sich XML-Dateien laden, in denen jeweils
Testdaten in Form von Telegrammen und erwarteten Datums-Uhrzeit-Ergebnissen
oder relevanten Berechnungszwischenständen abgelegt sind.

Auf diese Weise lassen sich automatisiert auch sehr spezielle Fälle
wie bspw. Schaltsekunden testen. Mit gewissen Einschränkungen erlauben diese
Tests auch das Einstreuen von Störungen, indem man bspw. die 0/1-Werte aus
den Telegrammen durch Ersetzen mit 3-Ziffern überschreibt und somit
Empfangsprobleme simuliert.

Falls man ein eigenes DCF77-Projekt entwickelt, könnten insbesondere die
QOS-Testdaten interessant sein, da dort auch jeweils der erwartete Zeitstempel
angegeben ist, zu dem die Eingabedaten decodiert werden sollten.

Mittels `ant cov` im Ordner `test_framework` kann das Test-Framework auch
so aufgerufen werden, dass die _Line-Coverage_ mittels `lcov` ermittelt wird
und im Ordner `cov` ein HTML-Report abgelegt wird. Dieser Report kann hilfreich
sein, wenn man sich unsicher ist, ob ein gewisser Codeabschnitt überhaupt durch
die Tests erreicht wird.

Bedienungsanleitung
===================

_TODO EXPLAIN THE GUI HERE_
