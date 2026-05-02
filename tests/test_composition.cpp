// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenTimelineIO project

#include "utils.h"

#include <opentimelineio/clip.h>
#include <opentimelineio/composition.h>
#include <opentimelineio/item.h>
#include <opentimelineio/stack.h>
#include <opentimelineio/track.h>
#include <opentimelineio/transition.h>

#include <iostream>

using namespace OTIO_NS;

int
main(int argc, char** argv)
{
    Tests tests;

    // test a basic case of find_children
    tests.add_test("test_find_children", [] {
        SerializableObject::Retainer<Composition> comp = new Composition;
        SerializableObject::Retainer<Item>        item = new Item;

        comp->append_child(item);
        OTIO_NS::ErrorStatus err;
        auto                 result = comp->find_children<>(&err);
        assertEqual(result.size(), 1);
        assertEqual(result[0].value, item.value);
    });

    // test stack and track correctly calls find_clips from composition parent class
    tests.add_test("test_find_clips", [] {
        SerializableObject::Retainer<Stack>      stack      = new Stack();
        SerializableObject::Retainer<Track>      track      = new Track;
        SerializableObject::Retainer<Clip>       clip       = new Clip;
        SerializableObject::Retainer<Transition> transition = new Transition;

        stack->append_child(track);
        track->append_child(transition);
        track->append_child(clip);

        OTIO_NS::ErrorStatus err;
        auto                 items = stack->find_clips(&err);
        assertFalse(is_error(err));
        assertEqual(items.size(), 1);
        assertEqual(items[0].value, clip.value);

        items = track->find_clips(&err);
        assertFalse(is_error(err));
        assertEqual(items.size(), 1);
        assertEqual(items[0].value, clip.value);
    });

    tests.add_test("regression: test_remove_child_oob_index", [] {
        auto track = new Track;
        auto clip  = new Clip;

        track->append_child(clip);

        OTIO_NS::ErrorStatus err;
        bool                 result = track->remove_child(999, &err);

        assertFalse(result);
        assertTrue(is_error(err));
    });

    tests.add_test("regression: test_remove_child_negative_oob_index", [] {
        auto track = new Track;
        auto clip1 = new Clip;
        auto clip2 = new Clip;

        track->append_child(clip1);
        track->append_child(clip2);

        OTIO_NS::ErrorStatus err;
        bool                 result = track->remove_child(-5, &err);

        assertFalse(result);
        assertTrue(is_error(err));
    });

    tests.run(argc, argv);
    return 0;
}
