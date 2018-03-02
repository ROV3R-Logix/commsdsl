#include "BundleFieldImpl.h"

#include <cassert>
#include <limits>
#include <numeric>
#include <utility>
#include <algorithm>

namespace bbmp
{

namespace
{

const XmlWrap::NamesList& supportedTypes()
{
    static const XmlWrap::NamesList Names = FieldImpl::supportedTypes();

    return Names;
}

XmlWrap::NamesList getExtraNames()
{
    auto names = supportedTypes();
    names.push_back(common::membersStr());
    return names;
}

} // namespace

BundleFieldImpl::BundleFieldImpl(xmlNodePtr node, ProtocolImpl& protocol)
  : Base(node, protocol)
{
}

Object::ObjKind BundleFieldImpl::objKindImpl() const
{
    return ObjKind::Bundle;
}

const XmlWrap::NamesList& BundleFieldImpl::extraChildrenNamesImpl() const
{
    static const XmlWrap::NamesList Names = getExtraNames();
    return Names;
}

bool BundleFieldImpl::parseImpl()
{
    auto membersNodes = XmlWrap::getChildren(getNode(), common::membersStr());
    if (1U < membersNodes.size()) {
        logError() << XmlWrap::logPrefix(getNode()) <<
                      "Only single \"" << common::membersStr() << "\" child element is "
                      "supported for \"" << common::bundleStr() << "\".";
        return false;
    }

    auto memberFieldsTypes = XmlWrap::getChildren(getNode(), supportedTypes());
    if ((0U < membersNodes.size()) && (0U < memberFieldsTypes.size())) {
        logError() << XmlWrap::logPrefix(getNode()) <<
                      "The \"" << common::bundleStr() << "\" element does not support "
                      "list of stand member fields as child elements together with \"" <<
                      common::membersStr() << "\" child element.";
        return false;
    }

    if ((0U == membersNodes.size()) && (0U == memberFieldsTypes.size())) {
        logError() << XmlWrap::logPrefix(getNode()) <<
                      "The \"" << common::bundleStr() << "\" must contain member fields.";
        return false;
    }

    if ((0U < memberFieldsTypes.size())) {
        assert(0U == membersNodes.size());
        auto allChildren = XmlWrap::getChildren(getNode());
        if (allChildren.size() != memberFieldsTypes.size()) {
            logError() << XmlWrap::logPrefix(getNode()) <<
                          "The member types of \"" << common::bundleStr() <<
                          "\" must be defined inside \"<" << common::membersStr() << ">\" child element "
                          "when there are other property describing children.";
            return false;
        }
    }

    if (0U < membersNodes.size()) {
        assert(0U == memberFieldsTypes.size());
        memberFieldsTypes = XmlWrap::getChildren(membersNodes.front());
        auto cleanMemberFieldsTypes = XmlWrap::getChildren(membersNodes.front(), supportedTypes());
        if (cleanMemberFieldsTypes.size() != memberFieldsTypes.size()) {
            logError() << XmlWrap::logPrefix(membersNodes.front()) <<
                          "The \"" << common::membersStr() << "\" child node of \"" <<
                          common::bundleStr() << "\" element must contain only supported types.";
            return false;
        }

        // memberFieldsTypes is updated with the list from <members>
    }

    m_members.reserve(m_members.size() + memberFieldsTypes.size());
    for (auto* memNode : memberFieldsTypes) {
        std::string memKind(reinterpret_cast<const char*>(memNode->name));
        auto mem = FieldImpl::create(memKind, memNode, protocol());
        if (!mem) {
            assert(!"Internal error");
            logError() << XmlWrap::logPrefix(getNode()) <<
                          "Internal error, failed to create objects for member fields.";
            return false;
        }

        mem->setParent(this);
        if (!mem->parse()) {
            return false;
        }

        m_members.push_back(std::move(mem));
    }
    return true;
}

bool BundleFieldImpl::validateImpl()
{
    for (auto& mem : m_members) {
        if (!mem->validate()) {
            return false;
        }
    }

    return true;
}

std::size_t BundleFieldImpl::lengthImpl() const
{
    return
        std::accumulate(
            m_members.begin(), m_members.end(), 0U,
            [](std::size_t soFar, auto& m)
            {
                return soFar + m->length();
            });
}



} // namespace bbmp
