#pragma once

#include <concepts>
#include <cstdint>
#include <cstring>
#include <deque>
#include <stdexcept>
#include <variant>
#include <vector> 
#include <memory> 
#include <unordered_map> 
#include <map>
#include <iostream> 
#include "bls_types.hpp"
#include "typedefs.hpp"



// Dynamic Message metadata elements 
#define CONTAINER_ELEMENT "__CONTAINER_ELEMENT__"
#define STRUCT_ATTR "__MSG_ATTR__"
class HeapDescriptor; 
class VectorDescriptor; 
class PrimDescriptor; 
class MapDescriptor;


// Message Header (Data Map Size ): 
struct MsgHeader{
    // Size of lump data: // (Total message size)
    uint32_t lumpData_sz = 0; 
    // Number of Descriptors (list of heap descriptor size) 
    uint32_t descList_sz = 0; 
    // Descriptor size
    uint32_t DescOffset = 0; 
    // lump offset: 
    uint32_t lumpOffset = 0;
    // Data map size: 
    uint32_t dataMap_location = 0; 
}; 


// Descriptor Object: 
struct Descriptor{
    // Object Type: 
    TYPE descType = TYPE::NONE; 
    // Number of elements in list
    uint32_t numElements = 0; 
    // Size of element (1 if singular element, n for containers with n elements)
    uint8_t eleSize = 0; 
    // Offset in lump element
    uint32_t lumpOffset = 0; 
    // Container Type 
    TYPE contained_type = TYPE::NONE;
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
    std::pair<TYPE, uint8_t> peekType(T &obj){
        if constexpr (TypeDef::Integer<T>) {
            return std::make_pair(TYPE::int_t, static_cast<uint8_t>(sizeof(T)));
        }
        else if constexpr (TypeDef::Float<T>) {
            return std::make_pair(TYPE::float_t, static_cast<uint8_t>(sizeof(T)));
        }
        else if constexpr (TypeDef::Boolean<T>) {
            return std::make_pair(TYPE::bool_t, static_cast<uint8_t>(sizeof(T)));
        }
        else if constexpr (TypeDef::String<T>) {
            return std::make_pair(TYPE::string_t, static_cast<uint8_t>(0));
        }
        else if constexpr (TypeDef::List<T>) {
            return std::make_pair(TYPE::list_t, static_cast<uint8_t>(0));
        }
        else if constexpr (TypeDef::Map<T>) {
            return std::make_pair(TYPE::map_t, static_cast<uint8_t>(0));
        }
        else {
            return std::make_pair(TYPE::NONE, static_cast<uint8_t>(0));
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

        Descriptor desc; 
        // test if vector
        if constexpr (TypeDef::Integer<T>) {
            desc.descType = TYPE::int_t;
        }
        else if constexpr (TypeDef::Float<T>) {
            desc.descType = TYPE::float_t;
        }
        else if constexpr (TypeDef::Boolean<T>) {
            desc.descType = TYPE::bool_t;
        }
        else {
            desc.descType = TYPE::ANY; 
        }

        desc.numElements = 1; 
        desc.eleSize = sizeof(T); 
        desc.lumpOffset = this->data.size(); 
        desc.contained_type = TYPE::NONE; 
        
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
        desc.descType = TYPE::string_t; 
        desc.eleSize = sizeof(char); 
        desc.numElements = messageObj.size(); 
        desc.lumpOffset = this->data.size(); 
        this->Descriptors.push_back(desc); 
        desc.contained_type = TYPE::ANY; 

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
            
            throw std::invalid_argument("Vector cannot be empty!");  
        }

        T obj; 
        auto containerObj = this->peekType(obj); 

        // Optimization techique (for containers with constant size elements (primatives, dont create headers for each!))
        desc.lumpOffset = this->data.size(); 
        desc.numElements = messageObj.size(); 
        desc.eleSize = containerObj.second; 
        desc.descType = TYPE::list_t; 

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
            throw std::invalid_argument("map cannot be empty!"); 
        }

        desc.descType = TYPE::map_t; 
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

