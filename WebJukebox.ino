#include <SoftwareSerial.h>
#include "WiFly.h"
#include <SdFat.h>
#include <MD_MIDIFile.h>
SdFat  SD;
MD_MIDIFile SMF;

#define SSID      "d3m"
#define KEY       "buxtehude"
// check your access point's security mode, mine was WPA20-PSK
// if yours is different you'll need to change the AUTH constant, see the file WiFly.h for avalable security codes
#define AUTH      WIFLY_AUTH_WPA2_PSK


// change this to match your SD shield or module;
// Arduino Ethernet shield: pin 4
// Adafruit SD shields and modules: pin 10
// Sparkfun SD shield: pin 8
// MKRZero SD: SDCARD_SS_PIN
const int chipSelect = 10;
 
#define USE_MIDI  1  // set to 1 for MIDI output, 0 for debug output
char userOutput[800];

#if USE_MIDI // set up for direct MIDI serial output
#define DEBUGS(s)   {if(strlen(userOutput)+strlen(s)<sizeof(userOutput))strcat(userOutput, s);}
#define DEBUG(s, x) {if(strlen(userOutput)+strlen(s)+10<sizeof(userOutput))sprintf(userOutput+strlen(userOutput), "\n%s %d", s, x);}
#define DEBUGX(s, x)
#define SERIAL_RATE 31250

#else // don't use MIDI to allow printing debug statements

#define DEBUGS(s)     Serial.print(s)
#define DEBUG(s, x)   { Serial.print(F(s)); Serial.print(x); }
#define DEBUGX(s, x)  { Serial.print(F(s)); Serial.print(x, HEX); }
#define SERIAL_RATE 57600

#endif // USE_MIDI

int flag = 0;
 
// Pins' connection
// Arduino       WiFly
//  2    <---->    TX
//  3    <---->    RX
// Mega          WiFly
//  11   <---->    TX
//  12   <---->    RX
 
SoftwareSerial wiflyUart(11, 12); // create a WiFi shield serial object
WiFly wifly(&wiflyUart); // pass the wifi siheld serial object to the WiFly class


void midiSilence(void)
// Turn everything off on every channel.
// Some midi files are badly behaved and leave notes hanging, so between songs turn
// off all the notes and sound
{
  midi_event ev;

  // All sound off
  // When All Sound Off is received all oscillators will turn off, and their volume
  // envelopes are set to zero as soon as possible.
  const char msg[]={
    120,  // All sounds off
    121,  // Reset all controllers
    123   // All notes off
  };
  
  ev.size = 0;
  ev.data[ev.size++] = 0xb0;
  ev.data[ev.size++] = 0;
  ev.data[ev.size++] = 0;

  for(int i=sizeof(msg)/sizeof(msg[0]); i--; ){
    ev.data[1]=msg[i];
    for (ev.channel = 0; ev.channel < 16; ev.channel++)
    midiCallback(&ev);
  }
}


void midiCallback(midi_event *pev)
// Called by the MIDIFile library when a file event needs to be processed
// thru the midi communications interface.
// This callback is set up in the setup() function.
{
  if ((pev->data[0] >= 0x80) && (pev->data[0] <= 0xe0))
  {
    Serial.write(pev->data[0] | pev->channel);
    Serial.write(&pev->data[1], pev->size-1);
  }
  else
    Serial.write(pev->data, pev->size);
}

void sysexCallback(sysex_event *pev)
// Called by the MIDIFile library when a system Exclusive (sysex) file event needs 
// to be processed through the midi communications interface. Most sysex events cannot 
// really be processed, so we just pass it through
// This callback is set up in the setup() function.
{
  Serial.write(pev->data, pev->size);
}


// Send the string s to wifly if emit is true; return the string length
uint16_t getLengthAndSend(const char* s, bool emit){
  if(emit)wiflyUart.print(s);
  return strlen(s);
}

class title {
  public:
    title(const char* aFile, const char* aDescription, const char* aTiming);
    void start();
    void cancel();
    uint16_t getHtmlEntry(bool emit);
    const char* file;
    const char* description;
    const char* timing;
};

title* currentTitle=NULL;
uint32_t currentTitleTimestamp;

