#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include "FS.h"
#include "SPIFFS.h"
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include <Crypto.h>
#include <SHA512.h>
#include <vector>


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
				uint8_t weekDay = _items[i].startSwitchTime.WeekDay;
				wd[weekDay-1]++;
				json["weekday"][String(weekDay)][wd[weekDay-1]]["mode"] = GetModeName(_items[i].task);
				json["weekday"][String(weekDay)][wd[weekDay-1]]["time"] = String(_items[i].startSwitchTime.Hour) + ":" + String(_items[i].startSwitchTime.Minute);
				json["weekday"][String(weekDay)][wd[weekDay-1]]["preparemin"] = _items[i].totalSwitchMinutes;
				
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
			_settings = {ssid, password, isHotspot};
			if(_settings.isHotspot){
				if(_settings.password == ""){
					WiFi.softAP(_settings.ssid);
				} else {
					WiFi.softAP(_settings.ssid, _settings.password);
				}	
			} else {
				WiFi.begin(_settings.ssid, _settings.password);
			}
			while (WiFi.status() != WL_CONNECTED){
				Serial.print(".");
        delay(500);
    	}
    	Serial.println(".");
    	Serial.println("WiFi Connected");
		}
		Ethernet(const String &path) : _path(path){
			_settings = {"Pzz Oven", "testtest", true};
			// _ReadSettings();
			if(_settings.isHotspot == true){
				if(_settings.password == ""){
					WiFi.softAP(_settings.ssid);
				} else {
					WiFi.softAP(_settings.ssid, _settings.password);
				}	
			} else {
				WiFi.begin(_settings.ssid, _settings.password);
			}
		}

		DynamicJsonDocument NetworksToJson(){
			_nwNum = WiFi.scanNetworks();
			DynamicJsonDocument wifiJson(JsonSize());
			if(_nwNum == 0){
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
			return abs(_nwNum) * 50;
		}

		// void Connect(EthernetSettings settings){
		// 	unsigned long time = millis();

		// 	if(settings.password = ""){
		// 		WiFi.begin(settings.ssid);
		// 	} else {
		// 		WiFi.begin(settings.ssid, settings.password);
		// 	}

		// 	delay(5000);
		// }

	private:
		EthernetSettings _settings;
		String _path;
		int _nwNum;


		// void _ReadSettings(){
		// 	STORAGE->Read(_path, (char*)&_settings, sizeof(_settings));
		// }

		// void _WriteSettings(){
		// 	STORAGE->Write(_path, (uint8_t*)&_settings, sizeof(_settings));
		// }

};

Ethernet * ETHERNET;
class WebPanel {
	public:
		WebPanel(int port = 80) : _server(port) {
			Serial.printf("WebServer was started on :%d port.\r\n", port);
			_Initialize();
		}

	private:
		AsyncWebServer _server;

		void _Initialize(){
			_server.on("/", HTTP_GET, [this](AsyncWebServerRequest* request){ _HandlerRooT(request); });
			_server.on("/table", HTTP_GET, [this](AsyncWebServerRequest* request){ _HandlerTable(request); });
			_server.on("/dispence", HTTP_GET, [this](AsyncWebServerRequest* request) { _DispenceHandler(request); });
			_server.on("/action", HTTP_GET, [this](AsyncWebServerRequest* request) { _DispenceHandler(request); });
			_server.on("/auth", HTTP_GET, [this](AsyncWebServerRequest* request) { _AuthHandler(request); });

			_server.begin();
		}
		void _AuthHandler(AsyncWebServerRequest* request){
			if(request->hasParam("password") && request->hasParam("login")){
				AsyncWebHeader* password = request->getHeader("password");
				AsyncWebHeader* login = request->getHeader("login");
				SHA512 sha512;
			}

		}

		void _HandlerRooT(AsyncWebServerRequest* request){
			Serial.println("пытается в The Root");
			request->send(200, "text/plain", "The Root");
		}

		void _HandlerTable(AsyncWebServerRequest* request){
			Serial.println("пытается в The Table");
			request->send(200, "text/plain", "The Table");
		}

		size_t _JsonSizeCalc(AsyncWebServerRequest* request){
			size_t bytes = 0;
			if(request->hasParam("table")){
				bytes += tasks.JsonSize() + 20;
			}
			if(request->hasParam("cook_temp")){
        bytes += 20;
      }
      if(request->hasParam("conveyer")){
        bytes += 15;
      }
      if(request->hasParam("fan")){
        bytes += 15;
      }
      if(request->hasParam("led")){
      	bytes += 25;
      }
      if(request->hasParam("wifi")){
        bytes += ETHERNET->JsonSize() + 20;
      }
      return bytes;
		}

		void _DispenceHandler(AsyncWebServerRequest* request){
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

		void _ActionHandler(AsyncWebServerRequest* request){
			
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
}
	
  
void loop(){
}
