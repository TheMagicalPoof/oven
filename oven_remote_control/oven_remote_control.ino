#include "ArduinoJson.h"
#include "AsyncJson.h"
#include <AsyncTCP.h>
#include "FS.h"
#include "SPIFFS.h"
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include <Crypto.h>
#include <SHA512.h>
#include <vector>
#include <esp_task_wdt.h>

#define HASH_SIZE 32
#define RESET_BUTTON_PIN 15
#define RESET_BUTTON_PRESSED_TIME 5000
#define LED_PIN 23

unsigned long CurrentMillis = 0;
unsigned long FirstMillis = 0;
unsigned long SecondMillis = 0;
unsigned long ButtonMillis = 0;

typedef struct {
			uint8_t r;
    	uint8_t g;
    	uint8_t b;
		} RGB;

typedef struct {
		RGB color;
  	String name;
  	int prepareMinutes;
  	bool prepare;
	} WorkMode;

	WorkMode _MODE1 = {{0, 0, 255}, "MODE 1", 120, false};
	WorkMode _MODE2 = {{0, 0, 255}, "MODE 2", 120, false};
	WorkMode _MODE3 = {{0, 0, 255}, "MODE 3", 120, false};
	WorkMode _MODE4 = {{0, 0, 255}, "MODE 4", 120, false};
	WorkMode _IDLE = {{0, 0, 255}, "IDLE", 0, false};
	WorkMode _DISABLED = {{0, 0, 255}, "DISABLED", 0, false};

typedef struct {
			uint8_t WeekDay;
    	uint8_t Hour;
    	uint8_t Minute;
		} Time;


class TimeManager {
	public:
		Time time;
		TimeManager(Time &time) : time(time){
		}

		void Shift(unsigned short int minutes){
      unsigned short int shift_hours = (this->time.Minute + minutes) / 60;
			this->time.Minute = (this->time.Minute + minutes) % 60;
			unsigned short int shift_days = (this->time.Hour + shift_hours) / 24;
			this->time.Hour = (this->time.Hour + shift_hours) % 24;
			this->time.WeekDay = ((this->time.WeekDay-1 + shift_days) % 7) + 1;						
		}

		// bool IsExistUntil(Time &checked, Time until){
		// 	Time before = this;

		// }

		void Shift(){
			Shift(1);
		}
};

class Schedule {
	public:
		typedef struct {
			int task;
    	Time startSwitchTime;
    	unsigned short int totalSwitchMinutes;
		} Item;

    enum Mode {NONE, MODE1, MODE2, MODE3, MODE4, IDLE, TURNOFF};
    String TaskName[7] = {"NONE", "MODE1", "MODE2", "MODE3", "MODE4", "IDLE", "TURNOFF"};
    
		Schedule(const String &path) : _path(path){
			Serial.println(path);

			// Read();
		}

    String GetModeName(int & mode){
      return TaskName[mode];
    }
      
		// unsigned short int GetTaskMinutesLeft(Time time){
		// 	unsigned short int minutes = 1;
		// 	Task task = GetTask(time);
		// 	while(true){
		// 		time.Shift();
		// 		if(minutes >= 120){
		// 			return 0;
		// 		}
		// 		// Serial.printf("Schedule->GetTaskMinutesLeft->return: Task:%d.\r\n", task);
		// 		if(GetTask(time) != task){
		// 			return minutes;
		// 		}
		// 		minutes++;
		// 	}
		// }

		void ItemsClear(){
			_items.clear();
		}

		void AddItem(Time & startSwitchTime, unsigned short int totalSwitchMinutes, int task){
			Item item = {task, startSwitchTime, totalSwitchMinutes};
			_items.push_back(item);
		}

		void Write(){
			// SPIFFS.remove(_path.c_str());
			File file = SPIFFS.open(_path.c_str(), FILE_WRITE);
			for(int i = 0; i < _items.size(); i++){
				file.write((uint8_t*)&_items[i], sizeof(Item));
			}
			file.close();
		}

		void Read(){
			_items.clear();
			File file = SPIFFS.open(_path.c_str(), FILE_READ);
			while(file.available()){
				Item buf;
				file.readBytes((char*)&buf, sizeof(Item));
				_items.push_back(buf);
			}
			file.close();
		}

		void Show(){
			for(int i = 0; i < _items.size(); i++){
				Serial.println(_items[i].task);
				Serial.print(_items[i].startSwitchTime.WeekDay);
				Serial.print(":");
				Serial.print(_items[i].startSwitchTime.Hour);
				Serial.print(":");
				Serial.println(_items[i].startSwitchTime.Minute);
				Serial.println(_items[i].totalSwitchMinutes);
				Serial.println("--------");
			}
		}

