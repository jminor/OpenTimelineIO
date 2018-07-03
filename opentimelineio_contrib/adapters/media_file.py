#
# Copyright 2018 Pixar Animation Studios
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

__doc__ = """ Media file read-only adapter. This adapter just wraps a video
or audio file in a single-clip timeline with the correct duration. """


import os
import subprocess
import re
import json

import opentimelineio as otio


def get_metadata(filepath):
    proc = subprocess.Popen(
        [
            'ffprobe',
            '-v', 'quiet',
            '-print_format', 'json',
            '-show_format',
            '-show_streams',
            filepath
        ],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        stdin=subprocess.PIPE,
        env=os.environ
    )
    out, err = proc.communicate()
    metadata = {}

    info = json.loads(out)
    format = info.get('format', {})
    streams = info.get('streams', [])
    time_base = None

    metadata['ffprobe'] = info

    # try to get duration and start_time from the top-level first
    if format.get('duration'):
        metadata['duration'] = format.get('duration')

    if format.get('start_time'):
        metadata['start'] = format.get('start_time')

    # now go looking in the streams to get more info
    for stream in streams:
        codec_type = stream.get('codec_type')
        if codec_type == 'video':
            metadata['has_video'] = True

            # trust this over anything from non-video tracks
            if stream.get('time_base'):
                time_base = stream.get('time_base')

            # trust this over anything from non-video tracks
            timecode = stream.get('tags', {}).get('timecode')
            if timecode:
                metadata['timecode'] = timecode

        elif codec_type == 'audio':
            metadata['has_audio'] = True

            # use this only if we didn't get info from other tracks
            if not time_base and stream.get('time_base'):
                time_base = stream.get('time_base')

            # use this only if we didn't get info from other tracks
            timecode = stream.get('tags', {}).get('timecode')
            if not metadata.get('timecode') and timecode:
                metadata['timecode'] = timecode

        else:
            # don't pull metadata from non-audio/video tracks
            pass

    if time_base:
        numerator, denominator = time_base.split('/')
        if numerator == '1':
            metadata['fps'] = int(denominator)
        else:
            metadata['fps'] = float(denominator) / float(numerator)

    return metadata


def guess_media_range(metadata, default_fps):
    start = otio.opentime.RationalTime()
    duration = otio.opentime.RationalTime()
    fps = metadata.get("fps", default_fps)
    if "duration" in metadata:
        duration = otio.opentime.from_seconds(float(metadata["duration"]))
    if "start" in metadata:
        # try this first
        start = otio.opentime.from_seconds(float(metadata["start"]))
    if "timecode" in metadata:
        # overwrite start with timecode if we have it
        start = otio.opentime.from_timecode(metadata["timecode"], fps)
    return otio.opentime.TimeRange(
        start_time=start.rescaled_to(fps),
        duration=duration.rescaled_to(fps)
    )


def read_from_file(filepath, default_fps=24):

    if not os.path.exists(filepath):
        raise otio.exceptions.CouldNotReadFileError(
            "File not found: {}".format(filepath)
        )

    timeline = otio.schema.Timeline()
    timeline.name = os.path.splitext(os.path.basename(filepath))[0]
    clip = otio.schema.Clip()
    clip.name = os.path.basename(filepath)
    clip.media_reference = otio.schema.ExternalReference(
        target_url=filepath,
    )
    metadata = get_metadata(filepath)
    clip.media_reference.metadata["ffprobe"] = metadata
    clip.media_reference.available_range = guess_media_range(
        metadata, default_fps
    )
    track = otio.schema.Track()
    if metadata.get('has_video'):
        track.kind = otio.schema.TrackKind.Video
    elif metadata.get('has_audio'):
        track.kind = otio.schema.TrackKind.Audio
    else:
        track.kind = None
    track.append(clip)
    timeline.tracks.append(track)
    return timeline
