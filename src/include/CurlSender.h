#include <string>
#include <fstream>
#include <cstdlib>
#include <iostream>
#include <sstream>

class CurlSender{
public:
	//Constructor: reads wecom_url from ini_path
	CurlSender(const std::string& ini_path){
		_url = read_url_from_ini(ini_path);
		std::cout << "read url : " << _url << std::endl;
	}

	//Send data to url by curl command
	bool send(const char* text) {
		char command[2048];
		std::string curl = "curl ";
		std::string url = R"("https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=574b86a2-a2c2-405c-8a4b-f1bc25fdf216")"; 
		std::string param = R"( -H "Content-Type: application/json" -d "{\"msgtype\": \"text\", \"text\": {\"content\": \"%s\"}}")";
		sprintf(command, (curl + url + param).c_str(), text);
		system(command);
	}
	
private:
	std::string _url;

	//Read wecom_url from ini file
	std::string read_url_from_ini(const std::string& ini_path) {
		std::ifstream file(ini_path);
		std::string line, url;
		while (std::getline(file, line)){
			if (line.find("wecom_url=") != std::string::npos) {
				url = line.substr(line.find("wecom_url=") + 10);
				break;
			}
		}
		return url;
	}

	//Escape double quotes and backslashes in data
	std::string escape(const std::string& s) {
		std::string out;
		for (char c: s) {
			if (c == '\\') out += "\\\\";
			else if (c == '"') out += "\\\"";
			else out += c;
		}
		return out;
	}
};
