#pragma once

#include <vector> 
#include <memory> 
#include <unordered_map> 
#include <iostream> 
#include "../libVM/HeapDescriptors.hpp"
#include <cstring> 
#include <boost/type_index.hpp>


// Dynamic Message metadata elements 
#define CONTAINER_ELEMENT "__CONTAINER_ELEMENT__"
#define STRUCT_ATTR "__MSG_ATTR__"
class HeapDescriptor; 
class VectorDescriptor; 
class PrimDescriptor; 
class MapDescriptor;

enum class Desctype{
    VECTOR, 
    MAP, 
    UINT32, 
    FLOAT, 
    STRING, 
    CHAR, 
    BOOL, 
    NONE, 
}; 

enum class Primtype{
    FLOAT, 
    INT, 
    BOOL,
    CHAR, 
    STRING, 
    // Implies the primative is no a primative type, do vistor pattern on elements
    VECTOR, 
    NONE, 
    MAP, 
}; 


// Message Header (Data Map Size ): 
struct MsgHeader{
    // Size of lump data: // (Total message size)
    uint32_t lumpData_sz; 
    // Number of Descriptors (list of heap descriptor size) 
    uint32_t descList_sz; 
    // Descriptor size
    uint32_t DescOffset; 
    // lump offset: 
    uint32_t lumpOffset;
    // Data map size: 
    uint32_t dataMap_location; 
}; 


// Descriptor Object: 
struct Descriptor{
    // Object Type: 
    Desctype descType; 
    // Number of elements in list
    uint32_t numElements = 0; 
    // Size of element (1 if singular element, n for containers with n elements)
    uint8_t eleSize = 0; 
    // Offset in lump element
    uint32_t lumpOffset; 
    // Container Type 
    Primtype contained_type;
};


/*
    The dynamic messaging object (in this version) does no typechecking on its own. 
    All typechecking is assumed to have been done at compile type in the analyzer step. 
*/

class DynamicMessage{
    private: 
        // Header information
        MsgHeader header;
        // Raw/Lump data
        std::vector<char> data; 
        // Descriptor list: 
        std::vector<Descriptor> Descriptors;
        // attribute map // Maps strings to attributes 
        std::unordered_map<std::string, uint32_t> attributeMap; 


    template <typename T> 
    std::pair<Primtype, uint8_t> peekType(T &obj){
        std::string type_name = boost::typeindex::type_id_with_cvr<T>().pretty_name(); 

        //std::cout<<"Object: "<<type_name<<std::endl;

        if(this->startsWith("float", type_name)){
            return std::make_pair(Primtype::FLOAT, static_cast<uint8_t>(sizeof(float)));
        }
        else if(this->startsWith("int", type_name)){
            return std::make_pair(Primtype::INT, static_cast<uint8_t>(sizeof(int)));
        }
        else if(this->startsWith("char", type_name)){
            return std::make_pair(Primtype::CHAR, static_cast<uint8_t>(sizeof(char)));
        }
        else if(this->startsWith("bool", type_name)){
            return std::make_pair(Primtype::BOOL, static_cast<uint8_t>(sizeof(bool)));
        }
        else if(this->startsWith("std::vector", type_name)){
            return std::make_pair(Primtype::VECTOR, static_cast<uint8_t>(0)); 
        }
        else if(this->startsWith("std::__cxx11::basic_string", type_name)){
            return std::make_pair(Primtype::STRING, static_cast<uint8_t>(0)); 

        }
        else if(this->startsWith("std::unordered_map", type_name)){
            return std::make_pair(Primtype::MAP, static_cast<uint8_t>(0)); 
        }
        else{
            return std::make_pair(Primtype::NONE, static_cast<uint8_t>(0));
        }
    }


