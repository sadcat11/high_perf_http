#pragma once

#include "file_descriptor.hpp"
#include "proxy.hpp"

namespace hph
{

    /**
     * Creates and binds a TCP listener.
     * @param config Listener configuration
     * @return Owned listener or empty descriptor on error
     */
    FileDescriptor create_listener(const ProxyConfig &config);

    /**
     * Connects to the configured backend.
     * @param config Backend configuration
     * @return Owned socket or empty descriptor on error
     */
    FileDescriptor connect_backend(const ProxyConfig &config);

}