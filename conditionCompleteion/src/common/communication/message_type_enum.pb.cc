// Generated by the protocol buffer compiler.  DO NOT EDIT!

#define INTERNAL_SUPPRESS_PROTOBUF_FIELD_DEPRECATION
#include "communication/message_type_enum.pb.h"

#include <algorithm>

#include <google/protobuf/stubs/once.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)

namespace messageTypeEnum {

namespace {

const ::google::protobuf::Descriptor* typeEnums_descriptor_ = NULL;
const ::google::protobuf::internal::GeneratedMessageReflection*
  typeEnums_reflection_ = NULL;
const ::google::protobuf::EnumDescriptor* typeEnums_mapping_descriptor_ = NULL;

}  // namespace


void protobuf_AssignDesc_message_5ftype_5fenum_2eproto() {
  protobuf_AddDesc_message_5ftype_5fenum_2eproto();
  const ::google::protobuf::FileDescriptor* file =
    ::google::protobuf::DescriptorPool::generated_pool()->FindFileByName(
      "message_type_enum.proto");
  GOOGLE_CHECK(file != NULL);
  typeEnums_descriptor_ = file->message_type(0);
  static const int typeEnums_offsets_[1] = {
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(typeEnums, type_),
  };
  typeEnums_reflection_ =
    new ::google::protobuf::internal::GeneratedMessageReflection(
      typeEnums_descriptor_,
      typeEnums::default_instance_,
      typeEnums_offsets_,
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(typeEnums, _has_bits_[0]),
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(typeEnums, _unknown_fields_),
      -1,
      ::google::protobuf::DescriptorPool::generated_pool(),
      ::google::protobuf::MessageFactory::generated_factory(),
      sizeof(typeEnums));
  typeEnums_mapping_descriptor_ = typeEnums_descriptor_->enum_type(0);
}

namespace {

GOOGLE_PROTOBUF_DECLARE_ONCE(protobuf_AssignDescriptors_once_);
inline void protobuf_AssignDescriptorsOnce() {
  ::google::protobuf::GoogleOnceInit(&protobuf_AssignDescriptors_once_,
                 &protobuf_AssignDesc_message_5ftype_5fenum_2eproto);
}

void protobuf_RegisterTypes(const ::std::string&) {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedMessage(
    typeEnums_descriptor_, &typeEnums::default_instance());
}

}  // namespace

void protobuf_ShutdownFile_message_5ftype_5fenum_2eproto() {
  delete typeEnums::default_instance_;
  delete typeEnums_reflection_;
}

void protobuf_AddDesc_message_5ftype_5fenum_2eproto() {
  static bool already_here = false;
  if (already_here) return;
  already_here = true;
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  ::google::protobuf::DescriptorPool::InternalAddGeneratedFile(
    "\n\027message_type_enum.proto\022\017messageTypeEn"
    "um\"\233\013\n\ttypeEnums\0220\n\004type\030\001 \001(\0162\".message"
    "TypeEnum.typeEnums.mapping\"\333\n\n\007mapping\022\016"
    "\n\nHEART_BEAT\020\001\022\022\n\016HEART_BEAT_ACK\020\002\022\030\n\024OS"
    "D_START_MONITORING\020\003\022\034\n\030OSD_START_MONITO"
    "RING_ACK\020\004\022\036\n\032RECV_PROC_START_MONITORING"
    "\020\005\022\"\n\036RECV_PROC_START_MONITORING_ACK\020\006\022\027"
    "\n\023LL_START_MONITORING\020\007\022\033\n\027LL_START_MONI"
    "TORING_ACK\020\010\022\022\n\016GET_GLOBAL_MAP\020\t\022\023\n\017GLOB"
    "AL_MAP_INFO\020\n\022\026\n\022COMP_TRANSFER_INFO\020\013\022\032\n"
    "\026COMP_TRANSFER_INFO_ACK\020\014\022\034\n\030COMP_TRANSF"
    "ER_FINAL_STAT\020\r\022 \n\034COMP_TRANSFER_FINAL_S"
    "TAT_ACK\020\016\022\031\n\025GET_SERVICE_COMPONENT\020\017\022\035\n\031"
    "GET_SERVICE_COMPONENT_ACK\020\020\022\026\n\022TRANSFER_"
    "COMPONENT\020\021\022\037\n\033TRANSFER_COMPONENT_RESPON"
    "SE\020\022\022\026\n\022GET_OBJECT_VERSION\020\023\022\032\n\026GET_OBJE"
    "CT_VERSION_ACK\020\024\022\025\n\021NODE_ADDITION_CLI\0202\022"
    "\031\n\025NODE_ADDITION_CLI_ACK\0203\022\024\n\020NODE_ADDIT"
    "ION_GL\0204\022\030\n\024NODE_ADDITION_GL_ACK\0205\022\017\n\013NO"
    "DE_RETIRE\0206\022\023\n\017NODE_RETIRE_ACK\0207\022\030\n\024NODE"
    "_SYSTEM_STOP_CLI\0208\022\025\n\021LOCAL_NODE_STATUS\020"
    "9\022\031\n\025LOCAL_NODE_STATUS_ACK\020:\022\017\n\013NODE_STA"
    "TUS\020;\022\023\n\017NODE_STATUS_ACK\020<\022\021\n\rNODE_STOP_"
    "CLI\020=\022\025\n\021NODE_STOP_CLI_ACK\020>\022\021\n\rSTOP_SER"
    "VICES\020\?\022\025\n\021STOP_SERVICES_ACK\020@\022\021\n\rNODE_F"
    "AILOVER\020A\022\025\n\021NODE_FAILOVER_ACK\020B\022\025\n\021TAKE"
    "_GL_OWNERSHIP\020C\022\031\n\025TAKE_GL_OWNERSHIP_ACK"
    "\020D\022\026\n\022BLOCK_NEW_REQUESTS\020E\022\032\n\026BLOCK_NEW_"
    "REQUESTS_ACK\020F\022\021\n\rNODE_DELETION\020G\022\025\n\021NOD"
    "E_DELETION_ACK\020H\022\036\n\032NODE_REJOIN_AFTER_RE"
    "COVERY\020I\022\"\n\036NODE_REJOIN_AFTER_RECOVERY_A"
    "CK\020J\022\026\n\022GET_CLUSTER_STATUS\020K\022\032\n\026GET_CLUS"
    "TER_STATUS_ACK\020L\022\020\n\014NODE_STOP_LL\020M\022\024\n\020NO"
    "DE_STOP_LL_ACK\020N\022\024\n\020CONTAINER_UPDATE\020O\022\034"
    "\n\030RELEASE_TRANSACTION_LOCK\020P\022\016\n\nSTATUS_A"
    "CK\020Q\022\033\n\027NODE_ADDITION_FINAL_ACK\020R\022\035\n\031INI"
    "TIATE_CLUSTER_RECOVERY\020S\022\016\n\tERROR_MSG\020\377\001", 1480);
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedFile(
    "message_type_enum.proto", &protobuf_RegisterTypes);
  typeEnums::default_instance_ = new typeEnums();
  typeEnums::default_instance_->InitAsDefaultInstance();
  ::google::protobuf::internal::OnShutdown(&protobuf_ShutdownFile_message_5ftype_5fenum_2eproto);
}

// Force AddDescriptors() to be called at static initialization time.
struct StaticDescriptorInitializer_message_5ftype_5fenum_2eproto {
  StaticDescriptorInitializer_message_5ftype_5fenum_2eproto() {
    protobuf_AddDesc_message_5ftype_5fenum_2eproto();
  }
} static_descriptor_initializer_message_5ftype_5fenum_2eproto_;


// ===================================================================

const ::google::protobuf::EnumDescriptor* typeEnums_mapping_descriptor() {
  protobuf_AssignDescriptorsOnce();
  return typeEnums_mapping_descriptor_;
}
bool typeEnums_mapping_IsValid(int value) {
  switch(value) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 50:
    case 51:
    case 52:
    case 53:
    case 54:
    case 55:
    case 56:
    case 57:
    case 58:
    case 59:
    case 60:
    case 61:
    case 62:
    case 63:
    case 64:
    case 65:
    case 66:
    case 67:
    case 68:
    case 69:
    case 70:
    case 71:
    case 72:
    case 73:
    case 74:
    case 75:
    case 76:
    case 77:
    case 78:
    case 79:
    case 80:
    case 81:
    case 82:
    case 83:
    case 255:
      return true;
    default:
      return false;
  }
}

