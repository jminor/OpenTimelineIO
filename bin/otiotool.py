#!/usr/bin/env python2.7
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

__doc__ = """ Multi-purpose command line utility for working with
          OpenTimelineIO. """

import os
import sys
import argparse
import subprocess
import tempfile
try:
    from urllib2 import urlopen
except ImportError:
    from urllib.request import urlopen
import difflib
import opentimelineio as otio

BIN_DIR = os.path.dirname(__file__)


def _parsed_args():
    """ parse commandline arguments with argparse """

    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    parser.add_argument(
        'inputs',
        metavar='FILE',
        type=str,
        nargs='+',
        help='input file(s)'
    )
    parser.add_argument(
        '--cat',
        action='store_true',
        help='print the result to stdout (output format is determined by '
             '--output_format)'
    )
    parser.add_argument(
        '--stack',
        action='store_true',
        help='stack all inputs into tracks of a single timeline'
    )
    parser.add_argument(
        '--removetransitions',
        action='store_true',
        help='remove all transitions from the timeline'
    )
    parser.add_argument(
        '--flatten',
        action='store_true',
        help='flatten all tracks of the timeline into one'
    )
    parser.add_argument(
        '--copymedia',
        help='copy all referenced media to the specified folder and relink'
             ' to that media'
    )
    parser.add_argument(
        '--out',
        type=str,
        help='write to this file (output format is determined by file'
             ' extension)'
    )
    # parser.add_argument(
    #     '--relink',
    #     action='store_true',
    #     help='attempt to relink clips with missing media'
    # )
    parser.add_argument(
        '--view',
        action='store_true',
        help='view the timeline in a GUI'
    )
    parser.add_argument(
        '--media',
        action='store_true',
        help='print a list of all the media referenced by the timeline'
    )
    parser.add_argument(
        '--clips',
        action='store_true',
        help='print a list of all the clip names in the timeline'
    )
    parser.add_argument(
        '--output_format',
        type=str,
        help='format to output with --cat',
        default='otio_json'
    )
    parser.add_argument(
        '--diff',
        action='store_true',
        help='compare two input timelines'
    )
    parser.add_argument(
        '-a',
        '--adapter_arg',
        nargs='+',
        default=[],
        action='append',
        type=str,
        help='specify an adapter-specific option as key=value (for example -a rate=30)'
    )
    parser.add_argument(
        '--unlink',
        action='store_true',
        help='unlink from existing media'
    )
    parser.add_argument(
        '--media_linker',
        type=str,
        help='relink any missing media references with the named media linker'
             ' (use "__default" for default)'
    )

    return parser.parse_args()


def copy_media(url, folder):
    if not os.path.exists(folder):
        os.mkdir(folder)
    filename = os.path.basename(url)
    # @TODO: This is prone to name collisions if the basename is not unique
    # We probably need to hash the url, or turn the whole url into a filename.
    localpath = os.path.join(folder, filename)
    if os.path.exists(localpath):
        return localpath
    try:
        if url.startswith("/"):
            print("COPYING: {}".format(url))
            data = open(url, "rb").read()
        else:
            print("DOWNLOADING: {}".format(url))
            data = urlopen(url).read()
        open(localpath, "wb").write(data)
    except Exception as ex:
        print("WARNING: Failed to copy {}".format(url))
        print(ex)
    return localpath