		DynamicJsonDocument ToJson(){		 
			DynamicJsonDocument json(JsonSize());
			int wd[7] = {-1,-1,-1,-1,-1,-1,-1};
			for (int i = 0; i < _items.size(); i++){
				for(int wd = 0; wd < 7; wd++){
					
				}
				uint8_t weekDay = _items[i].startSwitchTime.WeekDay;
				wd[weekDay-1]++;
				json["weekday"][weekDay-1][wd[weekDay-1]]["mode"] = GetModeName(_items[i].task);
				json["weekday"][weekDay-1][wd[weekDay-1]]["time"] = String(_items[i].startSwitchTime.Hour) + ":" + String(_items[i].startSwitchTime.Minute);
				json["weekday"][weekDay-1][wd[weekDay-1]]["preparemin"] = _items[i].totalSwitchMinutes;
				
			}
			return json;
		}

		size_t JsonSize(){
			return _items.size() * 128;
		}

	private:
		String _path;
		bool _is_edited = false;
		std::vector<Item> _items;
};

Schedule tasks("/schedule.bin");

class Ethernet {
	public:
		typedef struct {
    	const char* ssid;
    	const char* password;
    	bool isHotspot;
		} EthernetSettings;

		Ethernet(const String &path, const char* ssid, const char* password, bool isHotspot = false) : _path(path){
			Serial.println(isHotspot);
			
			SettingsSet(ssid, password, isHotspot);
			FileWrite();
			Connect();
		}

		Ethernet(const String &path) : _path(path){
			SettingsSet("Pzz Oven", "testtest", true);
			FileRead();
			Connect();
		}

		void TryConnect(const char* ssid, const char* password, bool isHotspot = false){
			EthernetSettings try
		}

		void Reset(){
			SettingsSet("Pzz Oven", "testtest", true);
			Connect();
		}

		void SettingsSet(const char* ssid, const char* password, bool isHotspot){
			_settings = {ssid, password, isHotspot};
			FileWrite();
		}

		void FileWrite(){
			File file = SPIFFS.open(_path.c_str(), FILE_WRITE);
			file.write((uint8_t*)&_settings, sizeof(EthernetSettings));
			file.close();
		}

		void FileRead(){
			File file = SPIFFS.open(_path.c_str(), FILE_READ);
			if(!file){
				return;
			}
			file.readBytes((char*)&_settings, sizeof(EthernetSettings));
			file.close();
		}

		void NetworksScan(){
			Serial.println("scan networks");
			WiFi.scanDelete();
			WiFi.scanNetworks(true, true);
		}

		DynamicJsonDocument NetworksToJson(){
			// Serial.println("Wifi networks");
			// Serial.println(_nwNum);
			DynamicJsonDocument wifiJson(JsonSize());
			if(_nwNum < 0){
				return wifiJson;
			}else{
				for (int i = 0; i < _nwNum; i++){
					wifiJson[i]["SSID"] = WiFi.SSID(i);
					wifiJson[i]["RSSI"] = WiFi.RSSI(i);
				}
			}
			return wifiJson;
		}

		size_t JsonSize(){
			_nwNum = WiFi.scanComplete();
			if(_nwNum <= 0 ){
				return 70;
			}
			return _nwNum * 70;
		}

		void Connect(){
			WiFi.disconnect();
			if(_settings.isHotspot){
				if(_settings.password == ""){
					WiFi.softAP(_settings.ssid);
				} else {
					WiFi.softAP(_settings.ssid, _settings.password);
				}	
			} else {
				WiFi.begin(_settings.ssid, _settings.password);
				while (WiFi.status() != WL_CONNECTED){
					Serial.print(".");
	        delay(500);
    		}
			}

			
    	Serial.println(".");
    	Serial.println("WiFi Connected");
		}

	private:
		EthernetSettings _settings;
		String _path;
		int _nwNum = -2;
};

Ethernet * ETHERNET;

String array_to_string(byte array[], unsigned int len){
	char buffer[len*2];
	Serial.println(len);
  for (unsigned int i = 0; i < len; i++){
    byte nib1 = (array[i] >> 4) & 0x0F;
    byte nib2 = (array[i] >> 0) & 0x0F;
    buffer[i*2+0] = nib1  < 0xA ? '0' + nib1  : 'A' + nib1  - 0xA;
    buffer[i*2+1] = nib2  < 0xA ? '0' + nib2  : 'A' + nib2  - 0xA;
  }
  buffer[len*2] = '\0';
  Serial.println(strlen(buffer));
  return String(buffer);
}

