# VPP Archiver
Archiver similar to .tar, .zip, .7z

## Usage: 
Clone the repo and run ```make```. You can start already start using with ```./vina++ ...``` or add it to your PATH
so you don't need the "./" before.

- List all the commands  
```vina++ -h```

- Includes "file.txt" and "anotherFile.txt" in archive.vpp  
```vina++ -i archive.vpp file.txt anotherFile.txt```

- Updates "despesas.ods" in archive.vpp, if it's newer  
```vina++ -a archive.vpp despesas.ods```

- Extract the file "arq.txt" from archive.vpp  
```vina++ -x archive.vpp arq.txt```

- Extract all files  
```vina++ -x backup.vpp```

- Move file "arq.txt" to after "despesas.ods" in backup.vpp  
```vina++ -m despesas.ods backup.vpp arq.txt```

- Includes the files "xy/dir/arq.txt", "/dir/foto.jpg"   
```vina++ -i novo_backup.vpp xy/dir/arq.txt /dir/foto.jpg```


## Contributing:
Feel free to open a PR adding new features/fixing bugs. If you want to contribute with this project just for learn C or
contribute to open source, below are listed some things you could do, ranked by difficulty:

### Easy
- Translate all the comments to English
- Improve the "-h" command.
- Create an output for the commands instead of just finishing without any message. (Like displaying a message that the extraction is complete)

### Medium
- Make the "move" function actually move the bytes inside the content area instead of just moving in the FilesList.
- If any errors occurs during the process, create a function that recovers the file to a not-corrupted state

### Hard
- Add a compression algorithm to make the archive size smaller. Could be a simple algo like RLE, feel free to DM to discuss it.

