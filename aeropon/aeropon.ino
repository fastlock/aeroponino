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
TO DO:
  1.aggiungere un interfaccia seriale
  2.rendere possibile modificare i parametri della macchina a stati
 */

//--------------HARDWARE PARAMETERS-----------------------
#define HP_WP_A_PIN 12 //Pompa alta pressione A
#define HP_WP_B_PIN 11 //Pompa alta pressione B
#define DEVIATOR_PIN 10 //Relè elettrovalvola deviatore
#define AGITATOR_PIN 9 //Pompa agitatrice
#define TOP_TANK_PIN 8 //Sensore livello alto tanica
#define BOTTOM_TANK_PIN 7 //Sensore livello basso tanica
#define LIGHT_SENSOR A0  //Sensore luminosità

#define LOW 1
#define HIGH 0
//--------------STATE MACHINE PARAMETERS-------------------
int DAYLIGHT = 800;          //soglia luce sistema notte/giorno
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

//---------------------------------------------------------
void Set_timer1(uint16_t seconds)
{
    timer_armed=true;
    t_start=millis();
    t_stop=millis()+(seconds*1000);
}
void watchtime(){
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
  if(digitalRead(TOP_TANK_PIN)==LOW){
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
  //Serial.println(light);
  if(light>=DAYLIGHT) {
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
  if(idle_cycle_count >= IDLE_CYCLE_LIMIT){
    idle_cycle_count=0;
    recirculation(!recirc_state);//avvio il ricircolo ogni due cicli
  }
  daywatch();//controllo se è giorno
  
  if(is_night){
    sleep();
    
  }
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
  General_state=2;
  alt(0);//fermo tutto
  while(1){
  if(is_night){
    
    idle();
    
  }
  else{
    return;
  }
  }
  
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
  idle();
  //attachInterrupt(0, fill,FALLING);
  //DEBUG
  Serial.begin(9600);
  Serial.println("AEROPONINO--WATER CONTROLLER SERIAL CONSOLE V.0.1");
  
}
void is_empty(){
  if(digitalRead(BOTTOM_TANK_PIN)==HIGH){
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
  digitalWrite(HP_WP_A_PIN,stat_a);
  digitalWrite(HP_WP_B_PIN,stat_b);
}

