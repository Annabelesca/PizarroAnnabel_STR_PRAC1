# <h1><p align="center">Sistemas de tiempo real</p></h1>
<h2><p align="center">Práctica 1. Compensador de luz</p></h1>

## 1. Decisiones de diseño

El objetivo principal de la práctica es programar un compensador de luz que, además de capturar la luminosidad de una habitación y mostrarla en porcentaje, debe controlar la regulación de intensidad de un led con el fin de compensar la luz faltante. Además de este objetivo, el programa debe tener unas funcionalidades extra: detectar fallos en el
hardware, calcular la media aritmética de los porcentajes de luminosidad capturados en los últimos diez segundos y, finalmente, que el usuario pueda escoger mediante un potenciómetro la cantidad de luz a compensar. 

Teniendo esto en mente, he divido el sistema en un bucle principal, un thread que se encarga de calcular la media aritmética de los valores en los 10 segundos posteriores a
la pulsación del botón y otro thread que se encargará de ir revisando el valor del potenciómetro. Más adelante, se va a explicar cada una de estas secciones un poco más a fondo, pero la idea es que cada una de las funcionalidades tenga su propio thread. 

Además de estas secciones, el programa cuenta con dos funciones auxiliares, una encargada de calcular el porcentaje de luz que se debe compensar con el led como de convertir los datos del fotoresistor al nivel de luminosidad que hay en el ambiente, y la
otra encargada de comprobar el correcto funcionamiento del sensor que se le pasa por parámetro, calculando la desviación de los valores de un pequeño muestreo.

### 1.1	Thread principal (main)
Es el encargado no solo de ir mostrando por pantalla los datos captados por el fotorresistor, sino también del control de errores del sensor en cuestión así como de iniciar los demás threads. Por defecto según la librería rtos.h, la prioridad de este thread es de tipo “osPriorityNormal”.

Su funcionalidad no es otra que leer los valores del fotorresistor (la lectura se hará en el rango de [0-1] ya que de esta manera nos viene ideal para el cálculo de los porcentajes) y de calcular los datos de interés. Además, de ello, se encargará de encender un buzzer en caso de que detecte que hay algún error en el sensor de luz. Este thread tendrá iteraciones cada 1 segundo usando la función “ThisThread::sleep_for(1s)” con tal de no apabullar al usuario con centenares de mensajes en pantalla con la misma (o muy similar) información así como para facilitar la lectura de los mismos.

### 1.2	Thread del cáculo de la media
Este thread será el encargado de leer valores durante 10 segundos y hacer la media de estos. El numero de valores a leer viene dado por una constante “FrecMuestreo” y el tiempo que el thread pasa “dormido” entre las lecturas depende directamente de esta magnitud y el tiempo que se quiera medir (en este caso 10s).

Una vez se han leído los valores y se ha hecho la media, se calculan los datos de interés y se muestran por pantalla.

Este thread está asociado a una interrupción que se genera al pulsar el botón, más concretamente en el paso de estado 1 a 0 (pulsado a no pulsado). Esta interrupción lo que hace es un signal al semáforo del thread. Cuando se ha creado el thread se ha definido la prioridad “osPriorityAboveNormal” lo que lo convierte en un thread más prioritario que el main, por tanto, cuando llegue la interrupción del botón, se dejará de ejecutar el thread del main para ejecutarse éste.

### 1.3	Thread potenciómetro 
Este thread es el encargado de ir calculando el lindar de la luz que hay que compensar en función de la posición del potenciómetro además de hacer el control del funcionamiento del sensor y, en caso de que este sea negativo, encender el buzzer para avisar del fallo.

Este thread, al igual que el thread para calcular la media se ha definido la prioridad “osPriorityAboveNormal”, pero a diferencia del anterior, no tiene asociada ninguna interrupción sino que se “despierta” por cuenta propia pasado un tiempo (marcado por la constante “FrecPotenciometro”) y, al ser más prioritario que el main, ocupa el procesador y se ejecuta.

