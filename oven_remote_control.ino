#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include "FS.h"
#include "SPIFFS.h"
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include <Crypto.h>
#include <SHA512.h>
#include <vector>



class Time {
	public:
		uint8_t Days;
		uint8_t Hours;
		uint8_t Minutes;
		

		Time(uint8_t days, uint8_t hours, uint8_t minutes){
			this->Minutes = minutes;
			this->Hours = hours;
			this->Days = days;
		}

		void Shift(unsigned short int minutes){
      unsigned short int shift_hours = (this->Minutes + minutes) / 60;
			this->Minutes = (this->Minutes + minutes) % 60;
			unsigned short int shift_days = (this->Hours + shift_hours) / 24;
			this->Hours = (this->Hours + shift_hours) % 24;
			this->Days = ((this->Days-1 + shift_days) % 7) + 1;						
		}

		bool IsExistBefore(Time &checked, Time until){
			Time before = this;
			while()

		}

		void Shift(){
			Shift(1);
		}
};

class Storage {
	public:
		Storage(){
			if(!SPIFFS.begin()){
				Serial.println("SPIFFS Mount Failed");
				return;
			}
		}


		bool Write(const String &path, const uint8_t * data, size_t size){
			File file = SPIFFS.open(path.c_str(), FILE_WRITE);
			if(!file){
				Serial.printf("Failed to open %s file.\r\n", path.c_str());
				return false;
			};
			Serial.printf("Bytes writen: %d\r\n", file.write(data, size));
			file.close();
			return true;
		}

		size_t Read(const String &path, char * data, size_t size){
			File file = SPIFFS.open(path.c_str(), FILE_READ);
			if(!file){
				Serial.printf("Open %s file has been failed.\r\n", path.c_str());
				return 0;
			}
			Serial.printf("Reading %s file....\r\n", path.c_str());
			size_t countBytes = file.readBytes(data, size);
			Serial.printf("Bytes readed: %d\r\n", countBytes);
			file.close();
			return countBytes;
		}

		bool Delete(const String &path){
			if(SPIFFS.remove(path)){
				Serial.printf("File %s has been deleted.\r\n", path.c_str());
				return true;
			} else {
				Serial.printf("Deleting %s file has been failed.\r\n", path.c_str());
				return false;
			}
		}

		bool Rename(const String &oldPath, const String &newPath){
			if(SPIFFS.rename(oldPath, newPath)){
				Serial.printf("File %s has been renamed to %s.\r\n", oldPath.c_str(), newPath.c_str());
				return true;
			} else {
				Serial.printf("File %s can't be renamed.\r\n", oldPath.c_str(), newPath.c_str());
				return false;
			}

		}

};

Storage * STORAGE;


class Schedule {
	public:
		typedef struct {
			int task;
    	Time startSwitchTime;
    	unsigned short int totalSwitchMinutes;
		} ScheduleItem;

    enum Mode {NONE, MODE1, MODE2, MODE3, MODE4, IDLE, TURNOFF};
    String TaskName[7] = {"NONE", "MODE1", "MODE2", "MODE3", "MODE4", "IDLE", "TURNOFF"};
    
		Schedule(const String &path) : _path(path){
			Serial.println(path);
			Read();
		}