String toHash(const String & login, const String & password){
			const char* secret = "KIsgsdD";
			char final[HASH_SIZE];
			SHA512 sha512;
			sha512.update(login.c_str(), login.length());
			sha512.update(secret, strlen(secret));
			sha512.update(password.c_str(), password.length());
			sha512.finalize(final, HASH_SIZE);
			return array_to_string((byte*)final, HASH_SIZE);
		}

class WebPanel {
	public:
		WebPanel(int port = 80) : _server(port) {
			Serial.printf("WebServer was started on :%d port.\r\n", port);
			_CredentialsRead();
			_ServerRoutes();
		}

	private:
		typedef struct {
    	String login;
    	String hash;
    	bool nowOnline;
		} AuthItem;

		AsyncWebServer _server;
		std::vector<AuthItem> _AuthItems;

		void _ServerRoutes(){
			DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
			_server.on("/", HTTP_GET, [this](AsyncWebServerRequest* request){ _RootHandler(request); });
			_server.on("/table", HTTP_GET, [this](AsyncWebServerRequest* request){ _TableHandler(request); });
			_server.on("/output", HTTP_GET, [this](AsyncWebServerRequest* request) { _OutputHandler(request); });
			_server.on("/input", HTTP_GET, [this](AsyncWebServerRequest* request) { _InputHandler(request); });
			_server.on("/auth", HTTP_POST, [this](AsyncWebServerRequest* request) { _AuthHandler(request); });
			_server.on("/newwifi", HTTP_GET, [this](AsyncWebServerRequest* request) { _NewWifiHandler(request); });

			_server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){ request->send(200, "text/plain", String(ESP.getFreeHeap())); });

			_server.addHandler(new AsyncCallbackJsonWebHandler("/data", [this](AsyncWebServerRequest *request, JsonVariant &json) { _WifiHandler(request, json); }));
			_server.begin();
		}
		void _NewWifiHandler(AsyncWebServerRequest *request){
			if(request->hasParam("ssid") && request->hasParam("password")){
				AsyncWebParameter* ssid = request->getParam("ssid");
				AsyncWebParameter* password = request->getParam("password");		
			} else {
				request->send(200, "text/plain", "Bad Request!");
				return;
			}
			if(request->hasParam("hotspot")){
				ETHERNET->SettingsSet(ssid->value().c_str(), password->value().c_str(), true)
			} else {
				ETHERNET->SettingsSet(ssid->value().c_str(), password->value().c_str(), false)
			}
			ETHERNET->
			
		}

		void _WifiHandler(AsyncWebServerRequest *request, JsonVariant json) {
			serializeJson(json, Serial);
	    request->send(200, "text/plain", "Fin!");
		}
		
		void _AuthHandler(AsyncWebServerRequest* request){
			// http://46.216.20.67:81/auth?login=asdas&password=asdasd
			DynamicJsonDocument authJson(200);
			authJson["authorized"] = false;
			if(request->hasParam("password", true) && request->hasParam("login", true)){
				AsyncWebParameter* password = request->getParam("password", true);
				AsyncWebParameter* login = request->getParam("login", true);
				String hash = toHash(login->value().c_str(), password->value().c_str());
				Serial.println(hash);
				
				for(int i = 0; i < _AuthItems.size(); i++){
					if(_AuthItems[i].hash == hash){
						authJson["authorized"] = true;
						authJson["token"] = hash;
					}
				}
			}
			String finishJson;
			serializeJson(authJson, finishJson);
			request->send(200, "text/plain", finishJson);
		}

		void _RootHandler(AsyncWebServerRequest* request){
			Serial.println("пытается в The Root");
			request->send(200, "text/plain", "The Root");
		}

		void _TableHandler(AsyncWebServerRequest* request){
			Serial.println("пытается в The Table");
			request->send(200, "text/plain", "The Table");
			// if(request->hasParam("ssid"))
		}

		size_t _JsonSizeCalc(AsyncWebServerRequest* request){
			size_t bytes = 0;
			if(request->hasParam("table")){
				bytes += tasks.JsonSize() + 40;
			}
			if(request->hasParam("cook_temp")){
        bytes += 40;
      }
      if(request->hasParam("conveyer")){
        bytes += 40;
      }
      if(request->hasParam("fan")){
        bytes += 40;
      }
      if(request->hasParam("led")){
      	bytes += 80;
      }
      if(request->hasParam("wifi")){
        bytes += ETHERNET->JsonSize() + 70;
      }
      return bytes;
		}

		void _OutputHandler(AsyncWebServerRequest* request){
			DynamicJsonDocument dispenceJson(_JsonSizeCalc(request));

			if(request->hasParam("table")) {
				dispenceJson["table"] = tasks.ToJson();
			}

      if(request->hasParam("cook_temp")){
        dispenceJson["cook_temp"] = random(100, 150);
      }

      if(request->hasParam("conveyer")){
        dispenceJson["conveyer"] = random(2, 10);
      }

      if(request->hasParam("fan")){
        dispenceJson["fan"] = random(2000, 5000);
      }

      if(request->hasParam("led")){
      	dispenceJson["led"][0] = random(0, 255);
      	dispenceJson["led"][1] = random(0, 255);
      	dispenceJson["led"][2] = random(0, 255);
      }

      if(request->hasParam("wifi")){
        dispenceJson["wifi"] = ETHERNET->NetworksToJson();
      }

      String finishJson;
			serializeJson(dispenceJson, finishJson);
			Serial.println(finishJson);
      request->send(200, "text/plain", finishJson);
		}

		void _InputHandler(AsyncWebServerRequest* request){
			if(request->hasParam("networkscan")){
				ETHERNET->NetworksScan();
			}
			request->send(200, "text/plain", "ggwp");
		}

		void _CredentialsRead(){
			_AuthItems.clear();
			File file = SPIFFS.open("auth.bin", FILE_READ);
			while(file.available()){
				AuthItem buf;
				file.readBytes((char*)&buf, sizeof(AuthItem));
				_AuthItems.push_back(buf);
			}
			file.close();
			if(_AuthItems.size() == 0){
				AuthItem item = {"test", "BF5F6AB37D23D8CADDBBF3999DCCF934B64251A2257B22E3A7C07FD45E38DF15", false};
				_AuthItems.push_back(item);
			}
		}

		void _CredentialsWrite(){
			SPIFFS.remove("auth.bin");
			File file = SPIFFS.open("auth.bin", FILE_WRITE);
			for(int i = 0; i < _AuthItems.size(); i++){
				file.write((uint8_t*)&_AuthItems[i], sizeof(AuthItem));
			}
			file.close();
		}

		bool _CheckAccessToken(AsyncWebServerRequest* request){
			if(request->hasHeader("Cookie")){
				AsyncWebHeader* rawCookie = request->getHeader("Cookie");
				for(int i = 0; i < _AuthItems.size(); i++){
					if(_AuthItems[i].hash == rawCookie->value()){
						return true;
					}
				}
			}
			return false;
		}
};

