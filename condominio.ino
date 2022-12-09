// C++ code
#include <Ethernet.h> //responsável pela comunicação que será estabelecida entre o Arduino UNO e a rede através do Shield ethernet 
#include <SPI.h> //tem como função coordenar a integração do shield citado com o Arduino UNO por meio do protocolo de comunicação SPI
#include <MySQL_Connection.h> //promover a conexão Arduino UNO/servidor MySQL e executar as sentenças de manipulação de informações em um banco de dados
#include <MySQL_Cursor.h> //promover a conexão Arduino UNO/servidor MySQL e executar as sentenças de manipulação de informações em um banco de dados


//DECLARAÇÃO DE INPUTS
#define SenAgua A0 //Fotorresistor representando um Sensor de Agua, cabo Turquesa
#define SenLuz A1 //Fotorresistor representando um Sensor de Luz, cabo Roxo
#define ButAgua 2 //Botão Agua, Cabo Rosa
#define ButLuz 3 //Botão Luz, Cabo Rosa
#define SenPres 4 //Sensor de Presença, cabo Marrom

//DECLARAÇÃO DE OUTPUTS
#define LedAgua 12 //Led informativo do fluxo de agua corrente, cabo Azul
#define LedLuz 11 //Led informativo de consumo de energia, cabo Amarelo
#define ReleAgua 8 //Rele representando uma válvula solenóide, cabo Roxo
#define ReleLuz 9 //Rele, cabo Rosa
#define Lampada 5 //Led representando uma lampada, cabo Verde


//Os leds conectados aos reles não tem ligação em portas arduíno


//configurando conexao rede
byte mac_addr[] = { 0x00, 0x27, 0x13, 0xAE, 0x79, 0x0F }; //informar MAC host
//IPAddress ip(192,168,1,88);          //~Define o endereco IP da rede lan
//IPAddress gateway(192,168,1,1);     //~Define o gateway da rede lan
//IPAddress subnet(255, 255, 255, 0); //~Define a máscara de rede da rede lan

//acesso ao BD
IPAddress server_addr(192,168,0,112); //informar ip banco de dados
char user[] = "arduino"; //usuário banco
char password[] = "arduino123"; //senha banco


//STRING PARA BANCO
char INSERIR_AGUA[] = "INSERT INTO agua (quant_consumo, id_casa) VALUES (%s, 1)"; //inserir numero da casa manual
char INSERIR_LUZ[] = "INSERT INTO energia (quant_consumo, id_casa) VALUES (%s, 1)"; //inserir numero da casa manual
char STATUS_BOTAOAGUA[] = "SELECT (status_agua) FROM casa WHERE id_casa = 1"; //inserir numero da casa manual
char STATUS_BOTAOLUZ[] = "SELECT (status_energia) FROM casa WHERE id_casa = 1"; //inserir numero da casa manual
char BANCODEDADOS[] = "USE projeto_condominio"; //Escolhe o banco

//Instanciando objetos
EthernetClient client; //responsável pela comunicação via ethernet
MySQL_Connection conn((Client *)&client); //será responsável pela conexão com o servidor MySQL 


//global sql
int leitura; 
float leituraconvertida;
char sentenca[128]; //isso vai puro para o banco

//global millis
unsigned long int tempo_atual = 0;
unsigned long ultimo_tempo = 0;
unsigned long leituramillis = millis();
unsigned long controlemillis = millis();
unsigned long bancomillis = millis();


//agua sql
char valorAgua[10];
int lerbancoagua;

//luz sql
char valorLuz[10];
int lerbancoluz; 

//Variaveis Globais de leitura de sensores
volatile byte valorBotaoAgua;
volatile byte valorBotaoLuz;
volatile byte estadoBotaoAgua;
volatile byte estadoBotaoLuz;
volatile byte valorPresenca;


//---------------------------------------------------------------------------------------------------------------------

