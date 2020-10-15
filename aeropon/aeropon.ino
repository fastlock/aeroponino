/*
     |
    \|/|/      
  \|\\|//|/    |
   \|\|/|/    \|/
    \\|//    \ | /
     \|/      \|/
     \|/       |
      |        |
|=====================|
|     /|\      /|\    | 
|     AEREOPONINO     |
|_\|/__\|/__\|/__\|/_ |
|=====================|
****************************
  Water controller
  FW_Version:0.0.1
  Author:Fastlock
  Board:Arduino/Genuino Uno
****************************

REV 3/10/2020
 */
//#include <EEPROM.h>//futuro
//--------------HARDWARE PARAMETERS-----------------------
#define HP_WP_A_PIN 12 //Pompa alta pressione A
#define HP_WP_B_PIN 11 //Pompa alta pressione B
#define DEVIATOR_PIN 10 //Relè elettrovalvola deviatore
#define AGITATOR_PIN 9 //Pompa agitatrice
#define TOP_TANK_PIN 8 //Sensore livello alto tanica
#define BOTTOM_TANK_PIN 7 //Sensore livello basso tanica
#define LIGHT_SENSOR A0  //Sensore luminosità

#define LOW 0
#define HIGH 1
#define low_sens 0
#define high_sens 1
//-------------- DEFAULT STATE MACHINE PARAMETERS-------------------
int DAYLIGHT = 150;          //soglia luce sistema notte/giorno
int IDLE_CYCLE_LIMIT = 2;    //dopo quanti cicli avviare agitatore
int DELAY_IRRIGATION = 10;  //Tempo di attesa tra le irrigazioni (secondi)
int IRRIGATION_DURATION= 5; //Tempo di irrigazione(secondi)
volatile int General_state=0;
bool is_night=false;
volatile bool pump_sel;//0->A,1->
volatile bool stato_a;
volatile bool stato_b;

int idle_cycle_count=0;
bool tank_full=true;
int pumps[2]={HP_WP_A_PIN,HP_WP_B_PIN};
bool recirc_state=false;
long int current=0;
long int t_start,t_stop;
bool timer_armed=false;
int light_val_address=2;
int delay_val_address=6;
int duration_val_address=10;
//---------------------------------------------------------
void Set_timer1(uint16_t seconds)
{
    timer_armed=true;
    t_start=millis();
    t_stop=millis()+(seconds*1000);
}
void watchtime(){
  //controllo se è giorno ogni ciclo di loop
  daywatch();
  if(is_night){
    sleep();
    return;
  }
  current=millis();
  if((current>=t_stop)&&timer_armed){
      timeout();//si è verificato un match.
  }
}
void timeout ()
{
    switch(General_state){
      case 0:
        //return from idle,start watering
        timer_armed=false;
        watering();
        
        break;
      case 1:
        //return from watering,start idle
        timer_armed=false;
        idle();
        
        break;
    }
}
void alt(int sel){
  //serve a fermare selettivamente una funzione rispetto ad un altra
  //1-> solo pompe irrigazione
  //0->tutti gli attuatori
  if(sel==1){
     digitalWrite(HP_WP_B_PIN,LOW);
     digitalWrite(HP_WP_A_PIN,LOW);
     stato_a=false;
     stato_b=false;
  }
  else{
    digitalWrite(HP_WP_B_PIN,LOW);
    digitalWrite(HP_WP_A_PIN,LOW);
    stato_a=false;
    stato_b=false;
    digitalWrite(AGITATOR_PIN,LOW);
    digitalWrite(DEVIATOR_PIN,LOW);
  }
}
bool is_full(){
  //controlla se il serbatoio è pieno
  if(digitalRead(TOP_TANK_PIN)==high_sens){ 
     tank_full=true;
     return true;
  }
  else {
    tank_full=false;
    return false;
  }
  
}
bool daywatch(){
  int light=analogRead(LIGHT_SENSOR);
  //Serial.print("VALORE LUCE");
  //Serial.println("Serbatoio vuoto");
  //Serial.println(light);
  if(light<=DAYLIGHT) {
    is_night = true;
  }
  else{
    is_night=false;
  }
  
}
void idle(){ 
  General_state=0;
  alt(1);//fermo le pompe
  idle_cycle_count++;
  if((idle_cycle_count >= IDLE_CYCLE_LIMIT)&&(!is_night)){
    idle_cycle_count=0;
    recirculation(!recirc_state);//avvio il ricircolo ogni due cicli
  }
  //daywatch();//controllo se è giorno
  
  /*if(is_night){
    Serial.println("È NOTTE");
    sleep();
    
  }
  */
  Set_timer1(DELAY_IRRIGATION);//imposto il timer di idle
}
void watering(){
  General_state=1;
  if(pump_sel)pump_sel=false;
  else pump_sel=true;
  digitalWrite(pumps[pump_sel],HIGH);//prima pompa A e dopo pompa B
  Set_timer1(IRRIGATION_DURATION);// irrigo per 20 s
  
}
void sleep(){
  alt(0);//fermo tutto
  Serial.println("Notte");
  //delay(1000);
  
}

