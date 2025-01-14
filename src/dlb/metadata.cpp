//
// Created by dewe on 9/12/24.
//
#include "metadata.h"

namespace metadata::dlb
{
    rapidjson::Value DLBMetaData::ToJson(rapidjson::MemoryPoolAllocator<>& allocator) const
    {
        rapidjson::Value obj(rapidjson::kObjectType);
        obj.AddMember("id", rapidjson::Value(id.c_str(), allocator), allocator);
        obj.AddMember("name", rapidjson::Value(name.c_str(), allocator), allocator);

        // Assuming enum to string conversion for class_
        obj.AddMember("type", NodeConnectionTypeWrapper::toNumber(nodeType), allocator);

        rapidjson::Value args_array(rapidjson::kArrayType);
        for (const auto& arg : args)
        {
            args_array.PushBack(arg.ToJson(allocator), allocator);
        }
        obj.AddMember("args", args_array, allocator);

        obj.AddMember("desc", rapidjson::Value(desc.c_str(), allocator), allocator);

        obj.AddMember("is_args_list", isArgsList, allocator);
        return obj;
    }
} // namespace metadata::dlb