def main():
    """Parse arguments and call _cat_otio_file."""
    args = _parsed_args()

    # Read all the inputs

    timelines = []
    adapter_args = {}
    for arg in args.adapter_arg:
        key, value = arg[0].split('=', 1)
        adapter_args[key] = value
    for input_path in args.inputs:
        timeline = otio.adapters.read_from_file(input_path, **adapter_args)
        timelines.append(timeline)

    # Combine them

    if args.stack:
        newtimeline = otio.schema.Timeline()
        newtimeline.name = "Stacked Timelines"
        newtimeline.tracks.name = newtimeline.name

        for timeline in timelines:
            if len(timeline.tracks) == 1:
                # push the timeline name down into the track name so we know
                # which track came from which file (but only if there's one).
                timeline.tracks[0].name = timeline.name
            newtimeline.tracks.extend(timeline.tracks)

        timelines = [newtimeline]

    # Modify them

    if args.removetransitions:
        for timeline in timelines:
            timeline.tracks = otio.algorithms.composition_without_transitions(
                timeline.tracks
            )

    if args.flatten:
        for timeline in timelines:
            track = otio.algorithms.flatten_stack(timeline.tracks)
            del timeline.tracks[:]
            timeline.tracks.append(track)

    if len(timelines) == 0:
        print("ERROR: No timeline(s) specified.")
        sys.exit(1)

    # If there was more than one, then put them into a collection

    if len(timelines) > 1:
        result = otio.schema.SerializableCollection()
        result.extend(timelines)
    else:
        result = timelines[0]

    if args.unlink:
        for clip in result.each_clip():
            if clip.media_reference:
                clip.media_reference = None

    if args.media_linker:
        media_linker_argument_map = {}
        for clip in result.each_clip():
            new_mr = otio.media_linker.linked_media_reference(
                clip,
                args.media_linker,
                media_linker_argument_map
            )
            if new_mr is not None:
                clip.media_reference = new_mr

    if args.copymedia:
        for clip in result.each_clip():
            url = None
            try:
                url = clip.media_reference.target_url
            except AttributeError:
                pass
            if url:
                path = copy_media(url, args.copymedia)
                clip.media_reference.target_url = path

    # Now we can do all of our output, reporting, etc.

    if args.diff:
        if len(timelines) != 2:
            print("ERROR: --diff only works with 2 input timelines"
                  " (you have {})".format(len(timelines)))
            sys.exit(1)

        d = difflib.Differ()

        a = timelines[0]
        b = timelines[1]
        print("A:", a.name)
        print("B:", b.name)
        aclips = list(a.each_clip())
        bclips = list(b.each_clip())
        if len(aclips) != len(bclips):
            print("DIFF: A has %d clips != B has %d clips".format(
                len(aclips),
                len(bclips)
            ))
        else:
            print("SAME: Both A and B have %d clips" % len(aclips))
        amap = {}
        bmap = {}
        for clip in aclips:
            amap.setdefault(clip.name, [])
            amap[clip.name] += [clip]
        for clip in bclips:
            bmap.setdefault(clip.name, [])
            bmap[clip.name] += [clip]
        anames = [clip.name for clip in aclips]
        bnames = [clip.name for clip in bclips]
        anameset = set(anames)
        bnameset = set(bnames)
        both = anameset.intersection(bnameset)
        for name in anames:
            if name not in bnameset:
                print("ONLY IN A:", name)
                pass
        for name in bnames:
            if name not in anameset:
                print("ONLY IN B:", name)
        for name in both:
            ina = amap[name]
            inb = bmap[name]
            if len(ina) == 1 and len(inb) == 1:
                aclip = ina[0]
                bclip = inb[0]
                # remove stuff we don't care about
                aclip.metadata.get("ALE", {})["Modified Date"] = "IGNORE"
                bclip.metadata.get("ALE", {})["Modified Date"] = "IGNORE"
                astr = otio.adapters.write_to_string(aclip, 'otio_json')
                bstr = otio.adapters.write_to_string(bclip, 'otio_json')
                if astr != bstr:
                    result = list(d.compare(
                        astr.splitlines(True),
                        bstr.splitlines(True)
                    ))
                    print("DIFF:", name)
                    print("".join(result))
            else:
                print("DIFF: {} appears in A {} times".format(name, len(ina)))
                print("DIFF: {} appears in B {} times".format(name, len(inb)))

    if args.out:
        otio.adapters.write_to_file(result, args.out)

    if args.cat:
        output = otio.adapters.write_to_string(result, args.output_format)
        print(output)

    if args.clips:
        for clip in result.each_clip():
            print(clip.name)

    if args.media:
        urls = []
        for clip in result.each_clip():
            url = None
            try:
                url = clip.media_reference.target_url
            except AttributeError:
                pass
            if url and url not in urls:
                urls.append(url)
        if len(urls) > 0:
            print("\n".join(urls))
        else:
            print("No media references found.")

    if args.view:
        folder = tempfile.mkdtemp()
        for timeline in timelines:
            path = os.path.join(folder, timeline.name or "Untitled") + ".otio"
            otio.adapters.write_to_file(timeline, path)
            output = subprocess.check_output(
                [os.path.join(BIN_DIR, "otioview.py"), path],
                stderr=subprocess.STDOUT
            )
            print(output)


if __name__ == '__main__':
    main()