WebPanel * WEBPANEL;



void setup(){
	if(!SPIFFS.begin()){
		Serial.println("SPIFFS Mount Failed");
	}
	Serial.begin(115200);

	// ETHERNET = new Ethernet("ethernet.bin", "Atlantida", "Kukuruza+137", false);
	// ETHERNET = new Ethernet("ethernet.bin", "White Power", "12121212", false);
	ETHERNET = new Ethernet("ethernet.bin", "IwG", "qawsedrf", false);
	ETHERNET->NetworksScan();
	// ETHERNET = new Ethernet("ethernet.bin", "dom2", "4438144381", false);
	// STORAGE = new Storage();
	new WebPanel(80);
	
	
	Time time1={2, 3, 15};
 	tasks.AddItem(time1, 60, 3);
 	Time time5={2, 4, 13};
 	tasks.AddItem(time5, 60, 6);
 	Time time6={2, 6, 42};
 	tasks.AddItem(time6, 60, 4);
 	Time time2={5, 11, 55};
 	tasks.AddItem(time2, 120, 2);
 	Time time3 = {7, 23, 32};
 	tasks.AddItem(time3, 10, 1);
 	tasks.Write();
 	tasks.ItemsClear();
 	tasks.Read();
 	tasks.Show();
	Serial.println(WiFi.softAPIP());
	Serial.println(WiFi.localIP());
	Serial.printf("setup(): Complite!\r\n");
	pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);
	pinMode(LED_PIN, OUTPUT);
	digitalWrite(LED_PIN, HIGH);


}
void ResetButtonCheck(){
	if(digitalRead(RESET_BUTTON_PIN)){
		ButtonMillis = CurrentMillis;
	}

	if(CurrentMillis - ButtonMillis >= RESET_BUTTON_PRESSED_TIME){
		digitalWrite(LED_PIN, !digitalRead(LED_PIN));
		ETHERNET -> Reset();
		ButtonMillis = CurrentMillis;
	}
}

void loopOneSecond(){
	ResetButtonCheck();
		
}

void loopTenMinutes(){

}

void loop(){
	CurrentMillis = millis();

	if(CurrentMillis - FirstMillis >= 1000){
		loopOneSecond();
		FirstMillis = CurrentMillis;
	}

	if(CurrentMillis - SecondMillis >= 600000){
		loopTenMinutes();
		SecondMillis = CurrentMillis;
	}
}