    // Deserialize helper for primative types: 
    template <typename T>
    void deserialize(int descPos, T& primRecv, int &travDesc){
        // Get the descAlias
        Descriptor desc = this->Descriptors[descPos]; 
        int objSize = desc.eleSize; 
        int count = desc.numElements; 
        int objectOffset = desc.lumpOffset; 
        travDesc = 1; 

        // copy the object into the primRecv memory
        std::memcpy(&primRecv, &this->data[objectOffset], count * objSize);
    }

    // deserialize string
    void deserialize(int descPos, std::string& reciever, int &travDesc){
        Descriptor desc = this->Descriptors[descPos]; 
        int strSize = desc.numElements; 
        int charSize = desc.eleSize; 
        int objectOffset = desc.lumpOffset; 
        travDesc = 1; 


        // Copy the object into the receiver object 
        reciever.resize(strSize); 
        std::memcpy(&reciever[0], &this->data[objectOffset], strSize * charSize); 
    }

    // deserialize vector
    template <typename T> 
    void deserialize(int descPos, std::vector<T> &reciever, int &travDesc){
        Descriptor desc = this->Descriptors[descPos]; 
        int vecSize = desc.numElements; 
        int offset = desc.lumpOffset; 
        // create the object
        T obj; 
        travDesc = 1;
        auto testCheck = peekType(obj); 

        if(testCheck.second != 0){ 
            reciever.resize(vecSize); 
            // Copy all the data in one step (if container for primatives): 
            int objSize = testCheck.second; 
            int totalCpyMemory = vecSize * objSize;
            std::memcpy(reciever.data(), this->data.data() + offset, totalCpyMemory);  
        }
        else{
            int fwd = 0; 
            for(int i = 1 ; i <= vecSize; i++){
                T pushObj; 
                this->deserialize(descPos + travDesc, pushObj, fwd); 
                reciever.push_back(pushObj); 
                travDesc += fwd; 
            }
        }
    }


    template <typename K, typename V> 
    void deserialize(int descPos, std::unordered_map<K, V> &reciever, int &descTrav){
        Descriptor desc = this->Descriptors[descPos]; 
        int mapEle = desc.numElements; 
        int offset = desc.lumpOffset; 

        int j = 0; 
        int i = 1; 

        for(int j = 0; j < mapEle; j++){
            // Odd offset elements are keys
            K keyObj; 
            int trav; 
            this->deserialize(descPos + i, keyObj, trav); 
            i += trav; 

            // Even offset elements are values (for now we assume )
            V valueObj; 
            this->deserialize(descPos + i, valueObj, trav); 
            reciever.emplace(keyObj, valueObj); 
            i += trav; 
        }

        descTrav = i; 
    }

    public: 
    DynamicMessage() = default; 

    // Utility Function
    static bool startsWith(std::string_view searchString, std::string &srcString){
        if(searchString.size() < 1){
            return false; 
        }

        std::string fsub = srcString.substr(0, searchString.size()); 
        if(fsub == searchString){
            return true; 
        }
        else{
            return false; 
        }
    }

    // Field Creation step

    template <typename T> 
    void createField(std::string fieldName, T &messageObj){
        if(fieldName != CONTAINER_ELEMENT && fieldName != STRUCT_ATTR){
            if(this->attributeMap.find(fieldName) != this->attributeMap.end()){
                throw std::invalid_argument("Field Name " + fieldName + " is already set!"); 
            }
            else{
                this->attributeMap[fieldName] = this->Descriptors.size(); 
            }

        }

        std::string type_name = boost::typeindex::type_id_with_cvr<T>().pretty_name(); 

        Descriptor desc; 
        // test if vector
        if(this->startsWith("int", type_name) || this->startsWith("unsigned int", type_name)){
            desc.descType = Desctype::UINT32; 
        }
        else if(this->startsWith("float", type_name)){
            desc.descType = Desctype::FLOAT; 
        }
        else if(this->startsWith("char", type_name)){
            desc.descType = Desctype::CHAR; 
        }
        else if(this->startsWith("bool", type_name)){
            desc.descType = Desctype::BOOL; 
        }
        else{
            throw std::invalid_argument("Object of unknown type " + (type_name) + " attempted to be serialized"); 
        }

        desc.numElements = 1; 
        desc.eleSize = sizeof(T); 
        desc.lumpOffset = this->data.size(); 
        desc.contained_type = Primtype::NONE; 
        
        // Push the descriptor back
        this->Descriptors.push_back(desc);
        
        // Push into lump_data: 
        int new_sz = this->data.size() + desc.eleSize; 
        this->data.resize(new_sz); 
        std::memcpy(this->data.data() + desc.lumpOffset, &messageObj, desc.eleSize); 
    }



