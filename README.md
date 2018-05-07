CherryLock
==========

![Logo](https://raw.githubusercontent.com/adamj57/CherryLock/master/logo.png)


What is CherryLock?
-------------------

CherryLock is a project for Students' Council in Technikum Mechatroniczne nr 1. It's a system that unlocks the door when corect tag is read by scanner located on the wall. Thanks to it, only certain people can enter the SU (Samorząd Uczniowski/Students' Council) on their own. CherryLock also enables users to open the door remotely using standalone button, placed inside the room. Usage (successful tag opens and button presses) is logged on the SD card. I'm in process of developing a app ([CherryLock Manager](http://github.com/adamj57/CherryLockManager)) that allows administrators to check on logs, add and remove new people from database and a few more things.

How it works?
-------------

Here's flowchart explaining that:

![Flowchart](https://raw.githubusercontent.com/adamj57/CherryLock/master/diagram.png)

Yep, that's it. It's that easy to use! It uses only one tag for adding/removing alllowed tags to the system on the fly.

EEPROM
------

Access card ID's and other stuff is stored on internal EEPROM of the Arduino. Be careful when you adapt it for your own purpose - EEPROM has a limited write cycles, roughly about 100 000. It's better to add all allowed tags at once, before implementing CherryLock. Also, be careful when it comes to deleting stuff - that also eats those write cycles, even **more efectively than the standard addition of the tag!** The way that deletion works is by shifting all data placed after the chosen value by 4 bytes to the left (it's more like copying right to left and then setting last 4 occupied adresses's values of memory mapping to 0, but let's call it shifting). See, it can hurt those poor write cycles :(

Memory mapping
--------------

EEPROM on Arduino UNO has 1024 bytes of space. In this section I will explain what belongs to where. First, let's start with basics.
Adress space of EEPROM is 0-1023 - each adress belonging to each byte. My notation is simple:

* `[45]` referes to adress 45,
* `[45:4]` referes to 32-bit (4-byte) word, starting from adress 45.

So, now when notation is explained - here's the mapping:

<table>
	<tr>
		<td><code>[0]</code></td>
		<td>Number of cards added. Used to determine where to search for card's ID's.</td>
	</tr>
	<tr>
		<td><code>[1]</code></td>
		<td>Contains 42, if the master card is defined. If other value is contained, then after restart system will ask you to define master card.</td>
	</tr>
	<tr>
		<td><code>[2:4]</code></td>
		<td>Master card's ID.</td>
	</tr>
	<tr>
		<td><code>[6:4]<br>[10:4]<br><center>.<br>    .<br>    .<br></center>[1014:4]<br>[1018:4]</code></td>
		<td>List of access cards' ID's. There's space for 253 cards.</td>
	</tr>
</table>

Typical pin layout
------------------

*Work in progress*

<table>
	<tr>
		<td>Arduino Pin</td>
		<td>Signal</td>
	</tr>
	<tr>
		<td>0</td>
		<td>Serial RX</td>
	</tr>
	<tr>
		<td>1</td>
		<td>Serial TX</td>
	</tr>
	<tr>
		<td>2</td>
		<td>Wireless Open Button</td>
	</tr>
	<tr>
		<td>3</td>
		<td>Wipe Button</td>
	</tr>
	<tr>
		<td>4</td>
		<td>Relay Enable</td>
	</tr>
	<tr>
		<td>5</td>
		<td>Blue LED</td>
	</tr>
	<tr>
		<td>6</td>
		<td>Green LED</td>
	</tr>
	<tr>
		<td>7</td>
		<td>Red LED</td>
	</tr>
	<tr>
		<td>8</td>
		<td>MFRC522 MOSI</td>
	</tr>
	<tr>
		<td>9</td>
		<td>MFRC522 MISO</td>
	</tr>
	<tr>
		<td>10</td>
		<td>MFRC522 SCK</td>
	</tr>
	<tr>
		<td>11</td>
		<td>SD MOSI</td>
	</tr>
	<tr>
		<td>12</td>
		<td>SD MISO</td>
	</tr>
	<tr>
		<td>13</td>
		<td>SD SCK</td>
	</tr>
	<tr>
		<td>A0</td>
		<td>MFRC522 SS</td>
	</tr>
	<tr>
		<td>A1</td>
		<td>MFRC522 RST</td>
	</tr>
	<tr>
		<td>A2</td>
		<td>SD SS</td>
	</tr>
	<tr>
		<td>A3</td>
		<td>[Reserved for SD Full LED, to be done]</td>
	</tr>
	<tr>
		<td>A4</td>
		<td>SDA (Now used only for RTC)</td>
	</tr>
	<tr>
		<td>A5</td>
		<td>SCL (Now used only for RTC)</td>
	</tr>
</table>