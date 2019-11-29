# -*- coding: utf-8 -*-
# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: remote_attestation.proto

import sys
_b=sys.version_info[0]<3 and (lambda x:x) or (lambda x:x.encode('latin1'))
from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from google.protobuf import reflection as _reflection
from google.protobuf import symbol_database as _symbol_database
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()




DESCRIPTOR = _descriptor.FileDescriptor(
  name='remote_attestation.proto',
  package='remote_attestation',
  syntax='proto3',
  serialized_options=None,
  serialized_pb=_b('\n\x18remote_attestation.proto\x12\x12remote_attestation\"\x18\n\x06Status\x12\x0e\n\x06status\x18\x01 \x01(\x05\"^\n\x06Report\x12\x0f\n\x07pem_key\x18\x01 \x01(\x04\x12\x10\n\x08key_size\x18\x02 \x01(\r\x12\x15\n\rremote_report\x18\x03 \x01(\x04\x12\x1a\n\x12remote_report_size\x18\x04 \x01(\r\"Y\n\x0c\x44\x61taMetadata\x12\x13\n\x0b\x65nc_sym_key\x18\x01 \x01(\x04\x12\x1b\n\x13training_data_fname\x18\x02 \x01(\t\x12\x17\n\x0ftest_data_fname\x18\x03 \x01(\t2\xaa\x01\n\x11RemoteAttestation\x12J\n\x0eGetAttestation\x12\x1a.remote_attestation.Status\x1a\x1a.remote_attestation.Report\"\x00\x12I\n\x07SendKey\x12 .remote_attestation.DataMetadata\x1a\x1a.remote_attestation.Status\"\x00\x62\x06proto3')
)




_STATUS = _descriptor.Descriptor(
  name='Status',
  full_name='remote_attestation.Status',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='status', full_name='remote_attestation.Status.status', index=0,
      number=1, type=5, cpp_type=1, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=48,
  serialized_end=72,
)


_REPORT = _descriptor.Descriptor(
  name='Report',
  full_name='remote_attestation.Report',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='pem_key', full_name='remote_attestation.Report.pem_key', index=0,
      number=1, type=4, cpp_type=4, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='key_size', full_name='remote_attestation.Report.key_size', index=1,
      number=2, type=13, cpp_type=3, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='remote_report', full_name='remote_attestation.Report.remote_report', index=2,
      number=3, type=4, cpp_type=4, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='remote_report_size', full_name='remote_attestation.Report.remote_report_size', index=3,
      number=4, type=13, cpp_type=3, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=74,
  serialized_end=168,
)


_DATAMETADATA = _descriptor.Descriptor(
  name='DataMetadata',
  full_name='remote_attestation.DataMetadata',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='enc_sym_key', full_name='remote_attestation.DataMetadata.enc_sym_key', index=0,
      number=1, type=4, cpp_type=4, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='training_data_fname', full_name='remote_attestation.DataMetadata.training_data_fname', index=1,
      number=2, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=_b("").decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='test_data_fname', full_name='remote_attestation.DataMetadata.test_data_fname', index=2,
      number=3, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=_b("").decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=170,
  serialized_end=259,
)

DESCRIPTOR.message_types_by_name['Status'] = _STATUS
DESCRIPTOR.message_types_by_name['Report'] = _REPORT
DESCRIPTOR.message_types_by_name['DataMetadata'] = _DATAMETADATA
_sym_db.RegisterFileDescriptor(DESCRIPTOR)

Status = _reflection.GeneratedProtocolMessageType('Status', (_message.Message,), {
  'DESCRIPTOR' : _STATUS,
  '__module__' : 'remote_attestation_pb2'
  # @@protoc_insertion_point(class_scope:remote_attestation.Status)
  })
_sym_db.RegisterMessage(Status)

Report = _reflection.GeneratedProtocolMessageType('Report', (_message.Message,), {
  'DESCRIPTOR' : _REPORT,
  '__module__' : 'remote_attestation_pb2'
  # @@protoc_insertion_point(class_scope:remote_attestation.Report)
  })
_sym_db.RegisterMessage(Report)

DataMetadata = _reflection.GeneratedProtocolMessageType('DataMetadata', (_message.Message,), {
  'DESCRIPTOR' : _DATAMETADATA,
  '__module__' : 'remote_attestation_pb2'
  # @@protoc_insertion_point(class_scope:remote_attestation.DataMetadata)
  })
_sym_db.RegisterMessage(DataMetadata)



_REMOTEATTESTATION = _descriptor.ServiceDescriptor(
  name='RemoteAttestation',
  full_name='remote_attestation.RemoteAttestation',
  file=DESCRIPTOR,
  index=0,
  serialized_options=None,
  serialized_start=262,
  serialized_end=432,
  methods=[
  _descriptor.MethodDescriptor(
    name='GetAttestation',
    full_name='remote_attestation.RemoteAttestation.GetAttestation',
    index=0,
    containing_service=None,
    input_type=_STATUS,
    output_type=_REPORT,
    serialized_options=None,
  ),
  _descriptor.MethodDescriptor(
    name='SendKey',
    full_name='remote_attestation.RemoteAttestation.SendKey',
    index=1,
    containing_service=None,
    input_type=_DATAMETADATA,
    output_type=_STATUS,
    serialized_options=None,
  ),
])
_sym_db.RegisterServiceDescriptor(_REMOTEATTESTATION)

DESCRIPTOR.services_by_name['RemoteAttestation'] = _REMOTEATTESTATION

# @@protoc_insertion_point(module_scope)
