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

//definir numero da casa
string Casa="1";

//configurando conexao rede
byte mac_addr[] = { 0x00, 0x27, 0x13, 0xAE, 0x79, 0x0F }; //informar MAC host
//IPAddress ip(192,168,1,88);          //~Define o endereco IP da rede lan
//IPAddress gateway(192,168,1,1);     //~Define o gateway da rede lan
//IPAddress subnet(255, 255, 255, 0); //~Define a máscara de rede da rede lan

//acesso ao BD
IPAddress server_addr(10,10,117,14); //informar ip banco de dados
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
int lerbanco; 

//global millis
unsigned long int tempo_atual = 0;
unsigned long ultimo_tempo = 0;


//leitura agua sql
char valorAgua[10];

//leitura luz sql
char valorLuz[10];


//Variaveis Globais de leitura de sensores
volatile byte valorBotaoAgua = LOW;
volatile byte valorBotaoLuz = LOW;
volatile byte estadoBotaoAgua = LOW;
volatile byte estadoBotaoLuz = LOW;
volatile byte valorPresenca = LOW;


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
   
  //definindo todos os leds por padrão como desligados
   digitalWrite(LedAgua, LOW);
   digitalWrite(LedLuz, LOW);
   digitalWrite(Lampada, LOW);  
  
}

//---------------------------------------------------------------------------------------------------------------------

void loop() 
{
  
  
   LendoSensor();
   ControleAgua();
   ControleLuz();
   Lampada(); 
   leituraAgua();
   leituraLuz();
   leituraBanco();

  
  
 // 	if(((millis() - controleLeitura) > esperaLeitura))
 //   	enviaDados(); 60000
 //   delay(10);
}


void LendoSensor(){
 estadoBotaoAgua = digitalRead(ButAgua);
 if (estadoBotaoAgua == HIGH) valorBotaoAgua=!valorBotaoAgua;
 
 estadoBotaoLuz = digitalRead(ButLuz);
 if (estadoBotaoLuz == HIGH) valorBotaoLuz=!valorBotaoLuz;
  
 valorPresenca = digitalRead(SenPres);

}


void ControleAgua(){ 
  if (estadoBotaoAgua == HIGH) digitalWrite(ReleAgua, valorBotaoAgua);
}    

void ControleLuz(){ 
  if (estadoBotaoLuz == HIGH) digitalWrite(ReleLuz, valorBotaoLuz);
}    
  

void Lampada(){
    tempo_atual = millis();
  
    if (valorPresenca == HIGH){ 
    ultimo_tempo = tempo_atual;
    digitalWrite(Lampada, HIGH);  
    }  
           
  	if ((valorPresenca == LOW) && (tempo_atual - ultimo_tempo >= 5000)){         
    digitalWrite(Lampada, LOW);
    }
}
    

void leituraAgua(){
   Serial.println("Executando leitura");
   leitura = analogRead(SenAgua); //recebe valor do sensor
   leituraconvertida = (float(analogRead(SenAgua))/2.28; //converte os valores de sensor para valores reais, 
   Serial.print("Agua: "); //mostra leitura
   Serial.println(round(leituraconvertida)); //mostra leitura
   dtostrf(leituraconvertida, 6, 2, valorAgua); //converte o valor da leitura para uma char para poder enviar ao banco, existem quatro parâmetros, onde estes são: a variável do tipo float que queremos converter, o número de algarismos que o resultado da conversão deve ter (incluindo a vírgula), o número de casas após a vírgula e o vetor de char em que será armazenada a informação retornada por esta função
   sprintf(sentenca, INSERIR_AGUA, valorAgua); //construir a sentença contendo a instrução que será utilizada na manipulação do banco de dados. O primeiro parâmetro desta função consiste na variável do tipo char na qual será armazenada a sentença após ser construída, o segundo, deve conter a variável que armazenou a parte textual da frase e o local onde será inserida a variável (%s) e o terceiro parâmetro é justamente a variável que será inserida no local especificado no segundo parâmetro   
   enviaDados();
  
   if(leituraconvertida>22,5) digitalWrite(LedAgua, HIGH); 
   else digitalWrite(LedAgua, LOW);
}

void leituraLuz(){
   Serial.println("Executando leitura");
   leitura = analogRead(SenLuz); //recebe valor do sensor
   leituraconvertida = (float(analogRead(SenLuz))/50; //converte os valores de sensor para valores reais
   Serial.print("Luz: "); //mostra leitura
   Serial.println(round(leituraconvertida)); //mostra leitura
   dtostrf(leituraconvertida, 6, 2, valorLuz); //converte o valor da leitura para uma char para poder enviar ao banco, existem quatro parâmetros, onde estes são: a variável do tipo float que queremos converter, o número de algarismos que o resultado da conversão deve ter (incluindo a vírgula), o número de casas após a vírgula e o vetor de char em que será armazenada a informação retornada por esta função
   sprintf(sentenca, INSERIR_LUZ, valorLuz); //construir a sentença contendo a instrução que será utilizada na manipulação do banco de dados. O primeiro parâmetro desta função consiste na variável do tipo char na qual será armazenada a sentença após ser construída, o segundo, deve conter a variável que armazenou a parte textual da frase e o local onde será inserida a variável (%s) e o terceiro parâmetro é justamente a variável que será inserida no local especificado no segundo parâmetro
   enviaDados();
  
     if(leituraconvertida>1.02) digitalWrite(LedLuz, HIGH);
     else digitalWrite(LedLuz, LOW);
}


void enviaDados(){	
 	MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn); //prepara para enviar ao banco
   	cur_mem->execute(sentenca); //deste vez é a sentença para incluir uma linha na tabela
    delete cur_mem; //limpa memória do arduíno após enviar ao banco
    delay(1000); //delay de um millis
}

    
void leituraBanco(){
   
   row_values *row = NULL;
   //long lerbanco = 0;
   delay(1000);

  MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
  cur_mem->execute(STATUS_BOTAOAGUA);
  
  column_names *columns = cur_mem->get_columns();

  do {
    row = cur_mem->get_next_row();
    if (row != NULL) {
      lerbanco = atol(row->values[0]);
    }
  } while (row != NULL);

  delete cur_mem;

  Serial.print("  Status Botao = ");
  Serial.println(lerbanco);

  delay(500); 
   
}
    
    


    
        
