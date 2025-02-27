#pragma once
#include "tool_configure.h"
#include "EASTL/vector.h"
#include "platform/guid.h"
#include "resource/resource_header.h"
#include "platform/configure.h"
#include "simdjson.h"
#include "asset/cooker.hpp"
#include "utils/hashmap.hpp"

namespace skr::io
{
class RAMService;
}

namespace skd sreflect
{
using namespace skr;
namespace asset sreflect
{
struct sreflect sattr(
"guid" : "76044661-E2C9-43A7-A4DE-AEDD8FB5C847", 
"serialize" : "json"
)
TOOL_API SImporter
{
    skr_guid_t assetGuid;
    SImporter(skr_guid_t inAsset)
        : assetGuid(inAsset)
    {
    }
    virtual ~SImporter() {}
    virtual void* Import(skr::io::RAMService*, const SAssetRecord* record) = 0;
};
struct SImporterRegistry {
    SImporter* LoadImporter(const SAssetRecord* record, simdjson::ondemand::value&& object);
    skr::flat_hash_map<skr_guid_t, SImporter* (*)(const SAssetRecord* record, simdjson::ondemand::value&& object), skr::guid::hash> loaders;
};
TOOL_API SImporterRegistry* GetImporterRegistry();
struct SImporterFactory {
    virtual bool CanImport(const SAssetRecord* record) = 0;
    virtual skr_guid_t GetResourceType() = 0;
    virtual void CreateImporter(const SAssetRecord* record) = 0;
};
} // namespace sreflect
} // namespace sreflect