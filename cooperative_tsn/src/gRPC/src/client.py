#
# Copyright 2024 Jannusch Bigge <jannusch.bigge@tu-dresden.de>
#
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

from __future__ import print_function

import logging

import json
import argparse

import grpc
import scheduler_pb2
import scheduler_pb2_grpc

def run(input_dir: str, output_dir: str, url: str):
    network_input = {}
    print("Try reading input file...")
    with open(input_dir) as f:
        network_input = json.load(f)

    print("Will try to connect to scheduler...")
    with grpc.insecure_channel(url) as channel:
        stub = scheduler_pb2_grpc.SchedulerStub(channel)
        response = stub.CreateSchedule(scheduler_pb2.ScheduleInput(input = json.dumps(network_input)))
    print(f"Scheuduler answered")
    with open(output_dir, 'w') as f:
        f.write(response.output)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="gRPC client written in Python to connect OMNeT++/INET with TSNsched")
    parser.add_argument("--input", type=str, help="Location to input file")
    parser.add_argument("--output", type=str, help="Location to output file")
    parser.add_argument("--url", type=str, help="URL of the scheduler (including the port)")
    args = parser.parse_args()
    output_dir = "files/output.json"
    input_dir = "files/input.json"
    url = "localhost:50051"
    if args.input:
        input_dir = args.input
    if args.output:
        output_dir = args.output
    if args.url:
        url = args.url
    
    logging.basicConfig()
    run(input_dir, output_dir, url)
