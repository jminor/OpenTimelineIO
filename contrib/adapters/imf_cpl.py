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

"""OpenTimelineIO Interoperable Master Format (IMF) Composition Playlist (CPL) Adapter"""

# See also the CPL spec here:
# SMPTE 2067-3:2016
# http://www.smpte-ra.org/schemas/2067-3/2016
# http://ieeexplore.ieee.org/document/7560854/

import uuid


def write_to_string(input_otio):

    header = input_otio.metadata.get("imf_cpl", {})

    # Let's take the easy road and just make the XML as a string.
    # We will build it up in parts as we go.

    output = """<?xml version="1.0" encoding="UTF-8" ?>
<CompositionPlaylist xmlns="http://www.smpte-ra.org/schemas/2067-3/2016" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
    <Id>urn:uuid:{my_uuid}</Id>
    <Annotation>{annotation}</Annotation>
    <IssueDate>{issue_date}</IssueDate>
    <Issuer>{issuer}</Issuer>
    <Creator>OpenTimelineIO IMF CPL Adapter</Creator>
    <ContentOriginator>{content_originator}</ContentOriginator>
    <ContentTitle>{content_title}</ContentTitle>
    <ContentKind>{content_kind}</ContentKind>
    <ContentVersionList>
        <ContentVersion>
            <Id>urn:uuid:{content_uuid}</Id>
            <LabelText>{label_text}</LabelText>
        </ContentVersion>
    </ContentVersionList>
""".format(
        my_uuid=uuid.uuid4(),
        content_uuid=uuid.uuid4(),
        annotation=header.get(
            "annotation",
            ""),
        issue_date="2017-04-13T23:09:36-00:00",  # TODO: Needs real date
        issuer=header.get(
            "issuer",
            "Unknown Issuer"),
        content_originator=header.get(
            "content_originator",
            "Unknown Content Originator"),
        content_title=input_otio.name,
        content_kind=header.get(
            "content_kind",
            "feature"),
        label_text=header.get(
            "label_text",
            "No Label")
        )
    # TODO: Are the default values above reasonable?

    # TODO: EssenceDescriptorList with
    # TODO: EssenceDescriptor stuff from MXF via XMLREG

    output += """
    <CompositionTimecode>
        <TimecodeDropFrame>0</TimecodeDropFrame>
        <TimecodeRate>24</TimecodeRate>
        <TimecodeStartAddress>00:00:00:00</TimecodeStartAddress>
    </CompositionTimecode>
    <EditRate>24000 1001</EditRate>
    <LocaleList>
        <Locale>
            <Annotation>???</Annotation>
            <LanguageList>
                <Language>en</Language>
            </LanguageList>
            <RegionList>
                <Region>001</Region>
            </RegionList>
            <ContentMaturityRatingList>
                <ContentMaturityRating>
                    <Agency>http://www.mpaa.org/2003-ratings</Agency>
                    <Rating>???</Rating>
                </ContentMaturityRating>
            </ContentMaturityRatingList>
        </Locale>
    </LocaleList>
    <ExtensionProperties>
        <cc:ApplicationIdentification xmlns:cc="http://www.smpte-ra.org/schemas/2067-2/2016">http://www.smpte-ra.org/schemas/2067-21/2016</cc:ApplicationIdentification>
    </ExtensionProperties>
"""
    # TODO: Replace values in the XML above...

    # We're assuming there's just one MainImageSequence.
    # TODO: Iterate over the timeline's tracks, emitting a sequence for
    # each one (video and audio).

    output += """
    <SegmentList>
        <Segment>
            <Id>urn:uuid:{segment_uuid}</Id>
            <SequenceList>
                <cc:MainImageSequence xmlns:cc="http://www.smpte-ra.org/schemas/2067-2/2016">
                    <Id>urn:uuid:{main_seq_uuid}</Id>
                    <TrackId>urn:uuid:{track_uuid}</TrackId>
                    <ResourceList>
""".format(
    segment_uuid=uuid.uuid4(),
    main_seq_uuid=uuid.uuid4(),
    track_uuid=uuid.uuid4()
)

    for clip in input_otio.each_clip():

        clip_metadata = clip.metadata.get("imf_cpl", {})

        # TODO: Use available_range of the MXF
        intrinsic_duration = clip.duration().value
        # TODO: Use source_range.start_time
        entry = 0
        # TODO: Use source_range.duration
        source_duration = clip.duration().value
        # TODO: Compute this from rate
        edit_rate = "24000 1001"

        output += """
                        <Resource xsi:type="TrackFileResourceType">
                            <Id>urn:uuid:{clip_uuid}</Id>
                            <EditRate>{edit_rate}</EditRate>
                            <Entry>{entry}</Entry>
                            <IntrinsicDuration>{intrinsic_duration}</IntrinsicDuration>
                            <SourceDuration>{source_duration}</SourceDuration>
                            <SourceEncoding>urn:uuid:{source_uuid}</SourceEncoding>
                            <TrackFileId>urn:uuid:{track_uuid}</TrackFileId>
                        </Resource>
""".format(
    clip_uuid=uuid.uuid4(),
    source_uuid=clip_metadata.get("source_uuid", "MISSING UUID for "+clip.name),
    track_uuid=clip_metadata.get("track_uuid", "MISSING TRACK UUID"),
    edit_rate=edit_rate,
    intrinsic_duration=intrinsic_duration,
    source_duration=source_duration,
    entry=entry
)

    output += """
                    </ResourceList>
                </cc:MainImageSequence>
            </SequenceList>
        </Segment>
    </SegmentList>
</CompositionPlaylist>
"""

    return output