    void createField(std::string fieldName, std::string& messageObj){
        // If the object is not a nested object and not already a field, then make new field.
        if(fieldName != CONTAINER_ELEMENT && fieldName != STRUCT_ATTR){
            if(this->attributeMap.find(fieldName) != this->attributeMap.end()){
                throw std::invalid_argument("Field " + fieldName  + " already set!"); 
            }   
            else{
                this->attributeMap[fieldName] = this->Descriptors.size();
            }
        }

        Descriptor desc; 

        // Create and push the descriptor object: 
        desc.descType = Desctype::STRING; 
        desc.eleSize = sizeof(char); 
        desc.numElements = messageObj.size(); 
        desc.lumpOffset = this->data.size(); 
        this->Descriptors.push_back(desc); 
        desc.contained_type = Primtype::CHAR; 

        // Insert the string into the header: 
        int new_sz = this->data.size() + (sizeof(char) *  desc.numElements);
        this->data.resize(new_sz); 
        std::memcpy(this->data.data() + desc.lumpOffset,  &messageObj[0] , (sizeof(char) * desc.numElements)); 
    }


    template <typename T> 
    void createField(std::string fieldName, std::vector<T> &messageObj){
        // If the object is not a nested object and not already a field, then make new field.
        if(fieldName != CONTAINER_ELEMENT && fieldName != STRUCT_ATTR){
            if(this->attributeMap.find(fieldName) != this->attributeMap.end()){
                throw std::invalid_argument("Field " + fieldName  + " already set!"); 
            }   
            else{
                this->attributeMap[fieldName] = this->Descriptors.size();
            }
        }

        Descriptor desc; 

        if(messageObj.empty()){
            throw std::invalid_argument("Cannot input an empty container!");  
        }

        T obj; 
        auto containerObj = this->peekType(obj); 

        // Optimization techique (for containers with constant size elements (primatives, dont create headers for each!))
        desc.lumpOffset = this->data.size(); 
        desc.numElements = messageObj.size(); 
        desc.eleSize = containerObj.second; 
        desc.descType = Desctype::VECTOR; 

        desc.contained_type = containerObj.first; 
        
        this->Descriptors.push_back(desc); 

        if(desc.eleSize != 0){
            // Copy the entire memory into the lump data: 
            int new_sz = this->data.size() + (desc.eleSize * desc.numElements); 
            this->data.resize(new_sz); 
            std::memcpy(this->data.data() + desc.lumpOffset, messageObj.data(), desc.eleSize * desc.numElements); 
        }
        else{ 
            for(auto element : messageObj){
                this->createField(CONTAINER_ELEMENT ,element); 
            }
        }
    }

    template <typename K, typename V> 
    void createField(std::string fieldName, std::unordered_map<K,V> &messageObj){
        if(fieldName != CONTAINER_ELEMENT && fieldName != STRUCT_ATTR){
            if(this->attributeMap.find(fieldName) != this->attributeMap.end()){
                    throw std::invalid_argument("Field Name " + fieldName + " is already set!"); 
            }
            else{
                    this->attributeMap[fieldName] = this->Descriptors.size(); 
            }
        }

        Descriptor desc; 

        if(messageObj.empty()){
            throw std::invalid_argument("Container object cannot be empty!"); 
        }

        desc.descType = Desctype::MAP; 
        desc.eleSize = -1; 
        desc.lumpOffset = this->data.size(); 
        // Times this by 2 when reading to read all descriptors: 
        desc.numElements = messageObj.size(); 

        V value_sample; 
        desc.contained_type = peekType(value_sample).first; 

        this->Descriptors.push_back(desc); 

        // Do not add the vector memory pull optimization (oh god no)
        for(auto &newObj : messageObj){
            K keyObj = newObj.first; 
            V valueObj = newObj.second; 

            this->createField(CONTAINER_ELEMENT, keyObj); 
            this->createField(CONTAINER_ELEMENT, valueObj); 
        }
    }

