#include <blepp/lescan.h>
#include "blepp/pretty_printers.h"
#include "blepp/gap.h"
#include <stdio.h>
#include <getopt.h>
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>

union ulf
{
    unsigned long ul;
    float f;
};


bool removeDuplicateAdvertise(BLEPP::AdvertisingResponse last,BLEPP::AdvertisingResponse* now, std::size_t size){
	for(std::size_t j = 0; j < last.raw_packet.size(); j++){
		std::string last_str = BLEPP::to_hex(last.raw_packet[j]);
		std::string now_str = BLEPP::to_hex(now->raw_packet[j]);
		if(last_str == now_str){
			return true;
		}
		return false;
	}
	return false;
}

bool checkPESBData(BLEPP::AdvertisingResponse* p, std::size_t size,std::string mac_filter){
	std::cout << "Adress -->" << p->address << std::endl;
	std::cout << "Filter --> " << mac_filter << " Size --> " << mac_filter.size() << std::endl;
	if(p->address.substr(0,mac_filter.size()) == mac_filter){
		return true;
	}
	return false;
}

void pointer_func(const BLEPP::AdvertisingResponse* p, std::size_t size)
{
	
    for (std::size_t i = 0; i < size; ++i){
    	std::cout << "==========================START=============================\n";
		std::cout << "Address = " << p->address << " \n";
		for(std::size_t j = 0; j < p->raw_packet.size(); j++){
			std::vector<uint8_t> v = p->raw_packet[j];
			if(v.size() > 0){
				std::size_t z = 0;
				unsigned int size;
				std::string size_str = BLEPP::to_hex(p->raw_packet[j][z]);
				std::istringstream converter(size_str);
				converter >> std::hex >> size;
				z++;
				uint8_t command = p->raw_packet[j][z];
				std::cout << "command: " << BLEPP::to_hex(command) << "\n";
				if(command == BLEPP::GAP::manufacturer_data){
					std::string registerNumber = "0x" + BLEPP::to_hex(p->raw_packet[j][9]);
					std::cout << "registerNumber: " << registerNumber << "\n";
					std::string number_str = "";
					
					for(std::size_t temp = 10; temp < 14; temp++){
						number_str += BLEPP::to_hex(p->raw_packet[j][temp]);
					}
					std::cout << "raw: " << number_str << "\n";
					ulf u;
					std::stringstream ss(number_str);
				    ss >> std::hex >> u.ul;
				    float number = u.f;
									
					if(registerNumber == "0x00"){
						std::cout << "HTS221 Temperature : "<< number<< "\n";	
					}
					else if(registerNumber == "0x02"){
						std::cout << "Humidity : \n"<< number<< "\n";
					}
					else if(registerNumber == "0x04"){
						std::cout << "LPS25 Temperature : "<< number<< "\n";
					}
					else if(registerNumber == "0x06"){
						std::cout << "LPS25 Pressure : "<< number << "\n";
					}
					else if(registerNumber == "0x08"){
						std::cout << "TSL2561 Luminosity : "<< number<< "\n";	
					}
					else if(registerNumber == "0x0a"){
						std::cout << "Battery Voltage [mV] : "<<number<< "\n";
					}
					else if(registerNumber == "0x0c"){
						std::cout << "Ext Digital PIR [0/1] : "<< number<< "\n";
					}
					else if(registerNumber == "0x0e"){
						std::cout << "External Input 4/20 mA [mA] : "<< number<< "\n";	
					}
					else if(registerNumber == "0x10"){
						std::cout << "External Input 0/10 V [mV] : "<< number<< "\n";	
					}
					else if(registerNumber == "0x12"){
						std::cout << "Status : "<< number<< "\n";
					}	
				}
				std::cout<< "\n";
			}
		}
		std::cout << "==========================STOP=============================\n";
	}       
}



int main(int argc, char** argv)
{
	BLEPP::log_level = BLEPP::LogLevels::Error;
	BLEPP::HCIScanner::ScanType type = BLEPP::HCIScanner::ScanType::Passive;
	BLEPP::HCIScanner::FilterDuplicates filter = BLEPP::HCIScanner::FilterDuplicates::Off;
	BLEPP::HCIScanner scanner(true,filter,type);
	std::string throbber="/|\\-";
	std::string mac_filter = "";
	BLEPP::AdvertisingResponse last_value;
	int i=0,c;
	std::string help = R"X(-[mh]:
	  -m  mac address filter
	  -h  show this message
	)X";
	
	while((c = getopt(argc,argv,"mh")) != -1){
		if(c == 'm'){
			mac_filter = *optarg;
			std::cout << "Mac filter --> " << mac_filter;
			return 0;
		}
		else if(c == 'h')
		{
			std::cout << "Usage: " << argv[0] << " " << help;
			return 0;
		}
		else 
		{
			std::cerr << argv[0] << ":  unknown option " << c << std::endl;
			return 1;
		}
	}
	if(mac_filter == ""){
		std::cout << "-m: parameter is mandatory "<< std::endl;
		return 0;	
	}
	
	while (1) {
		struct timeval timeout;     
		timeout.tv_sec = 0;     
		timeout.tv_usec = 300000;
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(scanner.get_fd(), &fds);
		int err = select(scanner.get_fd()+1, &fds, NULL, NULL,  &timeout);
		if(err < 0 && errno == EINTR)	
			break;
		if(FD_ISSET(scanner.get_fd(), &fds)){
			std::vector<BLEPP::AdvertisingResponse> ads = scanner.get_advertisements();
			if(checkPESBData(ads.data(),ads.size(),mac_filter)){
				if(!removeDuplicateAdvertise(last_value,ads.data(),ads.size())){
					pointer_func(ads.data(), ads.size());
					last_value = *ads.data();
				}	
			}		
		}
		else
			std::cout << throbber[i%4] << "\b" << std::flush;
		i++;	
	}
}
