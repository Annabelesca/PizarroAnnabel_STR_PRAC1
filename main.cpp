#include "mbed.h"
#include <cstdio>
#include <cstdlib>
#include "rtos.h"


//Definimos las constantes que nos permitiran calcular la lux
#define luxRel 500
#define VRef 3.3
#define Rl 10
#define FrecMuestreo 100
#define FrecPotenciometro 200ms
#define MuestrasError 10
#define Desviacion 0.025

//Definimos los sensores/actuadores y los pins donde se ecuentran
PwmOut Led(D5);
AnalogIn Photoresistor(A0);
AnalogIn Potentiometer(A2);
InterruptIn AvButton(A3);
Thread AvCalculator(osPriorityAboveNormal);
osThreadId AvCalculatorID;

Mutex stdio_mutex;

Thread LinPotenciometer(osPriorityAboveNormal);
osThreadId LinPotID;
Ticker timer;

//Variables globales
float lindar = 1;       //Definirá hasta que porcentaje de luz, compensará el LED
char hayError = 0;  //Variable que indicará si existe algún error en el programa

void AvCalc_Interrupcion();
void AvCalc_Thread();
void LinPot_Interrupcion();
void LinPot_Thread();


/*
luxCalc
    Funcion auxiliar que se encarga de calcular el porcentaje de luz que debe compensar el LED así como también el nivel de luminosidad que hay en el ambiente
Parametros:
    datos: Array de tres posiciones que contiene el porcentaje de luz que capta el sensor en la posicion 1
Retorna:
    datos: Array con las posiciones 0 y 2 modificadas. La posicion 0 contendrá el nivel de luminosidad (lux) y la posicion 2 el % de compensacion del LED
*/
void luxCalc(float datos[3]){
    float vout = datos[1]*(VRef/1);
    datos[0] = (((VRef*luxRel)*vout)-luxRel)/Rl;
    stdio_mutex.lock(); 
    if (lindar-datos[1]<0) datos[2]=0;
    else datos[2] = lindar - datos[1];
    stdio_mutex.unlock();
}

/*
comprobarFuncionamientoSensor
    Funcon auxiliar que se encarga de controlar el correcto funcionamiento del sensor que se le pasa por parametro haciendo una media de valores leidos y mirando
    la desviacion
Parametros:
    sensor: Sensor a ser comprobado
Retorna:
    false si no hay error y true si lo hay
*/
bool comprobarFuncionamientoSensor(AnalogIn sensor){
    float average=0, lectura; 
    bool error=false;

    for (int i=0; i<MuestrasError; i++) {   
        lectura=sensor.read();
        average=average+lectura;
    }
    average=average/MuestrasError;
    if (abs(average-lectura)>=Desviacion) error=true; //Si diferencia entre la media y la ultima lectura no está comprendida en un rango marcado por la desviacion,
    //es que puede existir un posible error
    
    return error;
}

/*
    Programa principal que se encarga de leer la luz ambiental y compensar con un led esta cantidad de luz
    Funcionalidades extra:
        - Muestra periodicamente por pantalla la luz (capturada y compensada por LED) en forma de porcentajes.
        - Detecta errores y activa un buzzer, indicando el error.
        - Permite hacer la media de los datos captados durante 10 segundos aprentando un boton. Una vez pasados los diez segundos, 
        se muestran los datos medios.
        - Permite escoger la cantidad de luz a compensar mediante un potenciometro.
*/
int main()
{
    printf("\n\n============================ STR Practica 1: Compensador de llum ============================\n\n");

    float datos[3] = {0, 0, 0};
    AvCalculator.start(callback(AvCalc_Thread));    //Iniciamos thread
    LinPotenciometer.start(callback(LinPot_Thread));
    AvButton.rise(&AvCalc_Interrupcion);
    timer.attach(&LinPot_Interrupcion, FrecPotenciometro);

    while (true) {
        if (comprobarFuncionamientoSensor(Photoresistor)==true){
            printf("Error en el funcionamiento del fotoresistor. Revisa si el pin esta conectado");
            exit(0);
        }
        datos[1] = Photoresistor.read();   //Leemos % de luz en el ambiente
        luxCalc(datos);                    //Calculamos nivel de luminancia y % de compensacion por el LED
        Led.write(datos[2]);
        printf("Nivel de iluminancia (lux): %.2f \t\tLuz ambiente: %.2f %% \t\tCompensación led: %.2f %% ", datos[0], datos[1]*100, datos[2]*100);
        printf("\tCantidad de luz a compensar: %f %%\n", lindar*100);
        ThisThread::sleep_for(1s);                       //Esperamos 1 segundos entre lecturas
    }
}

/*
AvCalc_Interrupcion
    Interrupcion que se encarga de despertar el thread encargado de hacer la media de datos
*/
void AvCalc_Interrupcion(){
   osSignalSet(AvCalculatorID, 0x01);
}

/*
AvCalc_Thread
    Funcion que se ejecuta como thread. Se encarga de calcular la media de los datos durante 10 segundos, usando para ello un muestreo de 100 datos
*/
void AvCalc_Thread(){
    AvCalculatorID = osThreadGetId();
    float average, lectura, datos[3];
    for(;;){
        osSignalWait(0x01, osWaitForever);
        timer.detach();
        printf("\nComenzamos a calcular la media de los siguientes 10 segundos (%u muestras)\n", FrecMuestreo);
        average=0;
        for(int i=0; i<FrecMuestreo; i++){
            lectura=Photoresistor.read();
            average=average+lectura;
            wait_us((10/(float)FrecMuestreo)*1000000);
        }
        datos[1]=average/FrecMuestreo;
        printf("\nRegistro de la media de los datos en los últimos 10 segundos:\n");
        luxCalc(datos);
        printf("\tNivel de iluminancia (lux): %.2f\t\tLuz ambiente: %.2f %% \t\tCompensación led: %.2f %% \n\n", datos[0], datos[1]*100, datos[2]*100);
        timer.attach(&LinPot_Interrupcion, FrecPotenciometro);
    }
}

/*
    LinPot_Interrupcion
    Interrupcion que se encarga de despertar el thread encargado de comprobar el potenciometro
*/
void LinPot_Interrupcion(){
    osSignalSet(LinPotID, 0x01);
}

/*
  LinPot_Thread
  Funcion que se encargará de calcular el lindar de luz a compensar en funcion al valor del potenciometro
  Se actualizará en funcion de la constante FrecPotenciometro  
 */
void LinPot_Thread(){
    LinPotID = osThreadGetId();
    for(;;){
        if (comprobarFuncionamientoSensor(Potentiometer)==true){
            printf("Error en el funcionamiento del potenciometro. Revisa si el pin esta conectado");
            exit(0);
        }
        //printf("\t***log: Comprobamos valor del potenciometro\n");
        stdio_mutex.lock();
        lindar = Potentiometer.read();
        stdio_mutex.unlock();   
        osSignalWait(0x01, osWaitForever);  //Wait al final de la iteracion para hacer la comprobación del funcionamiento nada mas empezar el thread   
    }
}