const title  liszt("LISZT.MID",       "Franz Liszt : Prélude et Fugue sur B.A.C.H. (10')", "15, 617, 8");
const title  messiaen("MESSIAEN.MID", "Olivier Messiaen : Banquet céleste (6')", "5, 375, 2");
const title  buxtehude("BUXTEHUD.MID","Dietrich Buxtehude : Passacaille (7')", "5, 425, 8");
const title  wagner("WAGNER.MID",     "Richard Wagner : Mort d'Isolde (8'40)", "8, 520, 8");
const title  dupre("DUPRE.MID",       "Marcel Dupré : Prélude et Fugue en sol mineur", "8, 300, 8");
const title  taille("COUPERIN.MID",   "François Couperin : Tierce en taille (4')", "5, 240, 8");
const title  franck("FRANCK.MID",     "César Franck : Troisième Choral (10'20)", "13, 622, 8");
const title  chromorne("CHROMORN.MID", "François Couperin : Chromorne en taille (4')", "7, 233, 8");
const title *playList[] = {&liszt, &franck, &wagner, &buxtehude, &chromorne, &messiaen};

title::title(const char* aFile, const char* aDescription,  const char* aTiming){
  file = aFile;
  description = aDescription;
  timing = aTiming;
}
void title::start(){
  if(this == currentTitle)return;
  int err = SMF.load(this->file);
  if (err != MD_MIDIFile::E_OK){
    DEBUG("SD load error:  ", err); 
  }else{
    currentTitle=this;
  }  
}
void title::cancel(){
  if(currentTitle==NULL)return;
  DEBUG("Now stopping: ", description);
  SMF.close();
}
uint16_t title::getHtmlEntry(bool emit){
  return 
    getLengthAndSend("<li><a href=\"", emit)+
    getLengthAndSend(file, emit)+
    getLengthAndSend("\">", emit)+
    getLengthAndSend(description, emit)+
    getLengthAndSend("</a></li>", emit);
}

const char * pageHeader = R"(
<html><head><title>Pipe organ as a juke box</title>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
body {  background-color: black;  font-family: verdana;  color: white;}
h1,h2 {  text-align: center;}
h3 { color: grey }
a {  color: yellow;  font-size: 20px;}
table {width: 100%;}
</style>
</head>
<link rel="icon" href="data:;base64,iVBORw0KGgo=">
<body><table><tr>
<td><svg id='logo' version="1.1"
     baseProfile="full"
     width="30" height="20"
     xmlns="http://www.w3.org/2000/svg">
  <rect width="100%" height="100%" fill="black" />
</svg>
</td>
<td><h1>Les grandes orgues</h1><h2>de la Basilique St Joseph de Grenoble</h2></tr></td>
</table>
<script>
var xmlns = "http://www.w3.org/2000/svg";

pipe = function(x, y, height, width, mouth){
	var polygon = document.createElementNS(xmlns, "polygon");
	var mwidth=height/60
	polygon.setAttribute("style","fill:black;stroke:green;")
	var points=(x+width/2).toString()+','+(y-mouth).toString()
	points+=' '+(x-width/2).toString()+','+(y-mouth).toString()
	points+=' '+(x-width/2).toString()+','+(y-mouth-height).toString()
	points+=' '+(x+width/2).toString()+','+(y-mouth-height).toString()
	points+=' '+(x+width/2).toString()+','+(y-mouth).toString()
	points+=' '+x.toString()+','+y.toString()
	points+=' '+(x-width/2).toString()+','+(y-mouth).toString()
	points+=' '+(x-width/2).toString()+','+(y-mouth-mwidth).toString()
	points+=' '+(x+width/2).toString()+','+(y-mouth-mwidth).toString()
	polygon.setAttribute("points",points); 
	return polygon
}
plateface = function(nbPipes, x, y, firstH, lastH, width, firstM, lastM, symetric){
	var deltaH=(lastH-firstH)/(nbPipes-1)
	var deltaM=(lastM-firstM)/(nbPipes-1)
	if(symetric){deltaH*=2; deltaM*=2;}
	var h=firstH
	var m=firstM
	var padding=0
	for(let p=0; p<nbPipes; p++){
		if(symetric && p == ~~(nbPipes/2)){
			deltaH*=-1
			deltaM*=-1
		}
		document.getElementById('logo').appendChild(pipe(x+p*(width+padding), y, h, width, m))
		h+=deltaH
		m+=deltaM
	}
	return x+width/2+(nbPipes-1)*(width+padding)
}
logo = function(x0, y0, scale){
	var s=scale
	var nA=3, nB=5, nC=10, nD=9
	var dy=7*s, p=4.5*s, wA=5*s, wB=3*s, wC=3*s, wD=2*s
	var x1=plateface(nA, x0, y0-dy, s*77, s*88, wA, s*17, s*17, true)+p
	var x2=plateface(nB, x1, y0-dy, s*68, s*42, wB, s*13, s*17, false)+p
	var x3=plateface(nC, x2, y0, s*42, s*30, wC, s*11, s*11, false)+p
	var x4=plateface(nD, x3, y0, s*32, s*41, wD, s*11, s*6, true)+p
	var x5=plateface(nC, x4, y0, s*30, s*42, wC, s*11, s*11, false)+p
	var x6=plateface(nB, x5, y0-dy, s*42, s*68, wB, s*17, s*13, false)+p
	var x7=plateface(nA, x6, y0-dy, s*77, s*88, wA, s*17, s*17, true)+p
}