void recirculation(bool state){
  if(state){
    digitalWrite(DEVIATOR_PIN,LOW);
    digitalWrite(AGITATOR_PIN,HIGH);
  }
  else{
    digitalWrite(DEVIATOR_PIN,LOW);
    digitalWrite(AGITATOR_PIN,LOW);
  }
  recirc_state=state;
  
}

void setup(){
  pinMode(HP_WP_A_PIN,OUTPUT);
  pinMode(HP_WP_B_PIN,OUTPUT);
  pinMode(AGITATOR_PIN,OUTPUT);
  pinMode(DEVIATOR_PIN,OUTPUT);
  pinMode(TOP_TANK_PIN,INPUT);
  pinMode(BOTTOM_TANK_PIN,INPUT);
  //if(EEPROM.read(light_val_address) !=0) DAYLIGHT=EEPROM.read(light_val_address);
  //if(EEPROM.read(delay_val_address) !=0) DELAY_IRRIGATION=EEPROM.read(delay_val_address);
  //if(EEPROM.read(duration_val_address) !=0) IRRIGATION_DURATION=EEPROM.read(duration_val_address);
  idle();

  Serial.begin(9600);
  Serial.println("AEROPONINO--WATER CONTROLLER SERIAL CONSOLE V.0.1");
  
}
void is_empty(){
  //riempio il serbatoio se è vuoto solo di giorno
  if((digitalRead(BOTTOM_TANK_PIN)==low_sens) && !is_night){//LOGICA INVERSA
    Serial.println("Serbatoio vuoto");
    fill();
  }
  else{
    return;
  }
}
void loop(){
  watchtime();//controllo l'orologio
  is_empty();
  Serial.print("CURRENT STATUS:");
  Serial.println(General_state);
  //serialCOMM();futuro
  
  
}
void fill(){
  //registro lo stato precendente all'interrupt
  int past_state=General_state;
  int stat_a=stato_a;
  int stat_b=stato_b;
  alt(0);//fermo tutto
  General_state=3;
  while(!is_full()){
    digitalWrite(DEVIATOR_PIN,HIGH);
    digitalWrite(AGITATOR_PIN,HIGH);
  }
  alt(0);
  //ristabilisco lo stato dopo l'esecuzione dell'interrupt
  General_state=past_state;
  //digitalWrite(HP_WP_A_PIN,!stat_a);
  //digitalWrite(HP_WP_B_PIN,!stat_b);
}
/*
//futuro
void serialCOMM(){
  int irr,idles,sday;
  sday=DAYLIGHT;
  idles=DELAY_IRRIGATION;
  irr=IRRIGATION_DURATION;
  if(Serial.available()){
    alt(0);
   if(Serial.read()=='c'){
     Serial.println("1.modifica soglia luce\n2.modifica tempo idle\n3.modifica tempo irrigazione\n4.mostra soglie\n5.valori real time \n6.salva ed esci");
    for(;;){
      
      switch(Serial.read()){
        case '1':
        Serial.println("luce");
        delay(1000);
        sday=Serial.readString().toInt();
        Serial.println(sday);
        delay(1000);
        break;
        case '2':
        Serial.println("idle");
        delay(1000);
        idles=Serial.readString().toInt();
        Serial.println(idles);
        delay(1000);
        break;
        case '3':
        Serial.println("irr");
        delay(1000);
        irr=Serial.readString().toInt();
        Serial.println(irr);
        delay(1000);
        break;
        case '4': 
        Serial.println("----soglie salvate----");
        Serial.print("Soglia Giorno:");
        Serial.println(DAYLIGHT);
        Serial.print("Tempo di pausa(s):");
        Serial.println(DELAY_IRRIGATION);
        Serial.print("Tempo di Irrigazione(s):");
        Serial.println(IRRIGATION_DURATION);
        break;
        case '5':
        Serial.println("----ultima lettura----");
        Serial.print("LUCE ATTUALE:");
        Serial.println(analogRead(LIGHT_SENSOR));
        Serial.print("valore eeprom:");
        Serial.println(EEPROM.read(light_val_address));
        break;
        case '6':
        Serial.println("SALVATAGGIO");
        DAYLIGHT=sday;
        DELAY_IRRIGATION=idles;
        IRRIGATION_DURATION=irr;
        EEPROM.put(light_val_address,sday);
        EEPROM.put(delay_val_address,idles);
        EEPROM.put(duration_val_address,irr);
        delay(3000);
        Serial.println("SALVATAGGIO COMPLETATO");
        delay(1000);

        return;
        default:continue;
      }
    
    }
    
   }
  }
}
*/

