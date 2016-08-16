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
////////// #include <LiquidCrystal.h> // http://blog.filipeflop.com/display/controlando-um-lcd-16x2-com-arduino.html

///////////// Pinos utilizados  /////////////////////
// inputs: DIN pin, CLK pin, LOAD (CS) pin. number of chips
LedControl mydisplay = LedControl(2, 1, 0, 1);

// Rele para abrir, fechar e sirene, fim de curso;
int rele_abre1 = 3, rele_abre2 = 4, fim_curso_abre = 8, rele_fecha1 = 5, rele_fecha2 = 6, fim_curso_fecha = 9, rele_alarme = 7;

// Botão para menu, decrementar e incrementar;
int input_menu = 10, input_dec = 11, input_inc = 12;

// Define os pinos que serão utilizados para ligação ao display
// LiquidCrystal lcd(<pino RS>, <pino enable>, <pino D4>, <pino D5>, <pino D6>, <pino D7>)
////////// LiquidCrystal lcd(12, 11, A5, A4, A3, A2); // para os pinos de dados será necessário utilizar os canais analógicos como digital.
//////////////////////////////////////////////////////


// Variáveis utilizadas no programa. byte +-128
int temp_ref = 120, temp_alarme_min = 110, temp_alarme_max = 125; //Valores default para primeira inicialização.
byte segundo = 0, alarme = 0, set_alarme = 0, digito = 0, pisca_display = 0, menu_select = 0;
int i = 0, alarmecount = 0, pisca_count = 0, pisca_delay = 0;
long temperatura = 0;


void setup() 
{
  //lcd.begin(16, 2);
  //lcd.clear(); //Limpa a tela
  //lcd.setCursor(0, 0); //Posiciona o cursor na coluna 0, linha 0;
  //lcd.print("Display"); //Envia o texto entre aspas para o LCD
  //lcd.setCursor(0, 1); //Posiciona o cursor na coluna 0, linha 1;
  //lcd.print("Inicializado"); //Envia o texto entre aspas para o LCD
  
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
    temperatura = (analogRead(0)*500L) / 1023; // Lê o canal 0
    Serial.print("Temperatura = ");
    Serial.print(temperatura);
    Serial.print("C\n");

    // Escreve temperatura no display
    digito = temperatura / 100;
    mydisplay.setDigit(0, 2, digito, false); // centena
    digito = (temperatura % 100 ) / 10;
    mydisplay.setDigit(0, 1, digito, false); // dezena
    digito = (temperatura % 100 ) % 10;    
    mydisplay.setDigit(0, 0, digito, false); // unidade

    // Tempertaura menor que a mínima fecha a janela
    if(temperatura < (temp_ref - 2))
    {
      if(digitalRead(fim_curso_fecha) == 1)         // Verifica se a janela ainda não está fechada.
      {
        Serial.print("Fechando Ar\n"); 
        digitalWrite(rele_abre1, HIGH);              // Desliga o outro rele para garantir que não vai dar curto na fonte.
        digitalWrite(rele_abre2, HIGH);              // Desliga o outro rele para garantir que não vai dar curto na fonte.
        delay(300);                                 // Aguarda 300ms para garantir que o rele está desligado.
        digitalWrite(rele_fecha1, LOW);              // Liga o relé para fechar. 
        digitalWrite(rele_fecha2, LOW);              // Liga o relé para fechar.
        while(digitalRead(fim_curso_fecha)==1){}    // Fica preso aqui até que o input se torne 0.
        digitalWrite(rele_fecha1, HIGH);             // Desliga o relé depois de atingir o fim de curso.
        digitalWrite(rele_fecha2, HIGH);             // Desliga o relé depois de atingir o fim de curso.
        Serial.print("Janela fechada\n"); 
      }
    }

    // Tempertaura maior que a máxima abre a janela
    if(temperatura > (temp_ref + 2))
    {
      if(digitalRead(fim_curso_abre) == 1)          // Verifica se a janela ainda não está aberta.
      {
        Serial.print("Abrindo Ar\n"); 
        digitalWrite(rele_fecha1, HIGH);             // Desliga o outro rele para garantir que não vai dar curto na fonte.
        digitalWrite(rele_fecha2, HIGH);             // Desliga o outro rele para garantir que não vai dar curto na fonte.
        delay(300);                                 // Aguarda 300ms para garantir que o rele está desligado.
        digitalWrite(rele_abre1, LOW);               // Liga o relé para abrir.
        digitalWrite(rele_abre2, LOW);               // Liga o relé para abrir.
        while(digitalRead(fim_curso_abre)==1){}     // Fica preso aqui até que o input se torne 0.
        digitalWrite(rele_abre1, HIGH);              // Desliga o relé depois de atingir o fim de curso.
        digitalWrite(rele_abre2, HIGH);              // Desliga o relé depois de atingir o fim de curso.
        Serial.print("Janela aberta\n"); 
      }
    }

    // Acionamento da sirene
    if(temperatura == temp_ref){
      set_alarme = 1;
    }
    if(set_alarme == 1){    // flag para indicar que já chegou a temperatura de operação pelo menos uma vez e não acionar o alarme logo ao ligar.
      if(temperatura > temp_alarme_max)
      {
        if(alarme == 0) // flag para indicar que já tocou o alarme.
        {
          alarme = 1;
          Serial.print("Acionando Alarme\n"); 
          digitalWrite(rele_alarme, LOW); // Liga o alarme.
          alarmecount = 11;
        }
      }else{
        if(temperatura < temp_alarme_min)
        {
          if(alarme == 0) // flag para indicar que já tocou o alarme.
          {
            alarme = 1;
            Serial.print("Acionando Alarme\n"); 
            digitalWrite(rele_alarme, LOW); // Liga o alarme.
            alarmecount = 11;
          }
        }else{
          alarme = 0;
        } 
      }
    }
  }
}
/// --------------------------
/// Interrupção em 200ms
/// --------------------------
void timerIsr()
{
  boolean piscapisca = false;
  
  ///// Conta 1 segundo. //////
    i++; // incrementa i a cada 200ms;
    if(i == 5)
    {
      segundo = 1;
      i = 0;
    }
  /////////////////////////////////////
   // Contador para desligar o alarme.
    if (alarmecount > 0){
      alarmecount--;
    }
    if (alarmecount == 1){
      digitalWrite(rele_alarme, HIGH);        // Desliga o alarme depois de zerar alarmecount.
      Serial.print("Alarme desligado\n");     // Escreve na tela apenas uma vez.
    }
    

  //////////////////////////////
  // código para piscar o display de acordo com o menu selecionado.
  if(pisca_display == 1)
  {
    // Delay para piscar em frequência diferente de acordo com o menu.
    pisca_count++;
    if(pisca_count > pisca_delay)
    {
      piscapisca = !piscapisca;
      mydisplay.shutdown(0, piscapisca);  // alterna entre true e false
      pisca_count = 0;
    }
  }else 
  {
    mydisplay.shutdown(0, false);  // turns on display
    pisca_count = 0;
  }
    
}



