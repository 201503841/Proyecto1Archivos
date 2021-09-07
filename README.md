# Proyecto1Archivos
Proyecto archivos 2021

======================

  - [Introducción](#introducción)
  - [Pre-requisitos](#pre-requisitos)
  - [Instalacion](#instalacion)
  - [Comandos](#comandos)
  - [Reportes](#reportes)

  


## Introducción ##
La aplicación es totalmente en consola, a excepción de los
reportes en Graphviz. No distinguirá entre mayúsculas y
minúsculas. La aplicación será capaz de leer desde standard
input(stdin) y escribir a standard output (stdout) de ser
necesario. Hay parámetros obligatorios y opcionales. Solo se
puede colocar un comando por línea.




### Pre-requisitos 📋

```
Qt
Graphviz

```

### Instalación 🔧

_GRAPHVIZ

```
sudo apt-get update
sudo apt-get install graphviz

```


## COMANDOS ##

CONSOLA
=====================
La consola presenta la información del estudiante seguido de un menú corto en el cual se debe selecciona 1 si se desea escribir un comando 

![GitHub Logo](/Fotos/IMAGEN1.jpg)

COMANDO EXEC
======================
## Carga Masiva: 
Se ejecutara con el comando EXEC seguido del parametro path o direccion del archivo .sh que contiene los comandos.

![GitHub Logo](/Fotos/IMAGEN2.jpg)

![GitHub Logo](/Fotos/IMAGEN3.jpg)

COMANDO MKDISK
======================
## Creacion de Discos Duros: 
Se realizara por medio del Comando MKDISK. (Que ya contiene el archivo "calificacion.sh")

## Ejemplo en consola:

```
mkdisk -size=75 -u=M -path=/home/suseth/Escritorio/Disco1.dk -f=FF

```

![GitHub Logo](/Fotos/IMAGEN4.jpg)

COMANDO RMDISK
======================
## Eliminacion de Discos Duros:
Se realizaran por medio del comando RMDISK. Recibe como parametro la direccion en donde se encuentra el disco. Seguidamente le preguntara si esta seguro de eliminar el disco.

## Ejemplo en consola:

```
rmDisk -path=/home/suseth/Escritorio/Disco6.dk

```

![GitHub Logo](/Fotos/imagen9.jpg)

COMANDO FDISK
======================
## Creación de Particiones:
Se ejecutaran por medio del comando FDISK. Algunos parametros obligatorios es el tamaño de la partición, la dirección del disco y el nombre de la partición.


## Ejemplo en consola:

```
fdisk -type=P -u=M -name=Part1_D1 -size=20 -path=/home/suseth/Escritorio/Disco1.dk -f=BF

```

![GitHub Logo](/Fotos/IMAGEN5.jpg)

COMANDO MOUNT
======================
## Montar particiones en el disco:
Este comando montará una partición del disco en el sistema e imprimira en consola un resumen de todas las particiones montadas actualmente.

## Ejemplo en consola:

```
mount -path=/home/suseth/Escritorio/Disco1.dk -name=Part1_D1

```

![GitHub Logo](/Fotos/IMAGEN8.jpg)


COMANDO UMOUNT
======================
## Desmonta una partición del sistema:
Se utilizará el id que se le asignó a la partición al momento de cargarla. Si el ID no existe se notificará por un mensaje que no se encontre el id.

## Ejemplo en consola:

```
umount -id=413b
```

![GitHub Logo](/Fotos/imagen13.jpg)

COMANDO MKFS
======================
## FORMATEO:
Este comando realiza un formateo completo de la partición, seformateará como ext2 por defecto si en caso el parámetro fs no está
definido.
## Ejemplo en consola:

```
mkfs -type=fast -id=411a -fs=3fs

```

![GitHub Logo](/Fotos/IMAGEN10.jpg)


 También creará un archivo en la raíz llamado users.txt quetendrá los usuarios y contraseñas del sistema de archivos.

![GitHub Logo](/Fotos/IMAGEN11.jpg)


## REPORTES ##

REP
======================
Recibirá el nombre del reporte que se desea y lo generará con graphviz en una carpeta existente. 


## Ejemplo en consola:

```
rep -id=411a -path=/home/suseth/Escritorio/Reportes/reporte_D1.jpg -name=mbr

```


REPORTE MBR
======================
Mostrará tablas con toda la información del MBR, así como de los EBR que se pudieron haber creado.

## Ejemplo en consola:
```
rep -id=411a -path=/home/suseth/Escritorio/Reportes/reporte_D1.jpg -name=mbr

```

![GitHub Logo](/Fotos/IMAGEN6.jpg)



REPORTE DISK 
======================

Este reporte mostrará la estructura de las particiones, el mbr del disco y el porcentaje que cada partición o espacio libre tiene dentro del disco (La sumatoria de los porcentajes debe de ser 100%).

## Ejemplo en consola:
```
rep -id=411a -path=/home/suseth/Escritorio/Reportes/reportedisk_D1.jpg -name=disk

```
![GitHub Logo](/Fotos/IMAGEN7.jpg)


REPORTE TREE 
======================

Este reporte genera el árbol de el sistema ext2/ext3. Se mostrará toda la información de los inodos o bloques

## Ejemplo en consola:
```
rep -id=411a -Path=/home/suseth/Escritorio/Reportes/reportetree.jpg -name=tree

```
![GitHub Logo](/Fotos/IMAGEN12.jpg)
