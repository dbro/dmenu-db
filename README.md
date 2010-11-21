dmenu-db
========

This version of dmenu is a fork of the 
[official version 4.2.1](http://tools.suckless.org/dmenu/). It adds ...

* support for xft fonts (eg. [Inconsolata](http://www.levien.com/type/myfonts/inconsolata.html) )
* multiple token search, ie typing "ong nam" will match items with names
  like "xyz123abc456ThisIsAVeryLongFileNameThatIsDifficultToType"
* a few miscellaneous changes under the hood to make the code tidier

To use an xft font, use the normal "-fn FontName" option, and supply a valid
xft font name, eg. "Inconsolata-15". This command also recognizes the names of
regular x fonts and font sets, eg. "fixed".

These enhancements make dmenu suitable for use as an instant-search
navigator in the command line music player interface called [muss](http://github.com/dbro/muss)

Dan Brown, 2010

