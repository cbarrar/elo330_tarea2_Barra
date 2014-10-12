Tarea 2 - ELO330 Programación de Sistemas
----------------------------------
Introducción
-------------------------------------------

Es de mucha utilidad usara pipes para poder conectar programas, en el caso de esta tarea, se utilizan pipes para poder comunicarse con octave, octave proporciona funciones para realizar gráficos y además se hace uso de la herramienta de interpolación polinomial. En particular esta tarea trata sobre recuperación de señal de audio saturada, mediante octave usando interpolación polinomial se busca recostruir la señal perdida por la saturación. También se usa aplay un reproductor de audio que permite reproducir las señales.

Funcionamiento
---------------------------------------

Antes de ejecutar el programa es preciso tener instaladas las aplicaciones octave y aplay para ello usar las siguientes lineas de instrucción en el caso de Ubuntu:

    $sudo apt-get install aplay
    $sudo apt-get install octave
Para ejecutar el programa se debe realizar la siguiente instrucción desde consola:

     .\csa   <archivo_de_audio_original>  <ganancia> <offset> [p]
Donde: ganancia: corresponde al valor en que se desea saturar el audio original. offset: es la cantidad de tiempo que se desplazar el gráfico, se considera múltiplo de 0.125[ms]. p: indica si se desea o no reproducir los audios: original, saturado y recuperado.

PROBLEMA
-------------------------------

Al momento de interpolar usando octave existe un problema de desbordamiento de buffers, por cuestión de tiempo este problema no se resolvió, cabe señalar que el programa se comunica correctamente con octave y se reciben los valores recuperados, pero al momento de asignarlos se hace de forma errónea. Para poder visualizar y reproducir los audios se debe comentar la linea 155 (donde se llama a interpolatation).
        
