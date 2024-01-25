== Notes ==

So far only reading of the buttons works.
This is using one of the 8 * 7 segment displays with 16 buttons.
7 Segment output does not work, need to track down with a logic analyser. I am using the SPI peripheral in its 3-wire, half-duplex mode. Since the buttons work, I must just be sending the wrong command.