    //Serialization and capture: 
    std::vector<char> Serialize(){

        std::vector<char> fullMessage; 
        this->header.dataMap_location = this->Descriptors.size(); 
        this->createField(STRUCT_ATTR, this->attributeMap); 

        // Write all the data into the appropriate location: 
        int lump_offset = sizeof(header);
        int descriptor_offset = lump_offset + (this->data.size() * sizeof(char));

        this->header.lumpOffset = lump_offset; 
        this->header.DescOffset = descriptor_offset; 
        this->header.descList_sz = this->Descriptors.size(); 
        this->header.lumpData_sz = this->data.size(); 

        // Total send size: 
        int total_size = sizeof(header) + (this->data.size() * sizeof(char)) + (sizeof(Descriptor) * this->Descriptors.size()); 

        // Insert the header at the beginning of fullMessage
        fullMessage.insert(fullMessage.end(), 
                        reinterpret_cast<const char*>(&this->header), 
                        reinterpret_cast<const char*>(&this->header) + sizeof(this->header));

        // Insert the data at the lump offset
        fullMessage.insert(fullMessage.end(), 
                        this->data.begin(), 
                        this->data.end());

        // Insert the Descriptors at the descriptor offset
        const char* descriptorStart = reinterpret_cast<const char*>(this->Descriptors.data());
        const char* descriptorEnd = descriptorStart + sizeof(Descriptor) * this->Descriptors.size();
        fullMessage.insert(fullMessage.end(), 
                        descriptorStart, 
                        descriptorEnd);


        return fullMessage; 
    }

    MsgHeader getHeader(){
        return this->header; 
    }


    // Used to capture an incomming object based on the descriptor: 

    void Capture(std::vector<char> &recvString){
        MsgHeader obj; 

        // Copy Header
        std::memcpy(&obj, recvString.data(), sizeof(MsgHeader)); 
        this->header = obj; 

        // Build the lump data; 
        this->data.resize(obj.lumpData_sz); 
        std::memcpy(this->data.data(), recvString.data() + obj.lumpOffset, obj.lumpData_sz * sizeof(char)); 

        // Build the descriptor object
        this->Descriptors.resize(obj.descList_sz); 
        std::memcpy(this->Descriptors.data(), recvString.data() + obj.DescOffset, obj.descList_sz * sizeof(Descriptor));

        // Deseriaize the packaged 
        int idk;
        int map_location = obj.dataMap_location;  
        this->deserialize(map_location ,this->attributeMap, idk); 

        // Erase the last elements after the location
        this->Descriptors.erase(this->Descriptors.begin() + map_location, this->Descriptors.end());

        // Erase the heap descriptors at the map level: 
        this->attributeMap.erase(STRUCT_ATTR);
        
        // Clear the input string
        recvString.clear();  
    }; 


    // Unpacking functions for recievers: 
    // Unpack Primative
    template <typename T> 
    void unpack(std::string key, T& primRecv){
        int descAlias = this->attributeMap[key]; 
        int trav; 
        this->deserialize(descAlias, primRecv, trav); 
    }

    // Unpack string
    void unpack(std::string key, std::string& strRecv){
        int descAlias = this->attributeMap[key]; 
        int trav; 
        this->deserialize(descAlias, strRecv, trav);    
    }