void setup() 
{ 
   //Não só estes procedimentos, como todos os outros que envolvem a manipulação da porta serial, servem apenas para testes, isto é, para que você possa verificar se o programa está com algum tipo de erro ou se está funcionando perfeitamente, portanto, as próximas duas linhas referentes aos mesmos podem ser comentadas caso o programa esteja funcionando da maneira desejada.
   Serial.begin(9600); //iniciar comunicação serial
   while (!Serial); //para que o programa só continue sendo executado após a comunicação serial ter sido estabelecida
 
   //conecta
   Ethernet.begin(mac_addr);
   Serial.println("Conectando...");

  
  // CONEXAO AO BANCO
   if (conn.connect(server_addr, 3306, user, password)) //atenção na porta do banco 
   {
      delay(1000);
      Serial.println("Conectado ao SQL");
      digitalWrite(LED_BUILTIN, HIGH);
      
      MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
      cur_mem->execute(BANCODEDADOS);
      delete cur_mem;     
   }
   else
   {
      Serial.println("A conexão falhou");
      conn.close();
      digitalWrite(LED_BUILTIN, LOW);
      delay(1000);
      Serial.println("Conectando ao SQL novamente");
   }


   pinMode(SenAgua, INPUT);
   pinMode(SenLuz, INPUT);
   pinMode(ButAgua, INPUT);
   pinMode(ButLuz, INPUT);
   pinMode(SenPres, INPUT);
   pinMode(LedAgua, OUTPUT);
   pinMode(LedLuz, OUTPUT);
   pinMode(ReleAgua, OUTPUT);
   pinMode(ReleLuz, OUTPUT);
   pinMode(Lampada, OUTPUT);
  
}

//---------------------------------------------------------------------------------------------------------------------

void loop() 
{
  
   leituradesensor();
  
   controledesensor();

   bancodesensor();

   delay(100);

}


void leituradesensor(){
   
   if((millis() - leituramillis) > 100){
   LendoSensor1(); //botão
   delay(500);
   LendoSensor2(); //botão
   delay(500);
   LendoSensor3(); //presença
   delay(500);
   leituraBancoAgua(); //leitutabotaoweb
   delay(500);
   leituraBancoLuz(); //leitutabotaoweb
   delay(500);
   ledaguamonitor();
   delay(500);
   ledluzmonitor();
   delay(500);
  }
  if((millis() - leituramillis) > 100){
    leituramillis = millis();
  }

}

void controledesensor(){

  if((millis() - controlemillis) > 1000){
   ControleAgua(); //rele
   delay(500);
   ControleLuz(); //rele   
   delay(500);
   AcendeLampada(); //lampada
   delay(500);
  }
  if((millis() - controlemillis) > 1000){
      controlemillis = millis();
  }
}

void bancodesensor(){
  if((millis() - bancomillis) > 60000){
   leituraAgua(); //leitura para banco / led
   delay(1000);
   leituraLuz(); //leitura para banco / led
   delay(1000);
  }
  if((millis() - bancomillis) > 60000){
      bancomillis = millis();
  }
}


void LendoSensor1(){
 estadoBotaoAgua = digitalRead(ButAgua);
 if (estadoBotaoAgua == HIGH) valorBotaoAgua=!valorBotaoAgua;
Serial.print("Agua Corte Status: ");
Serial.println(valorBotaoAgua);
delay(1000);
}

void LendoSensor2(){
estadoBotaoLuz = digitalRead(ButLuz);
 if (estadoBotaoLuz == HIGH) valorBotaoLuz=!valorBotaoLuz;
Serial.print("Luz Corte Status: ");
Serial.println(valorBotaoLuz);
delay(1000);
}

void LendoSensor3(){
valorPresenca = digitalRead(SenPres);

Serial.print("Presenca: ");
Serial.println(valorPresenca);

delay(1000);

}

void ControleAgua(){ 
  if (valorBotaoAgua == HIGH ^ lerbancoagua == 1) digitalWrite(ReleAgua, HIGH); //USO DA PORTA LÓGICA XOR POIS SÃO DOIS INTERRUPTORES
   else digitalWrite(ReleAgua, LOW);
  delay(50);
}    

void ControleLuz(){ 
  if (valorBotaoLuz == HIGH ^ lerbancoluz == 1) digitalWrite(ReleLuz, HIGH); //USO DA PORTA LÓGICA XOR POIS SÃO DOIS INTERRUPTORES
   else digitalWrite(ReleLuz, LOW);
  delay(50);
}    

void AcendeLampada(){
    tempo_atual = millis();
  
    if (valorPresenca == HIGH){ 
    ultimo_tempo = tempo_atual;
    digitalWrite(Lampada, HIGH);  
    }  
           
  	if ((valorPresenca == LOW) && (tempo_atual - ultimo_tempo >= 10000)){       //dez segundos ligado   
    digitalWrite(Lampada, LOW);
    }
}   

