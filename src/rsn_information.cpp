/*
 * Copyright (c) 2014, Matias Fontanini
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above
 *   copyright notice, this list of conditions and the following disclaimer
 *   in the documentation and/or other materials provided with the
 *   distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "rsn_information.h"
#ifdef HAVE_DOT11

#include <cstring>
#include <stdexcept>
#include "exceptions.h"
#include "pdu_option.h"
#include "memory_helpers.h"
#include "dot11/dot11_base.h"

using Tins::Memory::InputMemoryStream;

namespace Tins {

RSNInformation::RSNInformation() : _version(1), _capabilities(0) {

}

RSNInformation::RSNInformation(const serialization_type &buffer) {
    init(&buffer[0], static_cast<uint32_t>(buffer.size()));
}

RSNInformation::RSNInformation(const uint8_t *buffer, uint32_t total_sz) {
    init(buffer, total_sz);
}

void RSNInformation::init(const uint8_t *buffer, uint32_t total_sz) {
    InputMemoryStream stream(buffer, total_sz);
    version(Endian::le_to_host(stream.read<uint16_t>()));    
    group_suite((RSNInformation::CypherSuites)Endian::le_to_host(stream.read<uint32_t>()));
    int pairwise_cyphers_size = Endian::le_to_host(stream.read<uint16_t>());
    if (!stream.can_read(pairwise_cyphers_size)) {
        throw malformed_packet();
    }
    while (pairwise_cyphers_size--) {
        add_pairwise_cypher(
            (RSNInformation::CypherSuites)Endian::le_to_host(stream.read<uint32_t>())
        );
    }
    int akm_cyphers_size = Endian::le_to_host(stream.read<uint16_t>());
    if (!stream.can_read(akm_cyphers_size)) {
        throw malformed_packet();
    }
    while (akm_cyphers_size--) {
        add_akm_cypher(
            (RSNInformation::AKMSuites)Endian::le_to_host(stream.read<uint32_t>())
        );
    }
    capabilities(Endian::le_to_host(stream.read<uint16_t>()));
}

void RSNInformation::add_pairwise_cypher(CypherSuites cypher) {
    _pairwise_cyphers.push_back(cypher);
}

void RSNInformation::add_akm_cypher(AKMSuites akm) {
    _akm_cyphers.push_back(akm);
}

void RSNInformation::group_suite(CypherSuites group) {
    _group_suite = static_cast<CypherSuites>(Endian::host_to_le<uint32_t>(group));
}

void RSNInformation::version(uint16_t ver) {
    _version = Endian::host_to_le(ver);
}

void RSNInformation::capabilities(uint16_t cap) {
    _capabilities = Endian::host_to_le(cap);
}

RSNInformation::serialization_type RSNInformation::serialize() const {
    size_t size = sizeof(_version) + sizeof(_capabilities) + sizeof(uint32_t);
    size += (sizeof(uint16_t) << 1); // 2 lists count.
    size += sizeof(uint32_t) * (_akm_cyphers.size() + _pairwise_cyphers.size());

    const uint16_t pairwise_cyphers_size = Endian::host_to_le<uint16_t>(_pairwise_cyphers.size());
    const uint16_t akm_cyphers_size = Endian::host_to_le<uint16_t>(_akm_cyphers.size());
    
    serialization_type buffer(size);
    serialization_type::value_type *ptr = &buffer[0];
    std::memcpy(ptr, &_version, sizeof(_version));
    ptr += sizeof(uint16_t);
    std::memcpy(ptr, &_group_suite, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    std::memcpy(ptr, &pairwise_cyphers_size, sizeof(pairwise_cyphers_size));
    ptr += sizeof(uint16_t);
    for(cyphers_type::const_iterator it = _pairwise_cyphers.begin(); it != _pairwise_cyphers.end(); ++it) {
        const uint32_t value = Endian::host_to_le<uint32_t>(*it);
        std::memcpy(ptr, &value, sizeof(uint32_t));
        ptr += sizeof(uint32_t);
    }
    std::memcpy(ptr, &akm_cyphers_size, sizeof(akm_cyphers_size));
    ptr += sizeof(uint16_t);
    for(akm_type::const_iterator it = _akm_cyphers.begin(); it != _akm_cyphers.end(); ++it) {
        const uint32_t value = Endian::host_to_le<uint32_t>(*it);
        std::memcpy(ptr, &value, sizeof(uint32_t));
        ptr += sizeof(uint32_t);
    }
    std::memcpy(ptr, &_capabilities, sizeof(uint16_t));

    return buffer;
}

RSNInformation RSNInformation::wpa2_psk() {
    RSNInformation info;
    info.group_suite(RSNInformation::CCMP);
    info.add_pairwise_cypher(RSNInformation::CCMP);
    info.add_akm_cypher(RSNInformation::PSK);
    return info;
}

RSNInformation RSNInformation::from_option(const PDUOption<uint8_t, Dot11> &opt) {
    if(opt.data_size() < sizeof(uint16_t) * 2 + sizeof(uint32_t))
        throw malformed_option();
    return RSNInformation(opt.data_ptr(), static_cast<uint32_t>(opt.data_size()));
}
}

#endif // HAVE_DOT11
