//
// Created by dewe on 9/12/24.
//

#pragma once
#include "../metadata_options.h"


namespace stratifyx::metadata::dlb {
    CREATE_ENUM(NodeConnectionType, OneToOne, ManyToOne, OneToMany, ManyToMany)

    struct DLBMetaData {
        std::string id;
        std::string name;
        NodeConnectionType nodeType;
        MetaDataArgsList args;
        bool isArgsList{false};
        std::string desc;

        rapidjson::Value ToJson(rapidjson::MemoryPoolAllocator<>& allocator) const;
    };
}