    // Unpack vector
    template <typename T> 
    void unpack(std::string key, std::vector<T>& vecRecv){
        int descAlias = this->attributeMap[key];
        int trav; 
        this->deserialize(descAlias, vecRecv, trav); 
    } 

    // Unpack unordered_map
    template <typename K, typename V> 
    void unpack(std::string key, std::unordered_map<K, V>& mapRecv){
        int descAlias = this->attributeMap[key]; 
        int trav; 
        this->deserialize(descAlias, mapRecv, trav); 

    }


    /*
        Dynamic Message -> Heap Tree PIPELINE (all Dynamic Messages -> Map Heap Descriptor)
    */

    std::string getContType(Primtype type){
        switch(type){
            case(Primtype::FLOAT) : {
                return "FLOAT"; 
            }
            case(Primtype::INT) : {
                return "INT"; 
            }
            case(Primtype::CHAR) : {
                return "CHAR"; 
            }
            case(Primtype::BOOL) : {
                return "BOOL"; 
            }
            case(Primtype::STRING) : {
                return "STRING"; 
            }
            case(Primtype::VECTOR) : {
                return "VECTOR"; 
            }
            case(Primtype::MAP) : {
                return "MAP"; 
            }
            default : {
                return "NONE"; 
            }
        }

    }

    std::shared_ptr<HeapDescriptor> toTree(){
        auto global_map = std::make_shared<MapDescriptor>("ANY");

        for(auto obj : this->attributeMap){ 
            uint32_t descIndex = obj.second; 
            auto root_descriptor = this->Descriptors[descIndex]; 
            std::shared_ptr<HeapDescriptor> heapObj; 

            switch(root_descriptor.descType) {
                case(Desctype::MAP) : {
                    int trav = 0;
                    heapObj = this->mapToTree(descIndex, trav); 
                    break; 
                }   

                case(Desctype::VECTOR) : {
                    int trav = 0; 
                    heapObj = this->vecToTree(descIndex, trav); 
                    break; 
                }
                default : {
                    int trav = 0; 
                    heapObj = this->primToTree(descIndex, trav); 
                    break; 
                }
            }

            global_map->map->emplace(obj.first, heapObj); 
 
        } 

        return global_map; 
    }



    private: 

    std::shared_ptr<HeapDescriptor> mapToTree(int descIndex, int &trav){
        auto obj_desc = this->Descriptors[descIndex]; 
        int sub_offset = 1; 

        int count = obj_desc.numElements;
        Primtype cont_type = obj_desc.contained_type; 
        std::string josh = getContType(cont_type); 

        auto map_object = std::make_shared<MapDescriptor>(josh); 

        for(int i = 0; i < count; i++){
            // We assume are strings are keys for now; 
            std::string key; 
            int trav; 
            this->deserialize(descIndex + sub_offset, key, trav);
            sub_offset += trav; 
            std::shared_ptr<HeapDescriptor> object; 

            switch(cont_type){
                case(Primtype::VECTOR) : {
                    object = this->vecToTree(descIndex + sub_offset ,trav); 
                    break; 
                }

                case(Primtype::MAP) : {
                    object = this->mapToTree(descIndex + sub_offset, trav); 
                    break; 
                }
                // Build the 
                default :{
                    object = this->primToTree(descIndex + sub_offset, trav); 
                    break; 
                }

            }

            map_object->map->operator[](key) = object; 
            sub_offset += trav; 
        }

        return map_object; 
    }

    template <typename T> 
    std::shared_ptr<HeapDescriptor> primVecExtract(Descriptor desc, T &cpy){
        int ptrPos = desc.lumpOffset; 
        int eleCount = desc.numElements; 

        std::string cont_type = getContType(desc.contained_type); 

        
        auto vecDesc = std::make_shared<VectorDescriptor>(cont_type); 

        for(int i = 0; i < eleCount; i++){
            T new_cpy; 
            std::memcpy(&new_cpy, this->data.data() + ptrPos, sizeof(T)); 
            ptrPos += sizeof(T); 
            auto cpyObject = std::make_shared<PrimDescriptor>(new_cpy); 
            vecDesc->vector->push_back(cpyObject); 
        }

        return vecDesc; 
    }