### 1.4	Otras consideraciones
1. A la hora de usar sleeps o bucles se han definido constantes para que de esta manera sea mucho más fácil debugar el código, ya que con cambiar un valor, se cambia en todo el código. +
1. El lindar de luz (cantidad de luz que debe compensar nuestro led) está definido como una variable global ya que deben acceder a él dos threads distintos. Es por esto que se ha utilizado mutex para bloquear el acceso a este dato en caso de que otro thread esté utilizando el recurso en ese momento.
1. Para la función de comprobar si hay errores en un sensor, se ha definido una constante “Desviación”, que marcará el rango en el que se puede encontrar la media de los valores muestreados para que sea válida. Se ha definido como 0.015, es decir, una desviación del 1.5%. Para que se pueda afirmar que un sensor funciona correctamente, la diferencia entre la media y la última lectura sea menor que 0.015. En los sensores del kit parece que esta desviación funciona, pero no estoy realmente segura de que sea extrapolable a otros sensores.
1. A la hora de llamar a la función auxiliar que calcula los datos de interés (lux y porcentaje de luz a compensar por el led) se ha pasado por referencia un vector de tres posiciones que contiene (en orden): [lux, luz captada por el sensor, luz a compensar por el led]. Cuando llega a la función auxiliar, se usan los datos de luz captada por el sensor para calcular el resto de los valores. Otra manera de la que también se podría haber hecho es pasando por referencia dichos valores para que fueran directamente modificados por la función.
1. Ya que había pocas funciones a implementar, he decido no crear librerías que diferenciasen los diferentes threads, por lo que todo el código fuente se encuentra en un mismo fichero.
1. Las librerías utilizadas en el proyecto (además de la propia de mbed) son: cstdio (para poder utilizar los printf), cstlib (para usar la función de valor absoluto) y la librería rtos, para usar los diferentes threads que componen el sistema.

## 2. Planificación de las tareas
Como ya hemos dicho anteriormente, el thread menos prioritario es el main mientras que los otros dos threads (potenciómetro y botón) tienen la misma prioridad entre ellos pero son más prioritarios que el main. 

Así pues, cuando el programa empiece, se ejecutará el main hasta la creación de los threads, cuyo código se ejecutará hasta llegar al semáforo (en el caso del thread de la media) o hasta llegar al sleep (en el caso del thread del potenciómetro). Una vez en este punto, el thread del potenciómetro se ejecutará cada 200ms y el del botón siempre que se produzca la interrupción que produzca el signal.

Podemos representar también una línea de tiempo del grupo de tareas anteriormente definidas, teniendo en cuenta las tareas periódicas y la inicialización de la tarea aperiódica (Thread del botón). Para la ejecución de esta última, ya que no depende del tiempo, se necesitaría de la interrupción generada por el botón, por ello únicamente aparece representada una pequeña porción de tiempo, que correspondería a su tiempo de ejecución en durante su creación hasta alcanzar el semáforo pertinente que lo pondría a “dormir”.
![Figura 1. Representación gráfica de la planificación](https://github.com/Annabelesca/PizarroAnnabel_STR_PRAC1/blob/main/Figuras/Figura1.png)

## 3. Esquema de conexionado del proyecto
### 3.1	Tipo de actuadores/sensores utilizados
Como ya hemos comentado, haremos uso de un botón, un fotorreceptor, un buzzer, un potenciómetro y un LED. Antes de conectar los sensores y actuadores a la placa, hay que pensar un poco de que tipo serán para así poder conectar los pines correctamente. 

En lo referente a los actuadores, tenemos dos actuadores (el buzzer y el LED). En ambos deberemos regular la intensidad (ya sea de timbre o la luz que se emite), es por ello por lo que debe ser un pin que permite conexiones de tipo PWM. En lo referente a los sensores, tanto el fotorreceptor como el potenciómetro deben de ser entradas analógicas.

Una vez hemos definido esto, podemos realizar las conexiones de los dispositivos a la placa, en nuestro caso una ST Núcleo F767ZI, como podemos ver en la siguiente ilustración.

### 3.2	Representación gráfica del conexionado
![Figura 2. Representación gráfica del conexionado](https://github.com/Annabelesca/PizarroAnnabel_STR_PRAC1/blob/main/Figuras/Figura2.PNG)


## 4. Conclusiones
Durante la práctica se ha podido crear un sistema a tiempo real con las funcionalidades especificadas en el guion de prácticas de forma satisfactoria usando un kit de iniciación de mbed.

Para realizar este proyecto (así como cualquier otro de tiempo real) ha sido importante tener en cuenta el tiempo de ejecución de cada uno de los hilos para un correcto funcionamiento del programa. Una incorrecta planificación de los procesos que forman la aplicación a tiempo real podría conllevar problemas a la hora de coordinarlos y podrían llegar a pisarse y no acabar (o acabar prematura e incorrectamente) su ejecución.

Otra cosa para tener en cuenta también es la lectura y escritura de los distintos hilos. En este caso no hay dependencias WAW (Write After Write) pero si podemos llegar a establecer dependencias RAW (Read After Write), aunque en este sistema a tiempo real una lectura de un valor anterior al real no afectará de forma relevante en el resultado a diferencia de unos STR con un tiempo de respuesta crítico.

Finalmente cabe destacar que en un sistema de tiempo real con estas características se debe tener en cuenta si algún pin conectado a algún sensor/actuador está bien conectado y funciona correctamente, ya que de lo contrario no se estarían obteniendo los valores reales de los dispositivos hardware. 

En resumen, en sistema a tiempo real se debe tener en cuenta una buena planificación de los procesos que forman el sistema, las dependencias de escritura/lectura de las variables compartidas de los distintos hilos así como de un buen sistema de detección de errores. 