    template <typename T>
    void packStates(T& states) {
        using namespace TypeDef;
        #define DEVTYPE_BEGIN(name, ...) \
        if constexpr (std::same_as<T, name>) { 
        #define ATTRIBUTE(name, ...) \
            this->createField(#name, states.name);
        #define DEVTYPE_END \
            return; \
        }
        #include "DEVTYPES.LIST"
        #undef DEVTYPE_BEGIN
        #undef ATTRIBUTE
        #undef DEVTYPE_END
        throw std::runtime_error("invalid states struct type");
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
        if (recvString.empty()) return;
        
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

    bool hasField(std::string targ_field){
        return this->attributeMap.contains(targ_field);
    }

    // Unpacking functions for recievers: 
    template <typename T> 
    void unpack(std::string key, T& recv){
        if(this->attributeMap.contains(key)){
            int descAlias = this->attributeMap[key]; 
            int trav;
            this->deserialize(descAlias, recv, trav); 
        }
        else{
            throw std::invalid_argument("Dynamic Message does not contain key with name: " + key);
        }

        
    }

    template <typename T>
    void unpackStates(T& states) {
        using namespace TypeDef;
        #define DEVTYPE_BEGIN(name, ...) \
        if constexpr (std::same_as<T, name>) { 
        #define ATTRIBUTE(name, ...) \
            if (this->hasField(#name)) { \
                this->unpack(#name, states.name); \
            } \
            else { \
                throw std::runtime_error("dynamic message is missing field " #name " needed for requested states"); \
            }
        #define DEVTYPE_END \
            return; \
        }
        #include "DEVTYPES.LIST"
        #undef DEVTYPE_BEGIN
        #undef ATTRIBUTE
        #undef DEVTYPE_END
        throw std::runtime_error("invalid states struct type");
    }

    /*
        Dynamic Message -> Heap Tree PIPELINE (all Dynamic Messages -> Map Heap Descriptor)
    */

    std::shared_ptr<HeapDescriptor> toTree(){
        auto global_map = std::make_shared<MapDescriptor>(TYPE::ANY, TYPE::string_t, TYPE::NONE);

        for(auto obj : this->attributeMap){ 
            uint32_t descIndex = obj.second; 
            auto root_descriptor = this->Descriptors[descIndex]; 
            BlsType heapObj; 

            switch(root_descriptor.descType) {
                case TYPE::map_t : {
                    int trav = 0;
                    heapObj = this->mapToTree(descIndex, trav); 
                    break; 
                }   

                case TYPE::list_t : {
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
        TYPE cont_type = obj_desc.contained_type; 

        auto map_object = std::make_shared<MapDescriptor>(cont_type); 

        for(int i = 0; i < count; i++){
            // We assume are strings are keys for now; 
            std::string key; 
            int new_trav; 
            this->deserialize(descIndex + sub_offset, key, new_trav);
            sub_offset += new_trav; 
            BlsType object; 

            switch(cont_type){
                case(TYPE::list_t) : {
                    object = this->vecToTree(descIndex + sub_offset ,new_trav); 
                    break; 
                }

                case(TYPE::map_t) : {
                    object = this->mapToTree(descIndex + sub_offset, new_trav); 
                    break; 
                }
                // Build the 
                default : {
                    object = this->primToTree(descIndex + sub_offset, new_trav); 
                    break; 
                }

            }

            map_object->map->operator[](key) = object; 
            sub_offset += new_trav; 
        }

        trav = sub_offset; 

        return map_object; 
    }

    template <typename T> 
    std::shared_ptr<HeapDescriptor> primVecExtract(Descriptor desc, T &cpy){
        int ptrPos = desc.lumpOffset; 
        int eleCount = desc.numElements; 
        auto vecDesc = std::make_shared<VectorDescriptor>(desc.contained_type); 

        for(int i = 0; i < eleCount; i++){
            T new_cpy; 
            std::memcpy(&new_cpy, this->data.data() + ptrPos, sizeof(T)); 
            ptrPos += sizeof(T); 
            auto cpyObject = BlsType(new_cpy); 
            vecDesc->vector->push_back(cpyObject); 
        }

        return vecDesc; 
    }


    std::shared_ptr<HeapDescriptor> vecToTree(int descIndex, int &trav){
        auto obj_desc = this->Descriptors[descIndex]; 
        auto contType = obj_desc.contained_type;
        int count = obj_desc.numElements; 
        int fwd = 0; 
 
        trav = 1; 

        switch(contType){
            case TYPE::float_t : {
                double t; 
                return primVecExtract(obj_desc, t); 
                break; 
            }
            case TYPE::int_t : {
                int64_t t; 
                return primVecExtract(obj_desc, t); 
                break; 
            }
            case TYPE::bool_t: {
                bool t; 
                return primVecExtract(obj_desc, t); 
                break; 
            }
            case TYPE::map_t: {
                auto new_vector = std::make_shared<VectorDescriptor>(TYPE::map_t);
                for(int i = 0; i < count; i++){
                    auto add_map = mapToTree(descIndex + trav, fwd); 
                    new_vector->vector->push_back(add_map); 
                    trav += fwd; 
                }          
                return new_vector;   
            }
            case TYPE::list_t: {
                auto new_vector = std::make_shared<VectorDescriptor>(TYPE::list_t);
                for(int i = 0; i < count; i++){
                    auto add_vector = vecToTree(descIndex + trav, fwd); 
                    new_vector->vector->push_back(add_vector); 
                    trav += fwd; 
                }
                return new_vector; 
            }
            case TYPE::string_t: {
                auto new_vector = std::make_shared<VectorDescriptor>(TYPE::string_t); 
                for(int i = 0; i < count; i++){
                    auto add_string = primToTree(descIndex + trav, fwd);
                    new_vector->vector->push_back(add_string); 
                    trav += fwd;  
                }
                return new_vector; 
            }
            default : {
                throw std::runtime_error("Unknown vector type"); 
                return NULL; 
            }
        }; 
    }

    BlsType primToTree(int descIndex, int &trav1){  
        auto descObj = this->Descriptors[descIndex];
        BlsType prim;
        switch (descObj.descType) {
            case TYPE::int_t:
                prim.emplace<int64_t>();
            break;
            case TYPE::bool_t:
                prim.emplace<bool>();
            break;
            case TYPE::float_t:
                prim.emplace<double>();
            break;
            case TYPE::string_t:
                prim.emplace<std::string>();
            break;
            default:
                throw std::invalid_argument("Invalid Descriptor Index, not a leaf node type"); 
            break;
        }
        std::visit([&, this](auto&& p) { this->deserialize(descIndex, p, trav1); }, prim);
        trav1 = 1;
        return prim;
    }


   private: 

   template <typename T> 
   void writeToPrimVector(std::vector<T> &temp_vector, std::shared_ptr<VectorDescriptor> vecDesc){ 
        for(int i = 0 ; i < vecDesc->vector->size(); i++){
            auto prim = vecDesc->vector->at(i);
            if(std::holds_alternative<T>(prim)){
                T item = std::get<T>(prim); 
                temp_vector.push_back(item); 
            }
            else{
                throw std::runtime_error("Unexpected contained type encountered"); 
            }
        }
   }

    // Make and 
   void makeFromHeap(std::string name, BlsType object){
        if (std::holds_alternative<std::shared_ptr<HeapDescriptor>>(object)) {
            auto heapObj = std::get<std::shared_ptr<HeapDescriptor>>(object);
            if(auto mapDesc = std::dynamic_pointer_cast<MapDescriptor>(heapObj)){
                Descriptor desc; 
                desc.descType = TYPE::map_t;
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
                        case(TYPE::int_t) : {
                            std::vector<int64_t> intVector; 
                            writeToPrimVector(intVector, vecDesc); 
                            this->createField(name, intVector);    
                            break; 
                        }
                        case(TYPE::float_t) : {
                            std::vector<double> floatVector; 
                            writeToPrimVector(floatVector, vecDesc); 
                            this->createField(name, floatVector); 
                            break; 
                        }
                        case(TYPE::string_t) : {
                            std::vector<std::string> strVector; 
                            writeToPrimVector(strVector, vecDesc); 
                            this->createField(name, strVector); 
                            break; 
                        }              
                        default : {
                            Descriptor desc; 
                            desc.descType = TYPE::list_t;
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
        }
        else {
            // Prim Desc should only be used for standalone vector elements; 
            if(std::holds_alternative<int64_t>(object)){
                // Should be fine
                int64_t jamar = std::get<int64_t>(object); 
                this->createField(name, jamar); 
                
            }
            else if(std::holds_alternative<double>(object)){
                double jamar = std::get<double>(object); 
                this->createField(name, jamar); 

            }
            else if(std::holds_alternative<bool>(object)){
                bool jamar = std::get<bool>(object); 
                this->createField(name, jamar); 
            }
            else if(std::holds_alternative<std::string>(object)){
                std::string jamar = std::get<std::string>(object); 
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


   // Utility function to get the volatility of a field
    void getFieldVolatility(std::unordered_map<std::string, std::deque<float>>  &vol_list, int vol_field_size){
        for(auto &obj : this->attributeMap){    
            int desc = obj.second; 

            TYPE type = this->Descriptors.at(desc).descType; 

            // If the value is of numeric type; 
            if(type == TYPE::float_t){
                double carrier;
                int trav_dist;
                this->deserialize(desc ,carrier, trav_dist);
                if (!vol_list.contains(obj.first)) {
                    vol_list.try_emplace(obj.first);
                }
                vol_list.at(obj.first).push_back(carrier);
                
            }
            else if(type == TYPE::int_t){
                int64_t carrier;
                int trav_dist;
                this->deserialize(desc, carrier, trav_dist);
                if (!vol_list.contains(obj.first)) {
                    vol_list.try_emplace(obj.first);
                }
                vol_list.at(obj.first).push_back(carrier);
            }
        }
   }

}; 