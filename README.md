# Mailbox_client

Drei Haupt-Tasks:

•	Deep Sleep

•	Get Data

•	UDP-Client

## Main

Als erstes wird der Get Data Task created, dieser wartet auf ein Notify.
Als nächstes wird der UDP-Client Task created, dieser versucht sich mit dem Netzwerk zu verbinden und wartet anschließen auf ein Notify.
Als letztes wird der Deep Sleep Task created, dieser beginnt dann mit dem Programmdurchlauf.

## Deep Sleep

Das Erste, was das Programm macht, ist den Deep Sleep Task laufen zu lassen. Die anderen Tasks werden zwar zuerst created, jedoch warten sie auf TaskNotifys.
Der Deep Sleep Task checkt als erstes den wakeup cause. Je nach wakeup cause benachrichtigt er den zugehörigen Task. Anschließend wartet er bis man ihm wieder Bescheid gibt, dass es Zeit wird, schlafen zu gehen.
Vorgehensweisen für folgende Wakeup Causes:

•	Undefined:  ESP geht schlafen (normalerweise nach einem Reset bzw. Programm Flash)

•	GPIO Wakeup:  GPIO Wakeup Task wird ausgeführt (Taskhandler wird als Parameter mitgegeben)

•	Timer Wakeup:  Timer Wakeup Task wird ausgeführt (Taskhandler wird als Parameter mitgegeben)

Wenn der Task auf eine Benachrichtigung warten, um schlafen zu gehen, analysiert er den TaskNotify-Value. Falls dieser von null verschieden ist, stellt der Deep Sleep Task einen Timer der den ESP wieder aufweckt. Die Zeit, die der ESP im Deep Sleep verbringt, entspricht dem TaskNotify-Value in Sekunden. Der Timer wird gestellt, falls es Übertragungsprobleme gegeben hat -> später erneut Versuchen.

## Get Data

Der get_data_task wird vom Deep Sleep Task aufgerufen. Als TaskNotify-Value wird der Wakeup Cause mitgegeben. Es wird folgendes unterschieden:

•	Scale Wakeup:

Der ESP wurde durch eine Massenveränderung auf der Waage aufgeweckt -> ADC Messungen werden durchgeführt um Gewicht und Akkuladung zu erhalten und zusätzlich die Zeit vom RTC Modul erfragt. Diese werden in einem struct mit dem Namen udp_data_t geschrieben


•	Timer Wakeup:

Es wird dieselbe Prozedur wie bei einem Scale wakeup durchlaufen jedoch mit einem Unterschied. Die Zeit wird nicht erneut abgefragt. Dadurch behält man die ursprüngliche Zeit des aller ersten Aufwachens des ESPs.
Anschließen wird der UDP-Client Task benachrichtigt.


## UDP-Client

Beim erstmaligen Ausführen (=beim xTaskcreate) des Tasks wird versucht eine WLAN-Verbindung zum Heimnetzwerk aufzubauen. Dies wird x-mal versucht bevor der ESP erneut in Deep Sleep versetzt wird, um es später erneut zu versuchen (Anzahl der Versuche in SDK-Config einstellbar).

Wenn die Verbindung erfolgreich war wird ein socket erstellt mit den entsprechenden Konfigurationen und anschließen gelangt das Programm in eine Endlosschleife. 

In dieser Endlosschleife befindet sich als erstes ein TaskNotifyWait. Sobald der Get Data Task Daten erlangt hat, benachrichtigt er diesen UDP-Client. Folglich werden die Daten an den definierten UDP-Server gesendet.

Es wird wieder eine bestimmte Anzahl an Versuchen getätigt, die Daten zu senden und zusätzlich eine gewisse Zeit auf eine Antwort zu warten. Falls nie die erwartete Antwort erhalten wird, wird der Deep Sleep Task mit einem bestimmten Notify-Value  benachrichtig ->  später erneut versuchen.

Und falls alles erfolgreich war wird der Deep Sleep Task ohne Notify-Value benachrichtigt, daher schläft der ESP bis zum nächsten GPIO wakeup weiter und beginnt dann das Selbe Spielt von vorne.
