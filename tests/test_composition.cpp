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
        SerializableObject::Retainer<Stack>      stack       = new Stack();
        SerializableObject::Retainer<Track>      track       = new Track;
        SerializableObject::Retainer<Clip>       clip        = new Clip;
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

    // Bug: remove_child with out-of-bounds index does not return an error,
    // because _children[index] is accessed (line 159) BEFORE the bounds
    // check (line 161).  This causes an out-of-bounds read.
    tests.add_test("test_remove_child_oob_index", [] {
        SerializableObject::Retainer<Composition> comp = new Composition;
        SerializableObject::Retainer<Clip>       clip  = new Clip;

        comp->append_child(clip);

        OTIO_NS::ErrorStatus err;
        // Removing index 999 from a 1-child composition should fail with
        // an error, but the bug causes the function to proceed without
        // validating the index first (OOB read on _children[999]).
        bool result = comp->remove_child(999, &err);

        // The function should have returned false and set an error,
        // but due to the bug it returns true and corrupts state.
        assertFalse(result);
        assertTrue(is_error(err));
     });

    tests.add_test("test_remove_child_negative_oob_index", [] {
        SerializableObject::Retainer<Composition> comp = new Composition;
        SerializableObject::Retainer<Clip>        clip1 = new Clip;
        SerializableObject::Retainer<Clip>        clip2 = new Clip;

        comp->append_child(clip1);
        comp->append_child(clip2);

        OTIO_NS::ErrorStatus err;
        // Removing index -5 from a 2-child composition should fail with
        // an error. adjusted_vector_index(-5, 2-element vec) returns -3,
        // then _children[-3] is accessed before the bounds check.
        bool result = comp->remove_child(-5, &err);

        assertFalse(result);
        assertTrue(is_error(err));
     });

    tests.run(argc, argv);
    return 0;
}
