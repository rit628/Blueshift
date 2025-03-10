#include <fstream> 

int main(int argc, char** argv){
    if(argc < 2){
        return 1; 
    }

    
    
    std::ofstream ostream; 
    ostream.open("./testDir/file1.txt"); 
    for(int i = 0; i < argc; i++){
        ostream << argv[i+1]; 
        /*
        if(i != argc-1){
            ostream << " "; 
        }
        */ 
    }
    ostream.close(); 
    
}