#ifndef _MSC_VER
const typeEnums_mapping typeEnums::HEART_BEAT;
const typeEnums_mapping typeEnums::HEART_BEAT_ACK;
const typeEnums_mapping typeEnums::OSD_START_MONITORING;
const typeEnums_mapping typeEnums::OSD_START_MONITORING_ACK;
const typeEnums_mapping typeEnums::RECV_PROC_START_MONITORING;
const typeEnums_mapping typeEnums::RECV_PROC_START_MONITORING_ACK;
const typeEnums_mapping typeEnums::LL_START_MONITORING;
const typeEnums_mapping typeEnums::LL_START_MONITORING_ACK;
const typeEnums_mapping typeEnums::GET_GLOBAL_MAP;
const typeEnums_mapping typeEnums::GLOBAL_MAP_INFO;
const typeEnums_mapping typeEnums::COMP_TRANSFER_INFO;
const typeEnums_mapping typeEnums::COMP_TRANSFER_INFO_ACK;
const typeEnums_mapping typeEnums::COMP_TRANSFER_FINAL_STAT;
const typeEnums_mapping typeEnums::COMP_TRANSFER_FINAL_STAT_ACK;
const typeEnums_mapping typeEnums::GET_SERVICE_COMPONENT;
const typeEnums_mapping typeEnums::GET_SERVICE_COMPONENT_ACK;
const typeEnums_mapping typeEnums::TRANSFER_COMPONENT;
const typeEnums_mapping typeEnums::TRANSFER_COMPONENT_RESPONSE;
const typeEnums_mapping typeEnums::GET_OBJECT_VERSION;
const typeEnums_mapping typeEnums::GET_OBJECT_VERSION_ACK;
const typeEnums_mapping typeEnums::NODE_ADDITION_CLI;
const typeEnums_mapping typeEnums::NODE_ADDITION_CLI_ACK;
const typeEnums_mapping typeEnums::NODE_ADDITION_GL;
const typeEnums_mapping typeEnums::NODE_ADDITION_GL_ACK;
const typeEnums_mapping typeEnums::NODE_RETIRE;
const typeEnums_mapping typeEnums::NODE_RETIRE_ACK;
const typeEnums_mapping typeEnums::NODE_SYSTEM_STOP_CLI;
const typeEnums_mapping typeEnums::LOCAL_NODE_STATUS;
const typeEnums_mapping typeEnums::LOCAL_NODE_STATUS_ACK;
const typeEnums_mapping typeEnums::NODE_STATUS;
const typeEnums_mapping typeEnums::NODE_STATUS_ACK;
const typeEnums_mapping typeEnums::NODE_STOP_CLI;
const typeEnums_mapping typeEnums::NODE_STOP_CLI_ACK;
const typeEnums_mapping typeEnums::STOP_SERVICES;
const typeEnums_mapping typeEnums::STOP_SERVICES_ACK;
const typeEnums_mapping typeEnums::NODE_FAILOVER;
const typeEnums_mapping typeEnums::NODE_FAILOVER_ACK;
const typeEnums_mapping typeEnums::TAKE_GL_OWNERSHIP;
const typeEnums_mapping typeEnums::TAKE_GL_OWNERSHIP_ACK;
const typeEnums_mapping typeEnums::BLOCK_NEW_REQUESTS;
const typeEnums_mapping typeEnums::BLOCK_NEW_REQUESTS_ACK;
const typeEnums_mapping typeEnums::NODE_DELETION;
const typeEnums_mapping typeEnums::NODE_DELETION_ACK;
const typeEnums_mapping typeEnums::NODE_REJOIN_AFTER_RECOVERY;
const typeEnums_mapping typeEnums::NODE_REJOIN_AFTER_RECOVERY_ACK;
const typeEnums_mapping typeEnums::GET_CLUSTER_STATUS;
const typeEnums_mapping typeEnums::GET_CLUSTER_STATUS_ACK;
const typeEnums_mapping typeEnums::NODE_STOP_LL;
const typeEnums_mapping typeEnums::NODE_STOP_LL_ACK;
const typeEnums_mapping typeEnums::CONTAINER_UPDATE;
const typeEnums_mapping typeEnums::RELEASE_TRANSACTION_LOCK;
const typeEnums_mapping typeEnums::STATUS_ACK;
const typeEnums_mapping typeEnums::NODE_ADDITION_FINAL_ACK;
const typeEnums_mapping typeEnums::INITIATE_CLUSTER_RECOVERY;
const typeEnums_mapping typeEnums::ERROR_MSG;
const typeEnums_mapping typeEnums::mapping_MIN;
const typeEnums_mapping typeEnums::mapping_MAX;
const int typeEnums::mapping_ARRAYSIZE;
#endif  // _MSC_VER
#ifndef _MSC_VER
const int typeEnums::kTypeFieldNumber;
#endif  // !_MSC_VER

