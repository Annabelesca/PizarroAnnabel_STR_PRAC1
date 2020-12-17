#include "mbed.h"
#include <cstdio>
#include "rtos.h"


#define luxRel 500
#define VRef 3.3
#define Rl 10
#define FrecMuestreo 100

PwmOut Led(D5);
AnalogIn Photoresistor(A0);
AnalogIn Potentiometer(A2);
InterruptIn AvButton(A3);
Thread AvCalculator(osPriorityAboveNormal);
osThreadId ISRthreadId;

void AvCalc();
void AvCalculator_Thread();

float luxCalc(float lectura){
    float Vout = lectura*(VRef/1);
    return (((VRef*luxRel)*Vout)-luxRel)/Rl;
}

int main()
{
    AvCalculator.start(callback(AvCalculator_Thread));    //Iniciamos thread
    AvButton.rise(&AvCalc);                               //

    printf("\n\n======== STR Practica 1: Compensador de llum ========\n\n");
    while (true) {
        float porcLuz = Photoresistor.read();   //Leemos % de luz en el ambiente
        float lux = luxCalc(porcLuz);           //Calculamos nivel de luminancia
        float porcLed = 1-porcLuz;              //Calculamos % de luz que debe compensar el LED
        Led=porcLed;                            //Encendemos LED
        printf("Nivel de iluminancia: %.2f\t\tLuz ambiente: %.2f \t\tCompensación led: %.2f \n", lux, porcLuz*100, porcLed*100);
        wait_us(1000000);                       //Esperamos 1 segundo entre lecturas
    }
}

void AvCalc(){
   osSignalSet(ISRthreadId, 0x01);
}

void AvCalculator_Thread(){
    ISRthreadId = osThreadGetId();
    float average;
    for(;;){
        osSignalWait(0x01, osWaitForever);
        printf("\nComenzamos a calcular la media de los siguientes 10 segundos (%u muestras)", FrecMuestreo);
        average=0;
        for(int i=0; i<FrecMuestreo; i++){
            average=average+Photoresistor.read();
            wait_us((10/(float)FrecMuestreo)*1000000);
        }
        average=average/FrecMuestreo;
        printf("\nRegistro de la media de los datos en los últimos 10 segundos:\n");
        printf("\tNivel de iluminancia: %.2f\t\tLuz ambiente: %.2f \t\tCompensación led: %.2f \n\n", luxCalc(average), average*100, (1-average)*100);       
    }
}
