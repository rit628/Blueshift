#include <fstream> 
#include <iostream>

int main(int argc, char** argv){
    if(argc < 3){
        std::cerr << "invalid number of arguments provided" << std::endl;
        return 1; 
    }
    
    std::ofstream ostream; 
    ostream.open("./samples/client/" + std::string(argv[1])); 
    ostream << argv[2];
    for(int i = 2; i < argc; i++){
        ostream << " " << argv[i+1];
    }
    ostream.close(); 
    
}