/// --------------------------
/// Menu para seleção dos limites de temperatura
/// --------------------------
// input_menu = 8, input_dec = 9, input_inc = 10;
void verifica_menu()
{
  
  // caso apertou botão menu pela primeira vez, incrementa a seleção do menu, pisca display e entra no while abaixo até o botão ser solto.
  if(digitalRead(input_menu) == 1){ 
    while(digitalRead(input_menu) == 1){} // aguarda o botão ser solto
    Serial.print("MENU\n");
    menu_select++;
    pisca_display = 1;
  }
  
  // Fica aqui dentro até passar por todo menu, voltando a ser zero.
  while(menu_select > 0)
  {
    switch (menu_select) //case com o menu select para selecionar o que será alterado
    { 
      case 1: // Seleção da temperatura de operação.
        Serial.print("Seleção da temperatura de operação\n");
        Serial.print("Valor: ");
        Serial.print(temp_ref);
        Serial.print("C\n");
        pisca_delay = 5; // Indica a taxa que irá piscar o display na função do timer para representar o menu_select.
        // Escreve temperatura de operação atual no display.
        digito = temp_ref / 100;
        mydisplay.setDigit(0, 2, digito, false); // centena
        digito = (temp_ref % 100 ) / 10;
        mydisplay.setDigit(0, 1, digito, false); // dezena
        digito = (temp_ref % 100 ) % 10;    
        mydisplay.setDigit(0, 0, digito, false); // unidade
        delay(100); //delay apenas para evitar que o display fique piscando enquanto não aperta nenhum botão.
        
        if(digitalRead(input_dec) == 1){ // Verifica se apertou o botão para decrementar
          delay(100); //Aguarda 100ms para possibilitar modificar continuamente.
          temp_ref--;
        }  
                   
        if(digitalRead(input_inc) == 1){ // Verifica se apertou o botão para incrementar
          delay(100); //Aguarda 100ms para possibilitar modificar continuamente.
          temp_ref++;
        }        
       
        break;
      case 2: // Seleção da temperatura máxima para acionar o alarme
        Serial.print("Seleção da temperatura máxima para acionar o alarme.\n");
        Serial.print("Valor: ");
        Serial.print(temp_alarme_max);
        Serial.print("C\n");
        pisca_delay = 3; // Indica a taxa que irá piscar o display na função do timer para representar o menu_select.
        // Escreve temperatura máxima para acionar o alarme atual no display
        digito = temp_alarme_max / 100;
        mydisplay.setDigit(0, 2, digito, false); // centena
        digito = (temp_alarme_max % 100 ) / 10;
        mydisplay.setDigit(0, 1, digito, false); // dezena
        digito = (temp_alarme_max % 100 ) % 10;    
        mydisplay.setDigit(0, 0, digito, false); // unidade
        delay(100); //delay apenas para evitar que o display fique piscando enquanto não aperta nenhum botão.
        
        if(digitalRead(input_dec) == 1){ // Verifica se apertou o botão para decrementar
          delay(100); //Aguarda 100ms para possibilitar modificar continuamente.
          temp_alarme_max--;
        }  
                   
        if(digitalRead(input_inc) == 1){ // Verifica se apertou o botão para incrementar
          delay(100); //Aguarda 100ms para possibilitar modificar continuamente.
          temp_alarme_max++;
        }  
        
        break;
        case 3: // Seleção da temperatura mínima para acionar o alarme
        Serial.print("Seleção da temperatura mínima para acionar o alarme.\n");
        Serial.print("Valor: ");
        Serial.print(temp_alarme_min);
        Serial.print("C\n");
        pisca_delay = 0; // Indica a taxa que irá piscar o display na função do timer para representar o menu_select.
        // Escreve temperatura alarme atual no display
        digito = temp_alarme_min / 100;
        mydisplay.setDigit(0, 2, digito, false); // centena
        digito = (temp_alarme_min % 100 ) / 10;
        mydisplay.setDigit(0, 1, digito, false); // dezena
        digito = (temp_alarme_min % 100 ) % 10;    
        mydisplay.setDigit(0, 0, digito, false); // unidade
        delay(100); //delay apenas para evitar que o display fique piscando enquanto não aperta nenhum botão.
        
        if(digitalRead(input_dec) == 1){ // Verifica se apertou o botão para decrementar
          delay(100); //Aguarda 100ms para possibilitar modificar continuamente.
          temp_alarme_min--;
        }  
                   
        if(digitalRead(input_inc) == 1){ // Verifica se apertou o botão para incrementar
          delay(100); //Aguarda 100ms para possibilitar modificar continuamente.
          temp_alarme_min++;
        }  
        
        break;
      default: 
        // if nothing else matches, do the default
        // default is optional
      break;
    }  

    // caso apertou botão, incrementa a seleção do menu
    if(digitalRead(input_menu) == 1)
    { 
      while(digitalRead(input_menu) == 1){} // aguarda o botão ser solto
      menu_select++;
      if(menu_select>3) // chegou ao final do menu.
      {
        Serial.print("Final do Menu, gravando valores\n");
        Serial.print("Temperatura de Operação: ");
        Serial.print(temp_ref);
        Serial.print("C\n");
        Serial.print("Temperatura mínima de acionamento do alarme: ");
        Serial.print(temp_alarme_min);
        Serial.print("C\n");
        Serial.print("Temperatura máxima de acionamento do alarme: ");
        Serial.print(temp_alarme_max);
        Serial.print("C\n");
        menu_select = 0; // sai do menu, para de piscar o display e grava os valores na memória.
        pisca_display = 0;
        EEPROM.write(1,temp_ref);
        EEPROM.write(2,temp_alarme_min);
        EEPROM.write(3,temp_alarme_max);
        Serial.print("Dados gravados!\n");
      }
    }

  }



    
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
  pinMode(rele_abre1, OUTPUT);
  digitalWrite(rele_abre1, HIGH);
  pinMode(rele_abre2, OUTPUT);
  digitalWrite(rele_abre2, HIGH);
  pinMode(rele_fecha1, OUTPUT);
  digitalWrite(rele_fecha1, HIGH);
  pinMode(rele_fecha2, OUTPUT);
  digitalWrite(rele_fecha2, HIGH);
  pinMode(rele_alarme, OUTPUT);
  digitalWrite(rele_alarme, HIGH); 
  
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
if(EEPROM.read(0) == 78){ // Se está escrito 78 no endereço 0 é porque o uC já foi inicializado, então lê os valores da memória.
  temp_ref = EEPROM.read(1);
  temp_alarme_min = EEPROM.read(2);
  temp_alarme_max = EEPROM.read(3);  
  }
  else // Senão inicializa com os seguintes valores
  {
  EEPROM.write(0,78); // Valor para identificar que o uC já foi inicializado antes.
  EEPROM.write(1,temp_ref);
  EEPROM.write(2,temp_alarme_min);
  EEPROM.write(3,temp_alarme_max);
  }
  
  Serial.print("temp_ref = ");
  Serial.print(temp_ref);
  Serial.print("\n");
  Serial.print("temp_alarme_min = ");
  Serial.print(temp_alarme_min);
  Serial.print("\n");
  Serial.print("temp_alarme_max = ");
  Serial.print(temp_alarme_max);
  Serial.print("\n");
}

/// --------------------------
/// Inicialização do display
/// --------------------------

// Configurar conforme os pinos do hardware obedecendo a ordem abaixo.

void inicializa_display()
{
  // Inicialização do Display  
  mydisplay.shutdown(0, false);  // turns on display
  mydisplay.setIntensity(0, 15); // 0 a 15 = brightest
  mydisplay.setScanLimit(0,2); // de 0 a 7 Indica quantos dígitos serão ligados.
  mydisplay.setDigit(0, 0, 0, false); //setDigit(int addr, int digit, byte value, boolean dp)
  mydisplay.setDigit(0, 1, 0, false); // setDigit(chip, posição, número, dot)
  mydisplay.setDigit(0, 2, 0, false);
//  mydisplay.clearDisplay(0); // Deve fazer a mesma coisa que o escrito acima.
///////////////////////////////////
}