typeEnums::typeEnums()
  : ::google::protobuf::Message() {
  SharedCtor();
}

void typeEnums::InitAsDefaultInstance() {
}

typeEnums::typeEnums(const typeEnums& from)
  : ::google::protobuf::Message() {
  SharedCtor();
  MergeFrom(from);
}

void typeEnums::SharedCtor() {
  _cached_size_ = 0;
  type_ = 1;
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

typeEnums::~typeEnums() {
  SharedDtor();
}

void typeEnums::SharedDtor() {
  if (this != default_instance_) {
  }
}

void typeEnums::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const ::google::protobuf::Descriptor* typeEnums::descriptor() {
  protobuf_AssignDescriptorsOnce();
  return typeEnums_descriptor_;
}

const typeEnums& typeEnums::default_instance() {
  if (default_instance_ == NULL) protobuf_AddDesc_message_5ftype_5fenum_2eproto();  return *default_instance_;
}

typeEnums* typeEnums::default_instance_ = NULL;

typeEnums* typeEnums::New() const {
  return new typeEnums;
}

void typeEnums::Clear() {
  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    type_ = 1;
  }
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->Clear();
}

bool typeEnums::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) return false
  ::google::protobuf::uint32 tag;
  while ((tag = input->ReadTag()) != 0) {
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // optional .messageTypeEnum.typeEnums.mapping type = 1;
      case 1: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT) {
          int value;
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   int, ::google::protobuf::internal::WireFormatLite::TYPE_ENUM>(
                 input, &value)));
          if (::messageTypeEnum::typeEnums_mapping_IsValid(value)) {
            set_type(static_cast< ::messageTypeEnum::typeEnums_mapping >(value));
          } else {
            mutable_unknown_fields()->AddVarint(1, value);
          }
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectAtEnd()) return true;
        break;
      }
      
      default: {
      handle_uninterpreted:
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_END_GROUP) {
          return true;
        }
        DO_(::google::protobuf::internal::WireFormat::SkipField(
              input, tag, mutable_unknown_fields()));
        break;
      }
    }
  }
  return true;
