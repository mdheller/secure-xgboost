# Copyright 2015 gRPC authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""The Python implementation of the GRPC RemoteAttestation server."""

from concurrent import futures
import logging

import grpc

import remote_attestation_pb2
import remote_attestation_pb2_grpc


class RemoteAttestationServicer(remote_attestation_pb2_grpc.RemoteAttestationServicer):

    def GetAttestation(self, request, context):
        """
        Calls get_remote_report_with_public_key()
        """
        return remote_attestation_pb2.Report(pem_key=1, key_size=2, remote_report=3, remote_report_size=4)

    def SendKey(self, request, context):
        """
        Sends symmetric key and filenames of data
        """
        return remote_attestation_pb2.Status(status=1)


def serve():
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
    remote_attestation_pb2_grpc.add_RemoteAttestationServicer_to_server(RemoteAttestationServicer(), server)
    server.add_insecure_port('[::]:50051')
    server.start()
    server.wait_for_termination()


if __name__ == '__main__':
    logging.basicConfig()
    serve()
