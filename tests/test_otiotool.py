#
# Copyright 2017 Pixar Animation Studios
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.
#

"""Tests for otiotool command line utility."""

# import opentimelineio as otio

import unittest
import subprocess
import os

OTIOTOOL_PATH = os.path.join(os.path.dirname(__file__), "../bin/otiotool.py")
SAMPLE_DATA_DIR = os.path.join(os.path.dirname(__file__), "sample_data")
CLIP_TEST = os.path.join(SAMPLE_DATA_DIR, "clip_example.otio")
MULTITRACK_EXAMPLE_PATH = os.path.join(SAMPLE_DATA_DIR, "multitrack.otio")
PREFLATTENED_EXAMPLE_PATH = os.path.join(SAMPLE_DATA_DIR, "preflattened.otio")


class TestOTIOTool(unittest.TestCase):

    def test_cat(self):
        proc = subprocess.Popen([
                OTIOTOOL_PATH,
                CLIP_TEST,
                "--cat"
            ],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
        expected = open(CLIP_TEST).read()
        stdout, stderr = proc.communicate()
        self.maxDiff = None
        self.assertMultiLineEqual("", stderr)
        self.assertMultiLineEqual(expected, stdout)

    def test_flatten(self):
        proc = subprocess.Popen([
                OTIOTOOL_PATH,
                MULTITRACK_EXAMPLE_PATH,
                "--flatten",
                "--cat"
            ],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
        expected = open(PREFLATTENED_EXAMPLE_PATH).read()
        stdout, stderr = proc.communicate()
        self.maxDiff = None
        self.assertMultiLineEqual("", stderr)
        self.assertMultiLineEqual(expected, stdout)


if __name__ == '__main__':
    unittest.main()
