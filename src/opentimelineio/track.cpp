#include "opentimelineio/track.h"
#include "opentimelineio/transition.h"
#include "opentimelineio/gap.h"
#include "opentimelineio/vectorIndexing.h"

Track::Track(std::string const& name,
             optional<TimeRange> const& source_range,
             std::string const& kind,
             AnyDictionary const& metadata)
    : Parent( name, source_range, metadata),
      _kind(kind) {
}

Track::~Track() {
}

std::string const& Track::composition_kind() const {
    static std::string kind = "Track";
    return kind;
}

bool Track::read_from(Reader& reader) {
    return reader.read("kind", &_kind) &&
        Parent::read_from(reader);
}

void Track::write_to(Writer& writer) const {
    Parent::write_to(writer);
    writer.write("kind", _kind);
}

static RationalTime _safe_duration(Composable* c, ErrorStatus* error_status) {
    if (auto item = dynamic_cast<Item*>(c)) {
        return item->duration(error_status);
    }
    else if (auto transition = dynamic_cast<Transition*>(c)) {
        return transition->duration(error_status);
    }
    else {
        *error_status = ErrorStatus(ErrorStatus::OBJECT_WITHOUT_DURATION,
                                    "Cannot determine duration from this kind of object", c);
        return RationalTime();
    }
}

TimeRange Track::range_of_child_at_index(int index, ErrorStatus* error_status) const {
    index = adjusted_vector_index(index, children());
    if (index < 0 || index >= int(children().size())) {
        *error_status = ErrorStatus::ILLEGAL_INDEX;
        return TimeRange();
    }
    
    Composable* child = children()[index];
    RationalTime child_duration = _safe_duration(child, error_status);
    if (*error_status) {
        return TimeRange();
    }
    
    RationalTime start_time(0, child_duration.rate());
    
    for (int i = 0; i < index; i++) {
        start_time += _safe_duration(children()[i].value, error_status);
        if (*error_status) {
            return TimeRange();
        }
    }
    
    if (auto transition = dynamic_cast<Transition*>(child)) {
        start_time -= transition->in_offset();
    }
    
    return TimeRange(start_time, child_duration);
}

TimeRange Track::trimmed_range_of_child_at_index(int index, ErrorStatus* error_status) const {
    auto child_range = range_of_child_at_index(index, error_status);
    if (*error_status) {
        return child_range;
    }
    
    auto trimmed_range = trim_child_range(child_range);
    if (!trimmed_range) {
        *error_status = ErrorStatus::INVALID_TIME_RANGE;
        return TimeRange();
    }
    
    return *trimmed_range;
}

TimeRange Track::available_range(ErrorStatus* error_status) const {
    RationalTime duration;
    for (auto child: children()) {
        if (auto item = dynamic_cast<Item*>(child.value)) {
            duration += item->duration(error_status);
            if (*error_status) {
                return TimeRange();
            }
        }
    }
    
    if (!children().empty()) {
        if (auto transition = dynamic_cast<Transition*>(children().front().value)) {
            duration += transition->in_offset();
        }
        if (auto transition = dynamic_cast<Transition*>(children().back().value)) {
            duration += transition->out_offset();
        }
    }

    return TimeRange(RationalTime(0, duration.rate()), duration);
}

std::pair<optional<RationalTime>, optional<RationalTime>>
Track::handles_of_child(Composable const* child, ErrorStatus* error_status) const {
    optional<RationalTime> head, tail;
    auto neighbors = neighbors_of(child, error_status);
    if (auto transition = dynamic_cast<Transition const*>(neighbors.first.value)) {
        *head = transition->in_offset();
    }
    if (auto transition = dynamic_cast<Transition const*>(neighbors.second.value)) {
        *tail = transition->out_offset();
    }
    return std::make_pair(head, tail);
}

std::pair<Composable::Retainer<Composable>, Composable::Retainer<Composable>>
Track::neighbors_of(Composable const* item, ErrorStatus* error_status, NeighborGapPolicy insert_gap) const {
    std::pair<Retainer<Composable>, Retainer<Composable>> result { nullptr, nullptr };
    
    auto index = _index_of_child(item, error_status);
    if (*error_status) {
        return result;
    }
    
    if (index == 0) {
        if (insert_gap == NeighborGapPolicy::around_transitions) {
            if (auto transition = dynamic_cast<Transition const*>(item)) {
                result.first = new Gap(TimeRange(RationalTime(), transition->in_offset()));
            }
        }
    }
    else {
        result.first = children()[index - 1];
    }
    
    if (index == int(children().size()) - 1) {
        if (insert_gap == NeighborGapPolicy::around_transitions) {
            if (auto transition = dynamic_cast<Transition const*>(item)) {
                result.second = new Gap(TimeRange(RationalTime(), transition->out_offset()));
            }
        }
    }
    else {
        result.second = children()[index + 1];
    }

    return result;
}

std::map<Composable*, TimeRange> Track::range_of_all_children(ErrorStatus* error_status) const {
    std::map<Composable*, TimeRange> result;
    if (children().empty()) {
        return result;
    }
    
    auto first_child = children().front().value;
    double rate = 1;
    
    if (auto transition = dynamic_cast<Transition*>(first_child)) {
        rate = transition->in_offset().rate();
    }
    else if (auto item = dynamic_cast<Item*>(first_child)) {
        rate = item->trimmed_range(error_status).duration().rate();
        if (*error_status) {
            return result;
        }
    }
    
    RationalTime last_end_time(0, rate);
    for (auto child: children()) {
        if (auto transition = dynamic_cast<Transition*>(child.value)) {
            result[child] = TimeRange(last_end_time - transition->in_offset(),
                                      transition->out_offset() + transition->in_offset());
        }
        else if (auto item = dynamic_cast<Item*>(child.value)) {
            auto last_range = TimeRange(last_end_time, item->trimmed_range(error_status).duration());
            result[child] = last_range;
            last_end_time = last_range.end_time_exclusive();
        }

        if (*error_status) {
            return result;
        }
    }

    return result;
}