logo(250, 480, 0.7)
</script>
)";
const char * pageFooter = "</body></html>";
const char *refreshOrCancel = R"(
</p><table><tr>
<td align=left><a href='/cancel'>Arrêter</a></td>
<td align=right><a id='refresh' href='/'>Mettre à jour</a></td>
</tr></table>
)";

const char *countDownBegin = R"(
<p id="countDown"></p>
<script>
var distances=[)";
const char *countDownEnd=R"(,0]
var phases=["Chargement : ", "Exécution : ", "Déchargement : "]
// Update the count down every 1 second
var rate=1000;
var phase=0;
var elapsed = distances[phases.length]
phases.forEach(function(p, i){
  if(elapsed <= distances[i]){
    distances[i]-=elapsed
    elapsed= distances[phases.length]=0
    }else{
  phase=i
    elapsed-=distances[i]
    distances[i]=0
    }
})
var distance = distances[phase]
var refreshLink = document.getElementById("refresh")
function doRefresh(){
  refreshLink.click();
}
function countDown() {
  var minutes = Math.floor((distance % (60 * 60)) / 60);
  var seconds = Math.floor(distance % 60);
  document.getElementById("countDown").innerHTML = phases[phase] + (minutes?minutes+"mn ":"")+seconds+"s"
  if(distance == 0 && phase < phases.length-1){
   phase++
   distance = distances[phase]
  }
  if(distance > 0)setTimeout(countDown, rate);
  distance--
}

if(rate)setTimeout(countDown, rate);
setTimeout(doRefresh, 1000*distances.reduce(function(a, b) { return a + b; }, 4))</script>

)";

uint16_t getPageBody(char* request, bool emit){
  uint16_t pageLength=0;
  // number of elements in array
  int const n = sizeof( playList ) / sizeof( playList[ 0 ] );
  int i;
  // Check for a cancel request
  if(currentTitle != NULL && strstr(request, "cancel") != NULL){
    currentTitle->cancel();
    i=-1;
  }else{ 
    for (i = n; i--;){ // search for a known file in the request
      title *e = (title*)playList[i];
      if(strstr(request, e->file) != NULL)break;
    }
    // if a play is in progress, display its page
    if(currentTitle != NULL){
       for (i = n; currentTitle != playList[--i];);
    }
  }  
  
  if(i<0){
    // not found: push the full list of titles
    pageLength+=getLengthAndSend("<h3>Œuvres disponibles :</h3><ul>", emit);
    for (int i = 0; i != n; ++i){
      title *e = (title*)playList[i];
      pageLength+=e->getHtmlEntry(emit);
    }
    pageLength+=getLengthAndSend(refreshOrCancel, emit);    
  }
  
  if(i >= 0){
    // Found:
    // If this title is not playing yet, start it now
    if(currentTitle != playList[i]){
      currentTitleTimestamp=millis();
      title* c=(title*)playList[i];
      if(!emit)c->start();
    }
    // push the page for this title
    pageLength+=getLengthAndSend("<h3>Œuvre en cours d'audition :</h3><p>", emit);
    pageLength+=getLengthAndSend(playList[i]->description, emit);
    pageLength+=getLengthAndSend(refreshOrCancel, emit);
    pageLength+=getLengthAndSend(countDownBegin, emit);
    pageLength+=getLengthAndSend(playList[i]->timing, emit);
    {
      char remain[10]; 
      sprintf(remain, ",%d", (millis()-currentTitleTimestamp)/1000);
      pageLength+=getLengthAndSend(remain, emit);
    }
    pageLength+=getLengthAndSend(countDownEnd, emit);
    
  }
  return pageLength;
}

