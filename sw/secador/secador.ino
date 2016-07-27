/*
 * Código para leitura e controle da temperatura do secador.
 * Utiliza um sensor LM35 para medir a temperatura.
 * O controle da temperatura se dá através de uma abertura de ar frio, acionada por um motor DC.
 * O valor da temperatura é mostrado em um display de 7 segmentos.
 */



// Bibliotecas utilizadas no projeto
#include <LedControl.h>
#include <EEPROM.h>
#include <TimerOne.h>

// Variáveis utilizadas no programa. byte +-128
int temp_max = 120, temp_min = 110, temp_alarme = 125; //Valores default para primeira inicialização.
byte segundo = 0, temperatura = 0, janela_fechada = 0, janela_aberta = 0, alarme = 0;
int i = 0, alarmecount = 0;

// Pinos utilizados
// Rele para abrir, fechar e sirene;
// Botão para menu, decrementar e incrementar;
int rele_abre = 3, rele_fecha = 5, rele_alarme = 7, input_menu = 8, input_dec = 9, input_inc = 10;
int fim_curso_abre = 4, fim_curso_fecha = 6;

void setup() 
{
  // initialize serial communications at 9600 bps:
  Serial.begin(9600); // testa com 9600 mas depois aumenta para o máximo que der, para o uC não perder tempo escrevendo
  Serial.print("Iniciando a configuração\n");
  inicializa_timer1();
  Serial.print("Inicialização Timer1\n");
  inicializa_gpio();
  Serial.print("Inicialização GPIO\n");
  inicializa_eeprom();
  Serial.print("Inicialização EEPROM\n");
  inicializa_display();
  Serial.print("Inicialização Display\n");
}

void loop() {
  // put your main code here, to run repeatedly:
  verifica_menu();

  // Roda a rotina da temperatura somente a cada segundo, a dinâmica da temperatura é lenta e não necessita taxa maior.
  if(segundo == 1)
  {
    segundo = 0;

    // LM35
    // VOUT = 1500 mV at 150°C
    // VOUT = 250 mV at 25°C
    // VOUT = –550 mV at –55°C
    // Fundo de escala em 5V = 500°C = 1023;
    // Então para passar para °C a medição, multiplica por 500 e depois divide por 1023
    temperatura = (analogRead(0)*500) / 1023; // Lê o canal 0
    Serial.print("Temperatura = ");
    Serial.print(temperatura);
    Serial.print("\n");

    // Tempertaura menor que a mínima fecha a janela
    if(temperatura < temp_min)
    {
      if(janela_fechada == 0) // flag para indicar que a janela já está fechada e não precisa entrar aqui.
      {
        janela_fechada = 1;
        janela_aberta = 0;
        Serial.print("Fechando Ar\n"); 
        digitalWrite(rele_abre, LOW); // Desliga o outro rele para garantir que não vai dar curto na fonte.
        delay(300); // Aguarda 300ms para garantir que o rele está desligado.
        digitalWrite(rele_fecha, HIGH);
        while(digitalRead(fim_curso_fecha)==1){
          } //Fica preso aqui até que o input se torne 0.
        digitalWrite(rele_fecha, LOW);
        Serial.print("Janela fechada\n"); 
      }
    }

    // Tempertaura maior que a máxima abre a janela
    if(temperatura > temp_max)
    {
      if(janela_aberta == 0) // flag para indicar que a janela já está aberta e não precisa entrar aqui.
      {
        janela_aberta = 1;
        janela_fechada = 0;
        Serial.print("Abrindo Ar\n"); 
        digitalWrite(rele_fecha, LOW); // Desliga o outro rele para garantir que não vai dar curto na fonte.
        delay(300); // Aguarda 300ms para garantir que o rele está desligado.
        digitalWrite(rele_abre, HIGH);
        while(digitalRead(fim_curso_fecha)==1){} //Fica preso aqui até que o input se torne 0.
        digitalWrite(rele_abre, LOW);
        Serial.print("Janela aberta\n"); 
      }
    }

    // Acionamento da sirene
    if(temperatura > temp_alarme)
    {
      if(alarme == 0) // flag para indicar que já tocou o alarme.
      {
        alarme = 1;
        Serial.print("Acionando Alarme\n"); 
        digitalWrite(rele_alarme, HIGH); // Liga o alarme.
        alarmecount = 10;
      }
    }else{
      alarme = 0;
    }

    
  }
  
}

