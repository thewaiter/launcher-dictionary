# Launcher-Dictionary

Module based on Spellcheck plugin for Launcher module.

Spellcheck module inspired me to create this module. The main goal is to have a quick access to dictionary via Launcher module in Bodhi GNU/Linux.

<p align="center">
  <img src="https://i.imgur.com/38gj3sj.png" alt="Screen Shot">
</p>

## Usage

First one must load the module in modules settings under Launcher tab. Now assuming one has sdcv installed and an stardict dictionary, then usage should be as simple as opening Mokshas Quick Launcher and typing 'd ' without the single quote marks. Note: one must type the blank following the d. The prompt will change to a colon and now type a word that should be translated. A list as pictured above will be displayed.
Press "CTRL + u" key combination for the new word searching.

## Dependecies

* [EFL](https://www.enlightenment.org/download)
* [Moksha](https://github.com/JeffHoogland/moksha)
* [sdcv](https://wiki.archlinux.org/index.php/Sdcv)
* [stardict dictionaries](https://tuxor1337.frama.io/firedict/dictionaries.html)

This module depends on sdcv utility. It is a console Stardict application.

sdcv installing:

```ShellSession
sudo apt update
sudo apt-get install sdcv
```

Unpack downloaded dicts and copy to /usr/share/stardict/dic directory

# Installation

It is recommended Bodhi users install from Bodhi's repo:

```ShellSession
sudo apt update
sudo apt install moksha-module-dictionary
```

Other users need to compile the code:

First install all the needed dependencies. Note this includes not only EFL but the EFL header files. If you have compiled and installed EFL and Moksha from source code this should be no problem.

Then the usual:

```ShellSession
meson . build
ninja -C build
sudo ninja -C build
```

## Reporting bugs

Please use the GitHub issue tracker for any bugs or feature suggestions:

>https://github.com/thewaiter/launcher-dictionary/issues

## Contributing

Help is always Welcome, as with all Open Source Projects the more people that help the better it gets!
More translations would be especially welcome and much needed.

Please submit patches to the code or documentation as GitHub pull requests!

Contributions must be licensed under this project's copyright (see COPYING).

## Module authors

Full credit for the original code of this module goes the enlightenment developers:

* Gustavo Barbieri
* Hannes Janetzek

Dictionary part

* Štefan Uram a.k.a the_waiter (c) 2019

Enjoy :)

To-Do

* [x] Line word wrapping
* [ ] Line word wrapping size according the Launcher window width
* [x] Line icon only if dict name 
* [x] Short text width in some themes - fixed in evrything.edc (e.text.detail)
* [ ] Text formating (color, size)
* [ ] Copying the whole block of text, not just one line
* [ ] Send the number value to the console when * follows the word
* [ ] Better README.md file
