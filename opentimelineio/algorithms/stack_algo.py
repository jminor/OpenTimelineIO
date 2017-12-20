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

__doc__ = """ Algorithms for stack objects. """

import copy

from .. import (
    exceptions,
    schema
)
from . import (
    track_algo
)

def flatten_stack(in_stack):
    """ Flatten a Stack into a single Track.
    """

    if not isinstance(in_stack, schema.Stack):
        raise ValueError("Input to flatten_stack must be a Stack")

    flat_track = schema.Track()
    flat_track.name = "Flattened"

    def _get_next_item(in_stack, track_index=0, trim_range=None):
        if track_index < len(in_stack):
            track = in_stack[track_index]
            if trim_range is not None:
                track = track_algo.track_trimmed_to_range(track, trim_range)
            for item in track:
                if item.visible():
                    yield item
                else:
                    # TODO: in the range...
                    for more in _get_next_item(
                        in_stack,
                        track_index+1,
                        item.range_in_parent()
                    ):
                        yield more

    for item in _get_next_item(in_stack):
        #print("ITEM: {}".format(item))
        flat_track.append(copy.deepcopy(item))

    return flat_track