#ifndef __CONFIG_H__
#define __CONFIG_H__
#include <stdint.h>

namespace accountInfoFile
{

static const uint16_t ACCOUNT_NAME_LENGTH = 256;
static const uint16_t CONTAINER_NAME_LENGTH = 256;
static const uint16_t TIMESTAMP_LENGTH = 16;
static const uint16_t HASH_LENGTH = 32;
static const uint16_t ID_LENGTH = 36;
static const uint16_t STATUS_LENGTH = 16;
static const uint16_t META_DATA_LENGTH = 30*1024; //30KB metadata( 24KB acl, 4KB metadata, 2KB system metadata)

}
#endif