/// --------------------------
/// Interrupção em 200ms
/// --------------------------
void timerIsr()
{
  ///// Conta 1 segundo. //////
    i++; // incrementa i a cada 200ms;
    if(i == 5)
    {
      segundo = 1;
      i = 0;
    }
  /////////////////////////////////////

    if (alarmecount > 0){
      alarmecount--;
    }else{
      digitalWrite(rele_alarme, LOW); // Desliga o alarme depois de zerar alarmecount.
    }


    
}



/// --------------------------
/// Menu para seleção dos limites de temperatura
/// --------------------------
void verifica_menu()
{
    
}




/// --------------------------
/// Inicialização do timer
/// --------------------------
void inicializa_timer1()
{
  Timer1.initialize(200000); // set a timer of length 200000 microseconds (or 0.2 sec - or 5Hz)
  Timer1.attachInterrupt( timerIsr ); // attach the service routine here
}


/// --------------------------
/// Inicialização dos pinos digitais
/// --------------------------
void inicializa_gpio() // Verificar os pinos escolhidos
{  
  pinMode(rele_abre, OUTPUT);
  digitalWrite(rele_abre, LOW);
  pinMode(rele_fecha, OUTPUT);
  digitalWrite(rele_fecha, LOW);
  pinMode(rele_alarme, OUTPUT);
  digitalWrite(rele_alarme, LOW); 
  
  pinMode(fim_curso_abre, INPUT);
  pinMode(fim_curso_fecha, INPUT);
  pinMode(input_menu, INPUT);
  pinMode(input_dec, INPUT);
  pinMode(input_inc, INPUT);
}

/// --------------------------
/// Inicialização da EEPROM
/// --------------------------
void inicializa_eeprom()
{  
// Inicializa os limites de temperatura para o primeiro uso
if(EEPROM.read(0) == 78){ // Se está escrito 5 no endereço 0 é porque o uC já foi inicializado, então lê os valores da memória.
  temp_max = EEPROM.read(1);
  temp_min = EEPROM.read(2);
  temp_alarme = EEPROM.read(3);  
  }
  else // Senão inicializa com os seguintes valores
  {
  EEPROM.write(0,78); // Valor para identificar que o uC já foi inicializado antes.
  EEPROM.write(1,temp_max);
  EEPROM.write(2,temp_min);
  EEPROM.write(3,temp_alarme);
  }
  
  Serial.print("temp_max = ");
  Serial.print(temp_max);
  Serial.print("\n");
  Serial.print("temp_min = ");
  Serial.print(temp_min);
  Serial.print("\n");
  Serial.print("temp_alarme = ");
  Serial.print(temp_alarme);
  Serial.print("\n");
}

/// --------------------------
/// Inicialização do display
/// --------------------------

// Configurar conforme os pinos do hardware obedecendo a ordem abaixo.
// inputs: DIN pin, CLK pin, LOAD (CS) pin. number of chips
LedControl mydisplay = LedControl(2, 1, 0, 1);

void inicializa_display()
{
  // Inicialização do Display  
  mydisplay.shutdown(0, false);  // turns on display
  mydisplay.setIntensity(0, 15); // 0 a 15 = brightest
  mydisplay.setScanLimit(0,7); // de 0 a 7 Indica quantos dígitos serão ligados.
  mydisplay.setDigit(0, 0, 0, false); //setDigit(int addr, int digit, byte value, boolean dp)
  mydisplay.setDigit(0, 1, 0, false); // setDigit(chip, posição, número, naosei)
  mydisplay.setDigit(0, 2, 0, false);
  mydisplay.setDigit(0, 3, 0, false); //testar com true
  mydisplay.setDigit(0, 4, 0, false);
  mydisplay.setDigit(0, 5, 0, false);
  mydisplay.setDigit(0, 6, 0, false);
  mydisplay.setDigit(0, 7, 0, false);
//  mydisplay.clearDisplay(0); // Deve fazer a mesma coisa que o escrito acima.
///////////////////////////////////
}