void setup()
{
    memset(userOutput, 0, sizeof(userOutput));
    DEBUGS("Starting...");
    
    // No current title
    currentTitle=NULL;
    
    wiflyUart.begin(9600); // start wifi shield uart port
    Serial.begin(SERIAL_RATE);
    DEBUGS("\n--------- WIFLY Webserver --------");
 
    // wait for initilization of wifly
    delay(1000);
 
    wifly.reset(); // reset the shield
    delay(1000);
    //set WiFly params
 
    wifly.sendCommand("set ip local 80\r"); // set the local comm port to 80
    delay(100);
 
    wifly.sendCommand("set comm remote 0\r"); // do not send a default string when a connection opens
    delay(100);
 
    wifly.sendCommand("set comm open *OPEN*\r"); // set the string that the wifi shield will output when a connection is opened
    delay(100);
 
    DEBUGS("\nJoin " SSID );
    if (wifly.join(SSID, KEY, AUTH)) {
        DEBUGS("\nOK");
    } else {
        DEBUGS("Join " SSID " Failed");
    }
     delay(5000);
    DEBUGS("\nget ip\n");
    wifly.sendCommand("get ip\r");
    char response[20];
    memset(response, 0, sizeof(response));
    while (wifly.receive((uint8_t *)&response, sizeof(response)-1, 300) > 0) { // print the response from the get ip command
        DEBUGS(response); memset(response, 0, sizeof(response));
    }
 
    DEBUGS("Web server ready: ");

    if(!SD.begin(chipSelect, SPI_FULL_SPEED) )DEBUGS("Error SD")
    else DEBUGS("\nSD init successful");

    // Initialize MIDIFile
    SMF.begin(&SD);
    SMF.setMidiHandler(midiCallback);
    SMF.setSysexHandler(sysexCallback);
    DEBUGS("\nMIDIFile init successful");
}

void loop()
{
  if(wifly.available())
  { // the wifi shield has data available
    if(wiflyUart.find((char *)"*OPEN*")) // see if the data available is from an open connection by looking for the *OPEN* string
    {
      DEBUGS("\nNew Browser Request: ");
      // delay(1000); // delay enough time for the browser to complete sending its HTTP request string
      char request[20];
      memset(request, 0, sizeof(request));  
      wifly.receive((uint8_t *)&request, sizeof(request), 300);
      DEBUGS(request);
        
      // Build the returned page
      uint16_t htmlLength=strlen(pageHeader)+getPageBody(request, false)+strlen(pageFooter)+strlen(userOutput)+strlen("<pre></pre>")-strlen("<html></html>")-1;
      
      // send HTTP header
      wiflyUart.println("HTTP/1.1 200 OK");
      wiflyUart.println("Content-Type: text/html; charset=UTF-8");
      wiflyUart.print("Content-Length: "); 
      wiflyUart.println(htmlLength); // length of HTML code between <html> and </html>
      wiflyUart.println("Connection: close");
      wiflyUart.println();
      
      // send webpage's HTML code
      wiflyUart.print(pageHeader);
      getPageBody(request, true);
      
      // send user message
      wiflyUart.print("<pre>");
      wiflyUart.print(userOutput);memset(userOutput, 0, sizeof(userOutput));
      wiflyUart.print("</pre>");

      wiflyUart.print(pageFooter);
    }
  }

  if(currentTitle != NULL){
    if (SMF.isEOF()){
      SMF.close();
      midiSilence();
      currentTitle=NULL;
    }else{
      SMF.getNextEvent();
    }
  }
}