void leituraAgua(){
   ledaguamonitor();
   dtostrf(leituraconvertida, 6, 2, valorAgua); //converte o valor da leitura para uma char para poder enviar ao banco, existem quatro parâmetros, onde estes são: a variável do tipo float que queremos converter, o número de algarismos que o resultado da conversão deve ter (incluindo a vírgula), o número de casas após a vírgula e o vetor de char em que será armazenada a informação retornada por esta função
   sprintf(sentenca, INSERIR_AGUA, valorAgua); //construir a sentença contendo a instrução que será utilizada na manipulação do banco de dados. O primeiro parâmetro desta função consiste na variável do tipo char na qual será armazenada a sentença após ser construída, o segundo, deve conter a variável que armazenou a parte textual da frase e o local onde será inserida a variável (%s) e o terceiro parâmetro é justamente a variável que será inserida no local especificado no segundo parâmetro   
   enviaDados();
  
}

void leituraLuz(){
   ledluzmonitor();
   dtostrf(leituraconvertida, 6, 2, valorLuz); //converte o valor da leitura para uma char para poder enviar ao banco, existem quatro parâmetros, onde estes são: a variável do tipo float que queremos converter, o número de algarismos que o resultado da conversão deve ter (incluindo a vírgula), o número de casas após a vírgula e o vetor de char em que será armazenada a informação retornada por esta função
   sprintf(sentenca, INSERIR_LUZ, valorLuz); //construir a sentença contendo a instrução que será utilizada na manipulação do banco de dados. O primeiro parâmetro desta função consiste na variável do tipo char na qual será armazenada a sentença após ser construída, o segundo, deve conter a variável que armazenou a parte textual da frase e o local onde será inserida a variável (%s) e o terceiro parâmetro é justamente a variável que será inserida no local especificado no segundo parâmetro
   enviaDados();

}


void ledaguamonitor(){
  Serial.println("Executando leitura");
   leitura = analogRead(SenAgua); //recebe valor do sensor
   leituraconvertida = (float(analogRead(SenAgua))/2.28); //converte os valores de sensor para valores reais, 
   Serial.print("Agua: "); //mostra leitura
   Serial.println(round(leituraconvertida)); //mostra leitura 
    if(leituraconvertida>45) digitalWrite(LedAgua, HIGH); 
    else digitalWrite(LedAgua, LOW);
}

void ledluzmonitor(){
   Serial.println("Executando leitura");
   leitura = analogRead(SenLuz); //recebe valor do sensor
   leituraconvertida = (float(analogRead(SenLuz))/50); //converte os valores de sensor para valores reais
   Serial.print("Luz: "); //mostra leitura
   Serial.println(round(leituraconvertida)); //mostra leitura
     if(leituraconvertida>2.05) digitalWrite(LedLuz, HIGH);
     else digitalWrite(LedLuz, LOW);
}

void enviaDados(){	
 	MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn); //prepara para enviar ao banco
   	cur_mem->execute(sentenca); //deste vez é a sentença para incluir uma linha na tabela
    delete cur_mem; //limpa memória do arduíno após enviar ao banco
    delay(1000); //delay de um millis
}
  
void leituraBancoAgua(){
   
   row_values *row = NULL;
   delay(1000);

  MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
  cur_mem->execute(STATUS_BOTAOAGUA);
  
  column_names *columns = cur_mem->get_columns();

  do {
    row = cur_mem->get_next_row();
    if (row != NULL) {
      lerbancoagua = atol(row->values[0]);
    }
  } while (row != NULL);

  delete cur_mem;

  Serial.print("  Status Botao Agua WEB = ");
  Serial.println(lerbancoagua);

  delay(500); 
   
}
    
void leituraBancoLuz(){
   
   row_values *row = NULL;
   delay(1000);

  MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
  cur_mem->execute(STATUS_BOTAOLUZ);
  
  column_names *columns = cur_mem->get_columns();

  do {
    row = cur_mem->get_next_row();
    if (row != NULL) {
      lerbancoluz = atol(row->values[0]);
    }
  } while (row != NULL);

  delete cur_mem;

  Serial.print("  Status Botao LUZ WEB = ");
  Serial.println(lerbancoluz);

  delay(500); 
   
}


    
        