		int GetMode(Time time){
			for(int i : _items){
				if(_items[i]){
					
			}
		}

    String GetModeName(Time time){
      return TaskName[GetTask(time)];
    }
      
		unsigned short int GetTaskMinutesLeft(Time time){
			unsigned short int minutes = 1;
			Task task = GetTask(time);
			while(true){
				time.Shift();
				if(minutes >= 120){
					return 0;
				}
				// Serial.printf("Schedule->GetTaskMinutesLeft->return: Task:%d.\r\n", task);
				if(GetTask(time) != task){
					return minutes;
				}
				minutes++;
			}
		}

		void AddTask(Time time, byte action, unsigned short int duration_min = 1){
			for(int i = 0; i < duration_min; i++){
				_tasks[time.Days-1][time.Hours][time.Minutes] = action;
				time.Shift();
			}
			_is_edited = true;
		}

		void Write(){
			Serial.printf("Schedule writing to %s file.\r\n", _path.c_str());
			STORAGE->Write(_path, (uint8_t*)&_tasks, 7*24*60);
		}

		void Read(){
			Serial.printf("Schedule reading from %s file.\r\n", _path.c_str());
			STORAGE->Read(_path, (char*)&_tasks, 7*24*60);
		}

		void ClearTasks(){
			Serial.printf("Clearing tasks list.\r\n");
			for(int i = 0; i<7*24*60; i++){
				((char *)_tasks)[i] = NONE;
			}
		}

		DynamicJsonDocument TasksToJson(){		
			Time time(1, 0, 0);
			int previous = -1;
			int n = 0;
			bool isFirst = true;
			DynamicJsonDocument tasksJson(1024);
			for (int i = 0; i < sizeof(_tasks); i++){
				int present = GetTask(time);
				if(present != previous){
					Serial.println(GetStringTask(time));
					Serial.printf("%d:%d:%d\r\n", time.Days, time.Hours, time.Minutes);
					if(isFirst){
						tasksJson[n]["mode"] = GetStringTask(time);
						tasksJson[n]["start"]["weekday"] = time.Days;
						tasksJson[n]["start"]["time"] = String(time.Hours) + ":" + String(time.Minutes);
					}else{
						tasksJson[n]["end"]["weekday"] = time.Days;
						tasksJson[n]["end"]["time"] = String(time.Hours) + ":" + String(time.Minutes);
						n++;
					}
					isFirst = !isFirst;
					// Serial.println(ESP.getFreeHeap());
				}
				previous = present;
				time.Shift();
			}
			return tasksJson;
		}

	private:
		String _path;
		bool _is_edited = false;
		std::vector<ScheduleItem> _items;
};

Schedule * tasks;

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
			_ReadSettings();
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
			int nwNum = WiFi.scanNetworks();
			DynamicJsonDocument wifiJson(512);
			if(nwNum == -1){
				return wifiJson;
			}else{
				for (int i = 0; i < nwNum; i++){
					wifiJson[i]["SSID"] = WiFi.SSID(i);
					wifiJson[i]["RSSI"] = WiFi.RSSI(i);	
				}
			}
			return wifiJson;
		}

		void Connect(EthernetSettings settings){
			unsigned long time = millis();

			if(settings.password = ""){
				WiFi.begin(settings.ssid);
			} else {
				WiFi.begin(settings.ssid, settings.password);
			}

			delay(5000);
		}

	private:
		EthernetSettings _settings;
		String _path;

		void _ReadSettings(){
			STORAGE->Read(_path, (char*)&_settings, sizeof(_settings));
		}

		void _WriteSettings(){
			STORAGE->Write(_path, (uint8_t*)&_settings, sizeof(_settings));
		}

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
			// if(request->hasParam("password") && request->hasParam("login")){
			// 	AsyncWebHeader* password = request->getHeader("password");
			// 	AsyncWebHeader* login = request->getHeader("login");
			// 	SHA512 sha512;
			// }

		}

		void _HandlerRooT(AsyncWebServerRequest* request){
			Serial.println("пытается в The Root");
			request->send(200, "text/plain", "The Root");
		}

		void _HandlerTable(AsyncWebServerRequest* request){
			Serial.println("пытается в The Table");
			request->send(200, "text/plain", "The Table");
		}

		void _DispenceHandler(AsyncWebServerRequest* request){
			DynamicJsonDocument dispenceJson(1024);

			if(request->hasParam("table")) {
				dispenceJson["table"] = tasks->TasksToJson();
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
	Serial.begin(115200);
	// ETHERNET = new Ethernet("ethernet.bin", "Atlantida", "Kukuruza+137", false);
	ETHERNET = new Ethernet("ethernet.bin", "White Power", "12121212", false);
	STORAGE = new Storage();
	tasks = new Schedule("/schedule.bin");
	new WebPanel(80);
	tasks->ClearTasks();
	Serial.printf("Task readed: %d\r\n", tasks->GetTask(Time(6, 10, 34)));
	tasks->AddTask(Time(6, 10, 34), 3, 45);
 	tasks->AddTask(Time(1, 0, 0), 4, 20);
 	tasks->AddTask(Time(2, 12, 00), 1, 60);
	tasks->AddTask(Time(4, 11, 12), 2, 120);
  Serial.printf("Task first readed: %d\r\n", tasks->GetTask(Time(6, 10, 34)));
  Serial.printf("Minutes Left: %d\r\n", tasks->GetTaskMinutesLeft(Time(6, 10, 34)));
	tasks->Write();
	tasks->ClearTasks();
	tasks->Read();
	Serial.printf("Task second readed: %d\r\n", tasks->GetTask(Time(6, 10, 34)));
	Serial.printf("Minutes Left: %d\r\n", tasks->GetTaskMinutesLeft(Time(6, 10, 34)));


	// Time time(0, 23, 51);
	// for(int i = 1; i<=63; i++){
	// 	time.Shift();
	// 	Serial.printf("Time:%d:%d:%d\r\n", time.Days, time.Hours, time.Minutes);
	// }
	Serial.println(WiFi.softAPIP());
	Serial.println(WiFi.localIP());
	Serial.printf("setup(): Complite!\r\n");
}
	
  
void loop(){
}
