# Mac OS Region Definitions

This folder contains the script and region definitions for the Mac OS toolbox.

- `extract.py`: Generate `script.csv` and `region.csv` from the `Script.h` file in Mac OS Universal Interfaces. The output of this program is checked in, so it does not need to be run again unless the logic is changed.

- `script.csv`: Constants identifying scripts.

- `region.csv`: Constants identifying localization regions.

- `charmap.csv`: Identifies character maps used by classic Mac OS. Each character map is given a name, a data file in the `../charmap` folder, and the script and, optionally, regions it corresponds to. This mapping is taken from the readme in the charmap folder. More specific mappings (which contain regions) in this file take precedence less specific mappings (which do not contain regions). For example, Turkish is more specific than Roman.
