//
// Created by tim on 21.04.2021.
//

#pragma once
#include "Common.h"
#include <variant>

using FrameData = std::array<uint8_t, s_dmxUniverseSize>;

struct Frame {
    uint16_t universe;
    FrameData data;
};

struct ArtNetConf {
    string broadcastName;
    vector<uint16_t> activeUniverses;
};

struct LmConf {
    vector<uint16_t> activeUniverses;
};

using InputConf = std::variant<ArtNetConf, LmConf>;

inline const std::vector<uint16_t> s_emptyVector = {};

static inline const vector<uint16_t> &GetActiveUniverses(const InputConf &conf)
{
    if (std::holds_alternative<ArtNetConf>(conf))
        return std::get<ArtNetConf>(conf).activeUniverses;
    else if (std::holds_alternative<LmConf>(conf))
        return std::get<LmConf>(conf).activeUniverses;

    assert(false);
    return s_emptyVector;
}