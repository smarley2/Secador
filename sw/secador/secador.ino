
/*
 * Valores padrão para temperatura
 * Temp_max = 120;
 * Temp_min = 110;
 * Temp_alarme = 125;
 */



// Bibliotecas utilizadas no projeto
#include <LedControl.h>
#include <EEPROM.h>

// Configurar conforme os pinos do hardware obedecendo a ordem abaixo.
// inputs: DIN pin, CLK pin, LOAD pin. number of chips
LedControl mydisplay = LedControl(45, 44, 43, 1);

byte Temp_max = 120, Temp_min = 110, Temp_alarme = 125; //Valores default para primeira inicialização.

void setup() 
{
  inicializa_eeprom();
  inicializa_display();
}

void loop() {
  // put your main code here, to run repeatedly:

}









void inicializa_eeprom()
{  
// Inicializa os limites de temperatura para o primeiro uso
if(EEPROM.read(0) == 5){ // Se está escrito 5 no endereço 0 é porque o uC já foi inicializado, então lê os valores da memória.
  Temp_max = EEPROM.read(1);
  Temp_min = EEPROM.read(2);
  Temp_alarme = EEPROM.read(3);  
  }
  else // Senão inicializa com os seguintes valores
  {
  EEPROM.write(0,5); // Valor para identificar que o uC já foi inicializado antes.
  EEPROM.write(1,Temp_max);
  EEPROM.write(2,Temp_min);
  EEPROM.write(3,Temp_alarme);
  }

}

void inicializa_display()
{
  // Inicialização do Display  
  mydisplay.shutdown(0, false);  // turns on display
  mydisplay.setIntensity(0, 15); // 0 a 15 = brightest
  mydisplay.setScanLimit(0,7); // de 0 a 7 Indica quantos dígitos serão ligados.
  mydisplay.setDigit(0, 0, 0, false); //setDigit(int addr, int digit, byte value, boolean dp)
  mydisplay.setDigit(0, 1, 0, false);
  mydisplay.setDigit(0, 2, 0, false);
  mydisplay.setDigit(0, 3, 0, false);
  mydisplay.setDigit(0, 4, 0, false);
  mydisplay.setDigit(0, 5, 0, false);
  mydisplay.setDigit(0, 6, 0, false);
  mydisplay.setDigit(0, 7, 0, false);
//  mydisplay.clearDisplay(0); // Deve fazer a mesma coisa que o escrito acima.
///////////////////////////////////
}

/* Como utilizar a biblioteca do LedControl com MAX7219
#include <LedControl.h>

// inputs: DIN pin, CLK pin, LOAD pin. number of chips
LedControl mydisplay = LedControl(45, 44, 43, 1);

void setup() {
  mydisplay.shutdown(0, false);  // turns on display
  mydisplay.setIntensity(0, 15); // 15 = brightest
  mydisplay.setDigit(0, 0, 9, false);
  mydisplay.setDigit(0, 1, 8, false);
  mydisplay.setDigit(0, 2, 7, false);
  mydisplay.setDigit(0, 3, 6, false);
  mydisplay.setDigit(0, 4, 5, true);
  mydisplay.setDigit(0, 5, 4, false);
  mydisplay.setDigit(0, 6, 3, false);
  mydisplay.setDigit(0, 7, 2, false);
}
*/