    std::shared_ptr<HeapDescriptor> vecToTree(int descIndex, int &trav){
        auto obj_desc = this->Descriptors[descIndex]; 
        auto contType = obj_desc.contained_type;
 
        trav = 1; 

        switch(contType){
            case(Primtype::FLOAT) : {
                float t; 
                return primVecExtract(obj_desc, t); 
                break; 
            }
            case (Primtype::INT) : {
                int t; 
                return primVecExtract(obj_desc, t); 
                break; 
            }
            case (Primtype::BOOL): {
                bool t; 
                return primVecExtract(obj_desc, t); 
                break; 
            }
            case (Primtype::CHAR) : {
                char t; 
                return primVecExtract(obj_desc, t);  
                break; 
            }
            default : { 
                int count = obj_desc.numElements; 
                int fwd = 0; 

                if(contType == Primtype::MAP){
                    auto new_vector = std::make_shared<VectorDescriptor>("MAP");
                    for(int i = 0; i < count; i++){
                        auto add_map = mapToTree(descIndex + trav, fwd); 
                        new_vector->vector->push_back(add_map); 
                        trav += fwd; 
                    }          
                    return new_vector;    
                }
                else if(contType == Primtype::VECTOR){
                    auto new_vector = std::make_shared<VectorDescriptor>("VECTOR");
                    for(int i = 0; i < count; i++){
                        auto add_vector = vecToTree(descIndex + trav, fwd); 
                        new_vector->vector->push_back(add_vector); 
                        trav += fwd; 
                    }
                    return new_vector; 
                }
                else if(contType == Primtype::STRING){
                    auto new_vector = std::make_shared<VectorDescriptor>("STRING"); 
                    for(int i = 0; i < count; i++){
                        auto add_string = primToTree(descIndex + trav, fwd);
                        new_vector->vector->push_back(add_string); 
                        trav += fwd;  
                    }
                    return new_vector; 
                }
                else{
                    throw std::runtime_error("I dont fucking know aynmore man"); 
                    return NULL; 
                }


            }

        }; 
        
    }

    std::shared_ptr<HeapDescriptor> primToTree(int descIndex, int &trav1){  
        auto descObj = this->Descriptors[descIndex]; 
        if(descObj.descType == Desctype::UINT32){
            int prim1; 
            this->deserialize(descIndex, prim1, trav1); 
            trav1 = 1;  
            return std::make_shared<PrimDescriptor>(prim1); 
        }
        else if(descObj.descType == Desctype::CHAR){
            char char1; 
            this->deserialize(descIndex, char1, trav1); 
            trav1 = 1; 
            return std::make_shared<PrimDescriptor>(char1); 
        }
        else if(descObj.descType == Desctype::BOOL){
            bool bool1; 
            this->deserialize(descIndex, bool1, trav1); 
            trav1 = 1; 
            return std::make_shared<PrimDescriptor>(bool1); 
        }
        else if(descObj.descType == Desctype::FLOAT){
            float f1; 
            this->deserialize(descIndex, f1, trav1); 
            trav1 = 1; 
            return std::make_shared<PrimDescriptor>(f1); 

        }
        else if(descObj.descType == Desctype::STRING){
            std::string str1; 
            this->deserialize(descIndex, str1, trav1); 
            trav1 = 1; 
            return std::make_shared<PrimDescriptor>(str1); 
        }
        else{
            throw std::invalid_argument("Invalid Descriptor Index, not a leaf node type"); 
        }
    }


   private: 