#undef DO_
}

void typeEnums::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // optional .messageTypeEnum.typeEnums.mapping type = 1;
  if (has_type()) {
    ::google::protobuf::internal::WireFormatLite::WriteEnum(
      1, this->type(), output);
  }
  
  if (!unknown_fields().empty()) {
    ::google::protobuf::internal::WireFormat::SerializeUnknownFields(
        unknown_fields(), output);
  }
}

::google::protobuf::uint8* typeEnums::SerializeWithCachedSizesToArray(
    ::google::protobuf::uint8* target) const {
  // optional .messageTypeEnum.typeEnums.mapping type = 1;
  if (has_type()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteEnumToArray(
      1, this->type(), target);
  }
  
  if (!unknown_fields().empty()) {
    target = ::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        unknown_fields(), target);
  }
  return target;
}

int typeEnums::ByteSize() const {
  int total_size = 0;
  
  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    // optional .messageTypeEnum.typeEnums.mapping type = 1;
    if (has_type()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::EnumSize(this->type());
    }
    
  }
  if (!unknown_fields().empty()) {
    total_size +=
      ::google::protobuf::internal::WireFormat::ComputeUnknownFieldsSize(
        unknown_fields());
  }
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void typeEnums::MergeFrom(const ::google::protobuf::Message& from) {
  GOOGLE_CHECK_NE(&from, this);
  const typeEnums* source =
    ::google::protobuf::internal::dynamic_cast_if_available<const typeEnums*>(
      &from);
  if (source == NULL) {
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
    MergeFrom(*source);
  }
}

void typeEnums::MergeFrom(const typeEnums& from) {
  GOOGLE_CHECK_NE(&from, this);
  if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (from.has_type()) {
      set_type(from.type());
    }
  }
  mutable_unknown_fields()->MergeFrom(from.unknown_fields());
}

void typeEnums::CopyFrom(const ::google::protobuf::Message& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void typeEnums::CopyFrom(const typeEnums& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool typeEnums::IsInitialized() const {
  
  return true;
}

void typeEnums::Swap(typeEnums* other) {
  if (other != this) {
    std::swap(type_, other->type_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.Swap(&other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::google::protobuf::Metadata typeEnums::GetMetadata() const {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::Metadata metadata;
  metadata.descriptor = typeEnums_descriptor_;
  metadata.reflection = typeEnums_reflection_;
  return metadata;
}


// @@protoc_insertion_point(namespace_scope)

}  // namespace messageTypeEnum

// @@protoc_insertion_point(global_scope)
