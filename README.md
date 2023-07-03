# VPP Archiver
Archiver similar to .tar, .zip, .7z

## Usage: 
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
