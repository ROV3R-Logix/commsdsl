//
// Copyright 2018 - 2019 (C). Alex Robenko. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "commsdsl/Message.h"
#include <cassert>

#include "MessageImpl.h"

namespace commsdsl
{

Message::Message(const MessageImpl* impl)
  : m_pImpl(impl)
{
}

Message::Message(const Message &) = default;

Message::~Message() = default;

bool Message::valid() const
{
    return m_pImpl != nullptr;
}

const std::string& Message::name() const
{
    assert(m_pImpl != nullptr);
    return m_pImpl->name();
}

const std::string& Message::displayName() const
{
    assert(m_pImpl != nullptr);
    return m_pImpl->displayName();
}

const std::string& Message::description() const
{
    assert(m_pImpl != nullptr);
    return m_pImpl->description();
}

std::uintmax_t Message::id() const
{
    assert(m_pImpl != nullptr);
    return m_pImpl->id();
}

unsigned Message::order() const
{
    assert(m_pImpl != nullptr);
    return m_pImpl->order();
}

std::size_t Message::minLength() const
{
    assert(m_pImpl != nullptr);
    return m_pImpl->minLength();
}

std::size_t Message::maxLength() const
{
    assert(m_pImpl != nullptr);
    return m_pImpl->maxLength();
}

unsigned Message::sinceVersion() const
{
    assert(m_pImpl != nullptr);
    return m_pImpl->getSinceVersion();
}

unsigned Message::deprecatedSince() const
{
    assert(m_pImpl != nullptr);
    return m_pImpl->getDeprecated();
}

bool Message::isDeprecatedRemoved() const
{
    assert(m_pImpl != nullptr);
    return m_pImpl->isDeprecatedRemoved();
}

Message::FieldsList Message::fields() const
{
    assert(m_pImpl != nullptr);
    return m_pImpl->fieldsList();
}

std::string Message::externalRef() const
{
    assert(m_pImpl != nullptr);
    return m_pImpl->externalRef();
}

bool Message::isCustomizable() const
{
    assert(m_pImpl != nullptr);
    return m_pImpl->isCustomizable();
}

Message::Sender Message::sender() const
{
    assert(m_pImpl != nullptr);
    return m_pImpl->sender();
}

const Message::AttributesMap& Message::extraAttributes() const
{
    assert(m_pImpl != nullptr);
    return m_pImpl->extraAttributes();
}

const Message::ElementsList& Message::extraElements() const
{
    assert(m_pImpl != nullptr);
    return m_pImpl->extraChildren();
}

const Message::PlatformsList& Message::platforms() const
{
    assert(m_pImpl != nullptr);
    return m_pImpl->platforms();
}


} // namespace commsdsl
