CherryLock
==========

![Logo](https://raw.githubusercontent.com/adamj57/CherryLock/master/logo.png)


What is CherryLock?
-------------------

CherryLock is a project for Students' Council in Technikum Mechatroniczne nr 1. It's a system that unlocks the door when coorect tag is read by scanner locateed on the wall. Thanks to it, only certain persons can enter the SU (Samorząd Uczniowski/Students' Council) on their own.

How it works?
-------------

Here is a ascii-flowchart explaining that:

                                        +---------+
    +----------------------------------->READ TAGS<-------------------------------------------+
    |                              +---------V----------+                                     |
    |                              |                    |                                     |
    |                              |                    |                                     |
    |                         +----v-----+        +-----v----+                                |
    |                         |MASTER TAG|        |OTHER TAGS|                                |
    |                         +--+-------+        ++-------------+                            |
    |                            |                 |             |                            |
    |                            |                 |             |                            |
    |                      +-----v---+        +----v----+   +----v------+                     |
    |         +------------+READ TAGS+---+    |KNOWN TAG|   |UNKNOWN TAG|                     |
    |         |            +-+-------+   |    +-----------+ +------------------+              |
    |         |              |           |                |                    |              |
    |    +----v-----+   +----v----+   +--v--------+     +-v----------+  +------v----+         |
    |    |MASTER TAG|   |KNOWN TAG|   |UNKNOWN TAG|     |GRANT ACCESS|  |DENY ACCESS|         |
    |    +----+-----+   +---+-----+   +-----+-----+     +-----+------+  +-----+-----+         |
    |         |             |               |                 |               |               |
    |       +-v--+     +----v------+     +--v---+             |               +--------------->
    +-------+EXIT|     |DELETE FROM|     |ADD TO|             |                               |
            +----+     |  EEPROM   |     |EEPROM|             |                               |
                       +-----------+     +------+             +-------------------------------+

Yep, that's it. It's that easy to use! It uses only one tag for adding/removing alllowed tags to the system on the fly.

EEPROM
------

Access card ID's and other stuff is stored on internal EEPROM of the Arduino. Be careful when you adapt it for your own purpose - EEPROM has a limited write cycles, roughly about 100 000. It's better to add all allowed tags at once, before implementing CherryLock. Also, be careful when it comes to deletes - they also eat those write cycles, even **more efectively than the standard addition of the tag!** The way that deletion works is by shifting (it's more like copying right to left and then setting last 4 occupied adresses's values of memory mapping to 0, but let's call it shifting) all data placed after the chosen value by 4 bytes to the left. See, it can hurt those poor write cycles :(

Memory mapping
--------------

EEPROM on Arduino UNO has 1024 bytes of space. In this section I will explain what belongs to where. First, let's start with basics.
Adress space of EEPROM is 0-1023 - each adress belonging to each byte. My notation is simple:

* `[45]` referes to adress 45,
* `[45:4]` referes to 32-bit word, starting from adress 45.

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
		<td><b>Signal</b></td>
		<td><b>Arduino/ATMega328P Pin</b></td>
		<td><b>MFRC522 Reader Pin</b></td>
		<td><b>Relay</b></td>
	</tr>
	<tr>
		<td>MFRC522 Reset</td>
		<td>9</td>
		<td>RST</td>
		<td>-</td>
	</tr>
	<tr>
		<td>SPI SS</td>
		<td>10</td>
		<td>SDA(SS)</td>
		<td>-</td>
	</tr>
	<tr>
		<td>SPI MOSI</td>
		<td>11</td>
		<td>MOSI</td>
		<td>-</td>
	</tr>
	<tr>
		<td>SPI MISO</td>
		<td>12</td>
		<td>MISO</td>
		<td>-</td>
	</tr>
	<tr>
		<td>SPI SCK</td>
		<td>13</td>
		<td>SCK</td>
		<td>-</td>
	</tr>
	<tr>
		<td>Relay switch</td>
		<td>4</td>
		<td>-</td>
		<td>O</td>
	</tr>
</table>