   template <typename T> 
   void writeToPrimVector(std::vector<T> &temp_vector, std::shared_ptr<VectorDescriptor> vecDesc){ 
        for(int i = 0 ; i < vecDesc->vector->size(); i++){
            if(auto primDesc = std::dynamic_pointer_cast<PrimDescriptor>(vecDesc->vector->at(i))){
                if(std::holds_alternative<T>(primDesc->variant)){
                    T item = std::get<T>(primDesc->variant); 
                    temp_vector.push_back(item); 
                }
                else{
                    throw std::runtime_error("Unexpected contained type encountered"); 
                }
            }
            else{
                throw std::runtime_error("Unexpected contained type encountered"); 
            }
        }
   }

    // Make and 
   void makeFromHeap(std::string name, std::shared_ptr<HeapDescriptor> heapObj){
        if(auto mapDesc = std::dynamic_pointer_cast<MapDescriptor>(heapObj)){
            Descriptor desc; 
            desc.descType = Desctype::MAP;
            desc.eleSize  = 0;
            desc.lumpOffset = this->data.size();
            desc.numElements = mapDesc->map->size();

            this->Descriptors.push_back(desc); 
    
            for(auto kvPair : *mapDesc->map){
                // Make the string object
                this->createField(CONTAINER_ELEMENT, kvPair.first); 
                // Write the heap object
                makeFromHeap(CONTAINER_ELEMENT, kvPair.second); 
            }
        }
        else if(auto vecDesc = std::dynamic_pointer_cast<VectorDescriptor>(heapObj)){
         
            // Storage of constant size vectors
            switch(vecDesc->contType){
                    case(ContainerType::INT) : {
                        std::vector<int> intVector; 
                        writeToPrimVector(intVector, vecDesc); 
                        this->createField(name, intVector);    
                        break; 
                    }
                    case(ContainerType::FLOAT) : {
                        std::vector<float> floatVector; 
                        writeToPrimVector(floatVector, vecDesc); 
                        this->createField(name, floatVector); 
                        break; 
                    }
                    case(ContainerType::STRING) : {
                        std::vector<std::string> strVector; 
                        writeToPrimVector(strVector, vecDesc); 
                        this->createField(name, strVector); 
                        break; 
                    }              
                    default : {
                        Descriptor desc; 
                        desc.descType = Desctype::VECTOR;
                        desc.eleSize = 0;
                        desc.lumpOffset = this->data.size(); 
                        desc.numElements = vecDesc->vector->size();

                        this->Descriptors.push_back(desc); 
                        
                        for(auto element : *vecDesc->vector){
                            makeFromHeap(CONTAINER_ELEMENT, element); 
                        }
                    }
                }
        }
        else if(auto primDesc = std::dynamic_pointer_cast<PrimDescriptor>(heapObj)){
            // Prim Desc should only be used for standalone vector elements; 
            auto var = primDesc->variant; 

            if(std::holds_alternative<int>(var)){
                // Should be fine
                int jamar = std::get<int>(var); 
                this->createField(name, jamar); 
                
            }
            else if(std::holds_alternative<float>(var)){
                float jamar = std::get<float>(var); 
                this->createField(name, jamar); 

            }
            else if(std::holds_alternative<bool>(var)){
                bool jamar = std::get<bool>(var); 
                this->createField(name, jamar); 
            }
            else if(std::holds_alternative<std::string>(var)){
                std::string jamar = std::get<std::string>(var); 
                this->createField(name, jamar); 
            }
            else{
                std::cout<<"Non primative variant attempted to be derived from Prim Descriptor"<<std::endl; 
            }
        }
   }

   
    /*
    
        HeapTree -> Dynamic Message Pipeline: 

    */

   public:
    // Takes a HeapTree Map object and converts it into a mapDesc object 
   void makeFromRoot(std::shared_ptr<HeapDescriptor> heapDesc){
        if(auto mapDesc = std::dynamic_pointer_cast<MapDescriptor>(heapDesc)){
            for(auto pair : *mapDesc->map){
                makeFromHeap(pair.first, pair.second); 
            }
         }
        else{
            throw std::invalid_argument("Root Heap Tree must be a map type for serialization"); 
        }
   }

}; 


