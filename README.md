# string-noise
Add noise to python strings using various augmentation styles. Applies an N:M character-sequence mapping with probabilistic replacement and weighted replacements. Implemented in C for fast processing. Intended for string augmentation while training in the loop, with realistic synthetic error production.

## Styles
### homoglyph
replace with similar looking unicode glpyhs
```
10%  Loяem !psum dolor sit amet, consect37ur adìpiscing e7it, sed do eiusmod tempor incididunt ut łabore et dolore magna al|ɋua.
30%  Lorem ip5υɱ ԁoloя sit ame7, ¢onsectetυř ªd!ρіscіng еlit, ßed dö eiusmоd τеmpor inсidіdunt u+ la6øre eτ cloloя3 nnаgna аligμ@.
100% Ĺõгеɱ іþ§μɱ đóIQ® 5!+ @ɱе7, [оmʃе{ŧε‡υя аԁіρі$¢іm9 3ĺì7, ßеԁ clô ε|ù§nnоð +εɱрøг 1m{іđіclūr‡ ú+ Iа♭оге 3τ ԁ0Ióɾ& ɱ4ɡrа 4ĺî9ūа.
```

### leet
1337 replacements
```
10%  Lorem 1psum dolor sit amet, consectetur adipiscing e|it, s3d do eiu5mod tempor incididunt ut labore et dolore magna aliqua.
30%  |o|23m ips|_|m dolo|2 sit @m3t, co|\\|$3ctetur ad!piscing e1i7, 5ed d0 eiusmo|] tempo|2 incid!dunt |_|t labo.-e et |)olore m46na a|1qua.
100% 10.-3|\\/| !|*5|_|/\\/\\ |)010|2 $17 @/\\/\\3+, <0|\\|53{73+|_|.- @|)1|D!${!|\\|6 31!+, $3|] |)0 3!|_|5|\\/|0|) 73|\\/||o0.- 1|\\|(!|]!|)|_|/\\/+ |_|+ |460|23 3+ |]0|0.-3 /\\/\\@6/\\/@ @110,|_|@.
```

### phonetic
similar sounds

### keyboard
Nearest keystroke mistakes
```
10%  Lorem ipsom dulor sit amet, consectetur adipiscing elit, sed do eiusmad tambor incididunt ut labure et dolore magna alyqua.
30%  
```

## Usage
`python setup.py install`

```
from string_noise import load_homoglyph

homoglyph = load_homoglyph()
homoglyph("hello world", probability=0.5)
```
